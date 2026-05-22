// tests/fairino_zmq_robot_agent.cpp

#include "monitoring/FairinoMonitorService.h"
#include "monitoring/RobotSnapshot.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QThread>

#ifdef ENABLE_ZEROMQ_TRANSPORT
#include <zmq.hpp>
#endif

#include <array>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <mutex>
#include <string>

namespace
{
std::atomic_bool g_running { true };

void handleSignal(int)
{
    g_running = false;
}

constexpr int kMaxRobotCount = 2;
constexpr int kJogHeartbeatTimeoutMs = 500;

using SteadyClock = std::chrono::steady_clock;

struct JogWatchdogState
{
    bool active = false;
    int joint = 0;
    QString sessionId;
    SteadyClock::time_point lastHeartbeat;
};


struct CommandRequest
{
    bool parsed = false;
    QString error;

    QString commandId;
    int robotId = 0;
    QString command;
    QJsonObject params;
};

using JogWatchdogStates =
    std::array<JogWatchdogState, kMaxRobotCount + 1>;

QString defaultJogSessionId(const CommandRequest& request)
{
    const QString requested =
        request.params.value("jogSessionId").toString();

    if (!requested.isEmpty()) {
        return requested;
    }

    return request.commandId;
}

template <typename T, std::size_t N>
QJsonArray toJsonArray(const std::array<T, N>& values)
{
    QJsonArray array;

    for (const auto& value : values) {
        array.append(static_cast<double>(value));
    }

    return array;
}

QString toIsoTimestamp(const std::chrono::system_clock::time_point& timestamp)
{
    const auto millis =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            timestamp.time_since_epoch()).count();

    return QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(millis))
        .toString(Qt::ISODateWithMs);
}

QByteArray buildSnapshotPayload(
    int robotId,
    const monitoring::FairinoMonitorService::SnapshotWithMeta& data)
{
    const monitoring::RobotSnapshot& s = data.snapshot;

    QJsonObject snapshot;
    snapshot["messageType"] = QStringLiteral("snapshot");
    snapshot["schemaVersion"] = 1;

    snapshot["robotId"] = robotId;
    snapshot["robotName"] = QStringLiteral("FR%1").arg(robotId);
    snapshot["model"] = robotId == 1
                            ? QStringLiteral("FR10")
                            : QStringLiteral("FR5");

    snapshot["connected"] = s.connected;

    // motion_done은 "동작 중" 의미와 다를 수 있음.
    // 현재는 기존 UI 호환용으로 유지.
    snapshot["running"] = s.motion_done == 0 ? true : false;

    snapshot["jointPositions"] = toJsonArray(s.joint_pos_deg);
    snapshot["tcpPose"] = toJsonArray(s.tcp_pose);

    // 실측 결과 joint_torque는 0으로 들어오고 driver_torque가 유효하므로
    // payload의 torques는 driver_torque 우선 사용.
    snapshot["torques"] =
        s.driver_torque_valid
            ? toJsonArray(s.driver_torque)
            : toJsonArray(s.joint_torque);

    snapshot["torqueSource"] =
        s.driver_torque_valid
            ? QStringLiteral("driver_torque")
            : QStringLiteral("joint_torque");

    snapshot["driverTemperatures"] = toJsonArray(s.driver_temperature);

    snapshot["robotState"] =
        s.emergency_stop ? QStringLiteral("ESTOP")
                         : QStringLiteral("RUN");

    snapshot["errorText"] =
        QString::fromStdString(s.last_error.empty()
                                   ? s.last_poll_error
                                   : s.last_error);

    if (snapshot["errorText"].toString().isEmpty()) {
        snapshot["errorText"] = QStringLiteral("0");
    }

    snapshot["sequenceState"] = QStringLiteral("SDK");
    snapshot["interlockState"] =
        s.emergency_stop ? QStringLiteral("NG") : QStringLiteral("OK");

    snapshot["timestamp"] = toIsoTimestamp(data.timestamp);
    snapshot["sequenceNumber"] =
        static_cast<double>(data.sequence_number);

    return QJsonDocument(snapshot).toJson(QJsonDocument::Compact);
}

