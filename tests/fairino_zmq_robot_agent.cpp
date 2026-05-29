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
#include <QJsonValue>
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
constexpr int kCommandMasterLeaseTimeoutMs = 3000;

using SteadyClock = std::chrono::steady_clock;

struct JogWatchdogState
{
    bool active = false;
    bool workspaceJog = false;
    int joint = 0;
    QString axis;
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
    QString operatorName;
    QJsonObject params;
};

struct CommandMasterState
{
    bool active = false;
    QString ownerId;
    SteadyClock::time_point lastCommand;
};

using JogWatchdogStates =
    std::array<JogWatchdogState, kMaxRobotCount + 1>;

QString defaultJogSessionId(const CommandRequest& request)
{
    const QString requested =
        request.params.value(QStringLiteral("jogSessionId")).toString();

    if (!requested.isEmpty()) {
        return requested;
    }

    return request.commandId;
}

QString commandOwnerId(const CommandRequest& request)
{
    if (!request.operatorName.trimmed().isEmpty()) {
        return request.operatorName.trimmed();
    }

    // 현재 GUI에서 operator를 아직 안 보내므로 fallback 사용.
    // 다중 GUI 제어권 구분은 다음 단계에서 clientId/operatorName 전달 필요.
    return QStringLiteral("default-controller");
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
//    qDebug()<<s.tcp_pose[0]<<s.tcp_pose[1]<<s.tcp_pose[2]<<s.tcp_pose[3]<<s.tcp_pose[4]<<s.tcp_pose[5];
//    qDebug()<<"비상버튼 "<<s.safety_si0<<s.safety_si1;

    snapshot["safetySi0"] = static_cast<int>(s.safety_si0);
    snapshot["safetySi1"] = static_cast<int>(s.safety_si1);

    const bool safetyStop = (s.safety_si0 != 0 || s.safety_si1 != 0);
    snapshot["safetyStop"] = safetyStop;

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
        (s.emergency_stop || safetyStop)
            ? QStringLiteral("NG")
            : QStringLiteral("OK");

    snapshot["reconnecting"] = s.reconnecting;
    snapshot["reconnectCount"] = s.reconnect_count;
    snapshot["lastRecoveryMessage"] =
        QString::fromStdString(s.last_recovery_message);
    snapshot["lastRecoveryEpochMs"] =
        static_cast<double>(s.last_recovery_epoch_ms);
    snapshot["recoveryEventId"] = s.recovery_event_id;

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
    result.operatorName = object.value(QStringLiteral("operator")).toString();
    result.params = object.value("params").toObject();

    const auto copyTopLevelParam = [&](const QString& key)
    {
        if (!result.params.contains(key) && object.contains(key)) {
            result.params.insert(key, object.value(key));
        }
    };

    copyTopLevelParam(QStringLiteral("joint"));
    copyTopLevelParam(QStringLiteral("axis"));
    copyTopLevelParam(QStringLiteral("direction"));
    copyTopLevelParam(QStringLiteral("frame"));
    copyTopLevelParam(QStringLiteral("ioName"));
    copyTopLevelParam(QStringLiteral("state"));
    copyTopLevelParam(QStringLiteral("speed"));
    copyTopLevelParam(QStringLiteral("jogSessionId"));

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

bool isStartWorkspaceJogCommand(const QString& command)
{
    return command == QStringLiteral("startWorkspaceJog") ||
           command == QStringLiteral("startBaseJog");
}

bool isStopWorkspaceJogCommand(const QString& command)
{
    return command == QStringLiteral("stopWorkspaceJog") ||
           command == QStringLiteral("stopBaseJog");
}

bool isSupportedRobotIoName(int robotId, const QString& ioName)
{
    if (robotId == 1) {
        return ioName == QStringLiteral("toolLock") ||
               ioName == QStringLiteral("bulkVacuum") ||
               ioName == QStringLiteral("sortingVacuum");
    }

    if (robotId == 2) {
        return ioName == QStringLiteral("magnet") ||
               ioName == QStringLiteral("clamp1") ||
               ioName == QStringLiteral("clamp2");
    }

    return false;
}

bool parseBoolValue(const QJsonValue& value, bool& out)
{
    if (value.isBool()) {
        out = value.toBool();
        return true;
    }

    if (value.isDouble()) {
        out = value.toInt() != 0;
        return true;
    }

    if (value.isString()) {
        const QString text = value.toString().trimmed().toLower();
        if (text == QStringLiteral("true") ||
            text == QStringLiteral("on") ||
            text == QStringLiteral("1")) {
            out = true;
            return true;
        }

        if (text == QStringLiteral("false") ||
            text == QStringLiteral("off") ||
            text == QStringLiteral("0")) {
            out = false;
            return true;
        }
    }

    return false;
}

bool hasSafetyStop(const monitoring::RobotSnapshot& snapshot)
{
    return snapshot.emergency_stop != 0 ||
           snapshot.safety_si0 != 0 ||
           snapshot.safety_si1 != 0;
}

bool requiresCommandMaster(const QString& command)
{
    return command == QStringLiteral("setManualMode") ||
           command == QStringLiteral("setAutoMode") ||
           command == QStringLiteral("clearError") ||
           command == QStringLiteral("startJointJog") ||
           isStartWorkspaceJogCommand(command) ||
           command == QStringLiteral("jogHeartbeat");
}

bool isSafetyStopCommand(const QString& command)
{
    return command == QStringLiteral("stopJointJog") ||
           isStopWorkspaceJogCommand(command) ||
           command == QStringLiteral("stop");
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

bool ensureCommandMaster(const CommandRequest& request,
                         CommandMasterState& masterState,
                         QString& rejectMessage)
{
    if (isSafetyStopCommand(request.command)) {
        return true;
    }

    if (!requiresCommandMaster(request.command)) {
        return true;
    }

    const QString ownerId = commandOwnerId(request);
    const auto now = SteadyClock::now();

    if (masterState.active) {
        const auto elapsedMs =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                now - masterState.lastCommand).count();

        if (elapsedMs >= kCommandMasterLeaseTimeoutMs) {
            qWarning() << "[FairinoZmqRobotAgent]"
                       << "command master lease expired"
                       << "owner =" << masterState.ownerId
                       << "elapsedMs =" << static_cast<qlonglong>(elapsedMs);

            masterState.active = false;
            masterState.ownerId.clear();
        }
    }

    if (!masterState.active) {
        masterState.active = true;
        masterState.ownerId = ownerId;
        masterState.lastCommand = now;

        qInfo() << "[FairinoZmqRobotAgent]"
                << "command master acquired"
                << "owner =" << masterState.ownerId;

        return true;
    }

    if (masterState.ownerId == ownerId) {
        masterState.lastCommand = now;
        return true;
    }

    rejectMessage =
        QStringLiteral("Command master is held by %1")
            .arg(masterState.ownerId);

    return false;
}

void checkJogTimeouts(JogWatchdogStates& jogStates,
                      monitoring::FairinoMonitorService& robot1,
                      monitoring::FairinoMonitorService& robot2,
                      CommandMasterState& masterState)
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
            const auto result =
                state.workspaceJog
                    ? robot->stopWorkspaceJogEx()
                    : robot->stopJointJogEx();

            qWarning() << "[FairinoZmqRobotAgent]"
                       << "jog heartbeat timeout stop"
                       << "robotId =" << robotId
                       << "kind =" << (state.workspaceJog ? "workspace" : "joint")
                       << "joint =" << state.joint
                       << "axis =" << state.axis
                       << "elapsedMs =" << static_cast<qlonglong>(elapsedMs)
                       << "ok =" << result.ok
                       << "code =" << result.code
                       << "message =" << QString::fromStdString(result.message);
        }

        state.active = false;
        state.workspaceJog = false;
        state.joint = 0;
        state.axis.clear();
        state.sessionId.clear();

        if (masterState.active) {
            qInfo() << "[FairinoZmqRobotAgent]"
                    << "command master released by jog timeout"
                    << "owner =" << masterState.ownerId
                    << "robotId =" << robotId;

            masterState.active = false;
            masterState.ownerId.clear();
        }
    }
}

