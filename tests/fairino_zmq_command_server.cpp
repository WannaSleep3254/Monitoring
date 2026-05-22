// tests/fairino_zmq_command_server.cpp

#include "monitoring/FairinoMonitorService.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

#ifdef ENABLE_ZEROMQ_TRANSPORT
#include <zmq.hpp>
#endif

#include <array>
#include <atomic>
#include <csignal>
#include <memory>
#include <string>

namespace
{
std::atomic_bool g_running { true };

void handleSignal(int)
{
    g_running = false;
}

struct CommandRequest
{
    bool parsed = false;
    QString error;

    QString commandId;
    int robotId = 0;
    QString command;
    QJsonObject params;
};

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

QByteArray buildParseErrorResponse(const QByteArray& payload,
                                   const QString& error)
{
    Q_UNUSED(payload)

    QJsonObject response;
    response["messageType"] = QStringLiteral("commandResponse");
    response["schemaVersion"] = 1;
    response["timestamp"] =
        QDateTime::currentDateTime().toString(Qt::ISODateWithMs);

    response["commandId"] = QStringLiteral("invalid");
    response["robotId"] = 0;
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

QByteArray executeCommand(const CommandRequest& request,
                          monitoring::FairinoMonitorService& robot1,
                          monitoring::FairinoMonitorService& robot2)
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

    } else if (request.command == QStringLiteral("jogHeartbeat")) {
        // 현재는 heartbeat를 별도 상태 갱신 없이 OK 처리.
        // 추후 jog session timeout 감시에 사용.
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
    qCritical() << "[FairinoZmqCommandServer]"
                << "ZeroMQ transport is disabled at build time";
    return 1;
#else
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    const QString endpoint =
        app.arguments().size() >= 2
            ? app.arguments().at(1)
            : QStringLiteral("tcp://*:5557");

    const QString robot1Ip =
        app.arguments().size() >= 3
            ? app.arguments().at(2)
            : QStringLiteral("192.168.57.121");

    const QString robot2Ip =
        app.arguments().size() >= 4
            ? app.arguments().at(3)
            : QStringLiteral("192.168.57.122");

    qInfo() << "[FairinoZmqCommandServer] endpoint =" << endpoint;
    qInfo() << "[FairinoZmqCommandServer] robot1Ip =" << robot1Ip;
    qInfo() << "[FairinoZmqCommandServer] robot2Ip =" << robot2Ip;

    try {
        monitoring::FairinoMonitorService robot1;
        monitoring::FairinoMonitorService robot2;

        monitoring::FairinoMonitorService::Options opt1;
        opt1.robot_id = 1;
        opt1.poll_period_ms = 200;
        opt1.enable_reconnect = true;

        monitoring::FairinoMonitorService::Options opt2;
        opt2.robot_id = 2;
        opt2.poll_period_ms = 200;
        opt2.enable_reconnect = true;

        const bool robot1Started =
            robot1.start(robot1Ip.toStdString(), opt1);

        const bool robot2Started =
            robot2.start(robot2Ip.toStdString(), opt2);

        qInfo() << "[FairinoZmqCommandServer] robot1Started =" << robot1Started;
        qInfo() << "[FairinoZmqCommandServer] robot2Started =" << robot2Started;

        if (!robot1Started && !robot2Started) {
            qCritical() << "[FairinoZmqCommandServer]"
                        << "all robot connections failed";
            return 2;
        }

        zmq::context_t context(1);
        zmq::socket_t socket(context, zmq::socket_type::rep);

        socket.set(zmq::sockopt::linger, 0);
        socket.set(zmq::sockopt::rcvtimeo, 100);
        socket.bind(endpoint.toStdString());

        qInfo() << "[FairinoZmqCommandServer] listening =" << endpoint;

        while (g_running) {
            zmq::message_t requestMessage;

            const auto recvResult =
                socket.recv(requestMessage, zmq::recv_flags::none);

            if (!recvResult.has_value()) {
                continue;
            }

            const QByteArray requestPayload(
                static_cast<const char*>(requestMessage.data()),
                static_cast<int>(requestMessage.size()));

            qInfo() << "[FairinoZmqCommandServer] request ="
                    << requestPayload;

            const CommandRequest request =
                parseCommandRequest(requestPayload);

            QByteArray responsePayload;

            if (!request.parsed) {
                responsePayload =
                    buildParseErrorResponse(requestPayload, request.error);
            } else {
                responsePayload =
                    executeCommand(request, robot1, robot2);
            }

            socket.send(
                zmq::buffer(responsePayload.constData(),
                            static_cast<size_t>(responsePayload.size())),
                zmq::send_flags::none);

            qInfo() << "[FairinoZmqCommandServer] response ="
                    << responsePayload;
        }

        qInfo() << "[FairinoZmqCommandServer] stopping...";

        robot1.stop();
        robot2.stop();

        socket.close();
        context.shutdown();
        context.close();

        qInfo() << "[FairinoZmqCommandServer] stopped";
        return 0;

    } catch (const zmq::error_t& error) {
        qCritical() << "[FairinoZmqCommandServer] ZeroMQ error:"
                    << error.what();
        return 3;
    }
#endif
}