bool publishSnapshot(zmq::socket_t& socket,
                     std::mutex& publishMutex,
                     int robotId,
                     const QByteArray& payload)
{
    const QString topic = QStringLiteral("robot/%1").arg(robotId);
    const QByteArray topicPayload = topic.toUtf8();

    std::lock_guard<std::mutex> lock(publishMutex);

    const auto topicResult =
        socket.send(zmq::buffer(topicPayload.constData(),
                                static_cast<size_t>(topicPayload.size())),
                    zmq::send_flags::sndmore);

    if (!topicResult.has_value()) {
        qWarning() << "[FairinoZmqRobotAgent] topic send failed"
                   << "robotId =" << robotId;
        return false;
    }

    const auto payloadResult =
        socket.send(zmq::buffer(payload.constData(),
                                static_cast<size_t>(payload.size())),
                    zmq::send_flags::none);

    if (!payloadResult.has_value()) {
        qWarning() << "[FairinoZmqRobotAgent] payload send failed"
                   << "robotId =" << robotId;
        return false;
    }

    return true;
}


CommandRequest parseCommandRequest(const QByteArray& payload)
{
    CommandRequest result;

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(payload, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        result.error =
            QStringLiteral("JSON parse error: %1").arg(parseError.errorString());
        return result;
    }

    if (!doc.isObject()) {
        result.error = QStringLiteral("JSON root is not an object");
        return result;
    }

    const QJsonObject object = doc.object();

    const QString messageType = object.value("messageType").toString();
    if (messageType != QStringLiteral("commandRequest")) {
        result.error =
            QStringLiteral("Invalid messageType: %1").arg(messageType);
        return result;
    }

    result.commandId = object.value("commandId").toString();
    result.robotId = object.value("robotId").toInt(0);
    result.command = object.value("command").toString();
    result.params = object.value("params").toObject();

    if (result.commandId.isEmpty()) {
        result.error = QStringLiteral("Missing commandId");
        return result;
    }

    if (result.robotId <= 0) {
        result.error = QStringLiteral("Invalid robotId");
        return result;
    }

    if (result.command.isEmpty()) {
        result.error = QStringLiteral("Missing command");
        return result;
    }

    result.parsed = true;
    return result;
}

QByteArray buildResponse(const CommandRequest& request,
                         bool ok,
                         int code,
                         const QString& message)
{
    QJsonObject response;
    response["messageType"] = QStringLiteral("commandResponse");
    response["schemaVersion"] = 1;
    response["timestamp"] =
        QDateTime::currentDateTime().toString(Qt::ISODateWithMs);

    response["commandId"] = request.commandId;
    response["robotId"] = request.robotId;
    response["command"] = request.command;

    response["ok"] = ok;
    response["code"] = code;
    response["message"] = message;

    return QJsonDocument(response).toJson(QJsonDocument::Compact);
}

QByteArray buildParseErrorResponse(const QString& error)
{
    QJsonObject response;
    response["messageType"] = QStringLiteral("commandResponse");
    response["schemaVersion"] = 1;
    response["timestamp"] =
        QDateTime::currentDateTime().toString(Qt::ISODateWithMs);

    // 기존 parser는 commandId, robotId, command가 없으면 parse 실패함.
    // parse error 자체도 commandResponse 형식으로 유지하기 위한 fallback.
    response["commandId"] = QStringLiteral("invalid");
    response["robotId"] = 1;
    response["command"] = QStringLiteral("invalid");

    response["ok"] = false;
    response["code"] = 101;
    response["message"] = error;

    return QJsonDocument(response).toJson(QJsonDocument::Compact);
}

monitoring::FairinoMonitorService* selectRobot(
    int robotId,
    monitoring::FairinoMonitorService& robot1,
    monitoring::FairinoMonitorService& robot2)
{
    if (robotId == 1)
        return &robot1;

    if (robotId == 2)
        return &robot2;

    return nullptr;
}

void checkJogTimeouts(JogWatchdogStates& jogStates,
                      monitoring::FairinoMonitorService& robot1,
                      monitoring::FairinoMonitorService& robot2)
{
    const auto now = SteadyClock::now();

    for (int robotId = 1; robotId <= kMaxRobotCount; ++robotId) {
        JogWatchdogState& state = jogStates[robotId];

        if (!state.active) {
            continue;
        }

        const auto elapsedMs =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                now - state.lastHeartbeat).count();

        if (elapsedMs < kJogHeartbeatTimeoutMs) {
            continue;
        }

        monitoring::FairinoMonitorService* robot =
            selectRobot(robotId, robot1, robot2);

        if (robot && robot->isRunning()) {
            const auto result = robot->stopJointJogEx();

            qWarning() << "[FairinoZmqRobotAgent]"
                       << "jog heartbeat timeout stop"
                       << "robotId =" << robotId
                       << "joint =" << state.joint
                       << "elapsedMs =" << static_cast<qlonglong>(elapsedMs)
                       << "ok =" << result.ok
                       << "code =" << result.code
                       << "message =" << QString::fromStdString(result.message);
        }

        state.active = false;
        state.joint = 0;
        state.sessionId.clear();
    }
}