QByteArray executeCommand(const CommandRequest& request,
                          monitoring::FairinoMonitorService& robot1,
                          monitoring::FairinoMonitorService& robot2,
                          JogWatchdogStates& jogStates,
                          CommandMasterState& masterState)
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

    QString masterRejectMessage;
    if (!ensureCommandMaster(request, masterState, masterRejectMessage)) {
        return buildResponse(request,
                             false,
                             400,
                             masterRejectMessage);
    }

    if (!robot->isRunning()) {
        if (isStopWorkspaceJogCommand(request.command)) {
            if (request.robotId >= 1 && request.robotId <= kMaxRobotCount) {
                JogWatchdogState& state = jogStates[request.robotId];
                state.active = false;
                state.workspaceJog = false;
                state.joint = 0;
                state.axis.clear();
                state.sessionId.clear();
            }

            if (masterState.active) {
                masterState.active = false;
                masterState.ownerId.clear();
            }

            return buildResponse(request,
                                 true,
                                 0,
                                 QStringLiteral("workspace jog stop ignored: robot not connected"));
        }

        return buildResponse(request,
                             false,
                             200,
                             QStringLiteral("Robot is not connected: %1")
                                 .arg(request.robotId));
    }

    if (request.command == QStringLiteral("setRobotIo")) {
        const QString ioName =
            request.params.value(QStringLiteral("ioName")).toString().trimmed();

        if (ioName.isEmpty()) {
            return buildResponse(request,
                                 false,
                                 -11002,
                                 QStringLiteral("Missing robot IO name"));
        }

        if (!isSupportedRobotIoName(request.robotId, ioName)) {
            return buildResponse(
                request,
                false,
                -11001,
                QStringLiteral("Unsupported robot IO: robotId=%1, ioName=%2")
                    .arg(request.robotId)
                    .arg(ioName));
        }

        bool state = false;
        if (!parseBoolValue(request.params.value(QStringLiteral("state")), state)) {
            return buildResponse(request,
                                 false,
                                 -11002,
                                 QStringLiteral("Invalid or missing robot IO state"));
        }

        const monitoring::RobotSnapshot latest = robot->latest();
        if (hasSafetyStop(latest)) {
            return buildResponse(request,
                                 false,
                                 -11003,
                                 QStringLiteral("Robot IO blocked by safety stop"));
        }

        if (request.robotId >= 1 && request.robotId <= kMaxRobotCount &&
            jogStates[request.robotId].active) {
            return buildResponse(request,
                                 false,
                                 -11004,
                                 QStringLiteral("Robot IO blocked while jog is active"));
        }

        qInfo() << "[FairinoZmqRobotAgent]"
                << "robot IO interface accepted"
                << "robotId =" << request.robotId
                << "ioName =" << ioName
                << "state =" << state
                << "mapping =" << "pending";

        return buildResponse(
            request,
            true,
            0,
            QStringLiteral("Robot IO interface accepted: ioName=%1, state=%2, mapping=pending")
                .arg(ioName)
                .arg(state ? QStringLiteral("true") : QStringLiteral("false")));
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
            state.workspaceJog = false;
            state.joint = 0;
            state.axis.clear();
            state.sessionId.clear();

            qInfo() << "[FairinoZmqRobotAgent]"
                    << "jog watchdog stopped"
                    << "robotId =" << request.robotId;
        }

        if (masterState.active) {
            qInfo() << "[FairinoZmqRobotAgent]"
                    << "command master released by stopJointJog"
                    << "owner =" << masterState.ownerId;

            masterState.active = false;
            masterState.ownerId.clear();
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
            state.workspaceJog = false;
            state.joint = joint;
            state.axis.clear();
            state.sessionId = defaultJogSessionId(request);
            state.lastHeartbeat = SteadyClock::now();

            qInfo() << "[FairinoZmqRobotAgent]"
                    << "jog watchdog started"
                    << "robotId =" << request.robotId
                    << "joint =" << joint
                    << "sessionId =" << state.sessionId;
        }

    } else if (isStopWorkspaceJogCommand(request.command)) {
        result = robot->stopWorkspaceJogEx();

        if (request.robotId >= 1 && request.robotId <= kMaxRobotCount) {
            JogWatchdogState& state = jogStates[request.robotId];

            state.active = false;
            state.workspaceJog = false;
            state.joint = 0;
            state.axis.clear();
            state.sessionId.clear();

            qInfo() << "[FairinoZmqRobotAgent]"
                    << "workspace jog watchdog stopped"
                    << "robotId =" << request.robotId;
        }

        if (masterState.active) {
            qInfo() << "[FairinoZmqRobotAgent]"
                    << "command master released by workspace jog stop"
                    << "owner =" << masterState.ownerId;

            masterState.active = false;
            masterState.ownerId.clear();
        }

    } else if (isStartWorkspaceJogCommand(request.command)) {
        const QString axis =
            request.params.value("axis").toString().trimmed().toUpper();

        const QString direction =
            request.params.value("direction").toString().trimmed();

        const QString frame =
            request.params.value("frame").toString().trimmed().toLower();

        if (direction != QStringLiteral("+") &&
            direction != QStringLiteral("-")) {
            result.ok = false;
            result.code = -10015;
            result.name = request.command.toStdString();
            result.message =
                "Invalid workspace jog direction: " + direction.toStdString();
        } else if (!frame.isEmpty() &&
                   frame != QStringLiteral("workspace") &&
                   frame != QStringLiteral("workpiece")) {
            result.ok = false;
            result.code = -10016;
            result.name = request.command.toStdString();
            result.message =
                "Invalid workspace jog frame: " + frame.toStdString();
        } else {
            const bool positive = direction == QStringLiteral("+");

            const double speed =
                request.params.value("speed").toDouble(5.0);

            // 초기 실기 테스트 안전값
            constexpr float kDefaultAccPercent = 5.0f;
            constexpr float kMaxWorkspaceJogDistanceOrDegree = 1.0f;

            result = robot->startWorkspaceJogEx(axis.toStdString(),
                                                positive,
                                                static_cast<float>(speed),
                                                kDefaultAccPercent,
                                                kMaxWorkspaceJogDistanceOrDegree);
        }

        if (result.ok && request.robotId >= 1 && request.robotId <= kMaxRobotCount) {
            JogWatchdogState& state = jogStates[request.robotId];

            state.active = true;
            state.workspaceJog = true;
            state.joint = 0;
            state.axis = axis;
            state.sessionId = defaultJogSessionId(request);
            state.lastHeartbeat = SteadyClock::now();

            qInfo() << "[FairinoZmqRobotAgent]"
                    << "workspace jog watchdog started"
                    << "robotId =" << request.robotId
                    << "axis =" << axis
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
        CommandMasterState commandMaster;

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
                checkJogTimeouts(jogStates, robot1, robot2, commandMaster);
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
                    executeCommand(request, robot1, robot2, jogStates, commandMaster);
            }

            repSocket.send(
                zmq::buffer(responsePayload.constData(),
                            static_cast<size_t>(responsePayload.size())),
                zmq::send_flags::none);

            qInfo() << "[FairinoZmqRobotAgent] command response ="
                    << responsePayload;

            checkJogTimeouts(jogStates, robot1, robot2, commandMaster);
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