QByteArray executeCommand(const CommandRequest& request,
                          monitoring::FairinoMonitorService& robot1,
                          monitoring::FairinoMonitorService& robot2,
                          JogWatchdogStates& jogStates)
{
    monitoring::FairinoMonitorService* robot =
        selectRobot(request.robotId, robot1, robot2);

    if (!robot) {
        return buildResponse(request,
                             false,
                             102,
                             QStringLiteral("Robot not found: %1")
                                 .arg(request.robotId));
    }

    if (!robot->isRunning()) {
        return buildResponse(request,
                             false,
                             200,
                             QStringLiteral("Robot is not connected: %1")
                                 .arg(request.robotId));
    }

    monitoring::FairinoMonitorService::CommandResult result;

    if (request.command == QStringLiteral("setManualMode")) {
        result = robot->setManualModeEx();

    } else if (request.command == QStringLiteral("setAutoMode")) {
        result = robot->setAutoModeEx();

    } else if (request.command == QStringLiteral("clearError")) {
        result = robot->clearErrorEx();

    } else if (request.command == QStringLiteral("stopJointJog")) {
        result = robot->stopJointJogEx();

        if (request.robotId >= 1 && request.robotId <= kMaxRobotCount) {
            JogWatchdogState& state = jogStates[request.robotId];

            state.active = false;
            state.joint = 0;
            state.sessionId.clear();

            qInfo() << "[FairinoZmqRobotAgent]"
                    << "jog watchdog stopped"
                    << "robotId =" << request.robotId;
        }

    } else if (request.command == QStringLiteral("startJointJog")) {
        const int joint =
            request.params.value("joint").toInt(0);

        const QString direction =
            request.params.value("direction").toString();

        const bool positive =
            direction == QStringLiteral("+") ||
            direction.compare(QStringLiteral("positive"),
                              Qt::CaseInsensitive) == 0;

        const double speed =
            request.params.value("speed").toDouble(5.0);

        // 초기 실기 테스트 안전값
        constexpr float kDefaultAccPercent = 5.0f;
        constexpr float kMaxJogDegree = 1.0f;

        result = robot->startJointJogEx(joint,
                                        positive,
                                        static_cast<float>(speed),
                                        kDefaultAccPercent,
                                        kMaxJogDegree);

        if (result.ok && request.robotId >= 1 && request.robotId <= kMaxRobotCount) {
            JogWatchdogState& state = jogStates[request.robotId];

            state.active = true;
            state.joint = joint;
            state.sessionId = defaultJogSessionId(request);
            state.lastHeartbeat = SteadyClock::now();

            qInfo() << "[FairinoZmqRobotAgent]"
                    << "jog watchdog started"
                    << "robotId =" << request.robotId
                    << "joint =" << joint
                    << "sessionId =" << state.sessionId;
        }

    } else if (request.command == QStringLiteral("jogHeartbeat")) {
        if (request.robotId < 1 || request.robotId > kMaxRobotCount) {
            return buildResponse(request,
                                 false,
                                 101,
                                 QStringLiteral("Invalid robotId for jog heartbeat"));
        }

        JogWatchdogState& state = jogStates[request.robotId];

        if (!state.active) {
            return buildResponse(request,
                                 true,
                                 0,
                                 QStringLiteral("jog heartbeat ignored: no active jog"));
        }

        const QString requestedSessionId =
            request.params.value(QStringLiteral("jogSessionId")).toString();

        if (!requestedSessionId.isEmpty() &&
            !state.sessionId.isEmpty() &&
            requestedSessionId != state.sessionId) {
            return buildResponse(request,
                                 false,
                                 101,
                                 QStringLiteral("jog heartbeat session mismatch"));
        }

        state.lastHeartbeat = SteadyClock::now();

        return buildResponse(request,
                             true,
                             0,
                             QStringLiteral("jog heartbeat ok"));
    } else {
        return buildResponse(request,
                             false,
                             100,
                             QStringLiteral("Unsupported command: %1")
                                 .arg(request.command));
    }

    const QString message =
        result.message.empty()
            ? QStringLiteral("Fairino command ok: %1").arg(request.command)
            : QString::fromStdString(result.message);

    return buildResponse(request,
                         result.ok,
                         result.code,
                         message);
}
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

#ifndef ENABLE_ZEROMQ_TRANSPORT
    qCritical() << "[FairinoZmqRobotAgent]"
                << "ZeroMQ transport is disabled at build time";
    return 1;
#else
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    const QString snapshotEndpoint =
        app.arguments().size() >= 2
            ? app.arguments().at(1)
            : QStringLiteral("tcp://*:5556");

    const QString commandEndpoint =
        app.arguments().size() >= 3
            ? app.arguments().at(2)
            : QStringLiteral("tcp://*:5557");

    const QString robot1Ip =
        app.arguments().size() >= 4
            ? app.arguments().at(3)
            : QStringLiteral("192.168.57.121");

    const QString robot2Ip =
        app.arguments().size() >= 5
            ? app.arguments().at(4)
            : QStringLiteral("192.168.57.122");

    qInfo() << "[FairinoZmqRobotAgent] snapshotEndpoint =" << snapshotEndpoint;
    qInfo() << "[FairinoZmqRobotAgent] commandEndpoint =" << commandEndpoint;
    qInfo() << "[FairinoZmqRobotAgent] robot1Ip =" << robot1Ip;
    qInfo() << "[FairinoZmqRobotAgent] robot2Ip =" << robot2Ip;

    try {
        zmq::context_t context(1);

        zmq::socket_t pubSocket(context, zmq::socket_type::pub);
        pubSocket.set(zmq::sockopt::linger, 0);
        pubSocket.bind(snapshotEndpoint.toStdString());

        zmq::socket_t repSocket(context, zmq::socket_type::rep);
        repSocket.set(zmq::sockopt::linger, 0);
        repSocket.set(zmq::sockopt::rcvtimeo, 100);
        repSocket.bind(commandEndpoint.toStdString());

        std::mutex publishMutex;

        monitoring::FairinoMonitorService robot1;
        monitoring::FairinoMonitorService robot2;

        JogWatchdogStates jogStates;

        monitoring::FairinoMonitorService::Options opt1;
        opt1.robot_id = 1;
        opt1.poll_period_ms = 200;
        opt1.enable_reconnect = true;

        monitoring::FairinoMonitorService::Options opt2;
        opt2.robot_id = 2;
        opt2.poll_period_ms = 200;
        opt2.enable_reconnect = true;

        robot1.setCallback(
            [&](const monitoring::FairinoMonitorService::SnapshotWithMeta& data) {
                const QByteArray payload = buildSnapshotPayload(1, data);
                publishSnapshot(pubSocket, publishMutex, 1, payload);
            });

        robot2.setCallback(
            [&](const monitoring::FairinoMonitorService::SnapshotWithMeta& data) {
                const QByteArray payload = buildSnapshotPayload(2, data);
                publishSnapshot(pubSocket, publishMutex, 2, payload);
            });

        const bool robot1Started =
            robot1.start(robot1Ip.toStdString(), opt1);

        const bool robot2Started =
            robot2.start(robot2Ip.toStdString(), opt2);

        qInfo() << "[FairinoZmqRobotAgent] robot1Started =" << robot1Started;
        qInfo() << "[FairinoZmqRobotAgent] robot2Started =" << robot2Started;

        if (!robot1Started && !robot2Started) {
            qCritical() << "[FairinoZmqRobotAgent]"
                        << "all robot connections failed";
            return 2;
        }

        qInfo() << "[FairinoZmqRobotAgent] agent started";

        while (g_running) {
            zmq::message_t requestMessage;

            const auto recvResult =
                repSocket.recv(requestMessage, zmq::recv_flags::none);

            if (!recvResult.has_value()) {
                checkJogTimeouts(jogStates, robot1, robot2);
                continue;
            }

            const QByteArray requestPayload(
                static_cast<const char*>(requestMessage.data()),
                static_cast<int>(requestMessage.size()));

            qInfo() << "[FairinoZmqRobotAgent] command request ="
                    << requestPayload;

            const CommandRequest request =
                parseCommandRequest(requestPayload);

            QByteArray responsePayload;

            if (!request.parsed) {
                responsePayload =
                    buildParseErrorResponse(request.error);
            } else {
                responsePayload =
                    executeCommand(request, robot1, robot2, jogStates);
            }

            repSocket.send(
                zmq::buffer(responsePayload.constData(),
                            static_cast<size_t>(responsePayload.size())),
                zmq::send_flags::none);

            qInfo() << "[FairinoZmqRobotAgent] command response ="
                    << responsePayload;

            checkJogTimeouts(jogStates, robot1, robot2);
        }

        qInfo() << "[FairinoZmqRobotAgent] stopping...";

        robot1.stop();
        robot2.stop();

        repSocket.close();
        pubSocket.close();
        context.shutdown();
        context.close();

        qInfo() << "[FairinoZmqRobotAgent] stopped";
        return 0;

    } catch (const zmq::error_t& error) {
        qCritical() << "[FairinoZmqRobotAgent] ZeroMQ error:"
                    << error.what();
        return 3;
    }
#endif
}
