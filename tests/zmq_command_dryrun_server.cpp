// tests/zmq_command_dryrun_server.cpp

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

#ifdef ENABLE_ZEROMQ_TRANSPORT
#include <zmq.hpp>
#endif

namespace
{
QByteArray buildResponse(const QByteArray& requestPayload)
{
    QJsonParseError parseError;
    const QJsonDocument requestDoc =
        QJsonDocument::fromJson(requestPayload, &parseError);

    QJsonObject response;
    response["messageType"] = QStringLiteral("commandResponse");
    response["schemaVersion"] = 1;
    response["timestamp"] =
        QDateTime::currentDateTime().toString(Qt::ISODateWithMs);

    if (parseError.error != QJsonParseError::NoError || !requestDoc.isObject()) {
        response["ok"] = false;
        response["code"] = -1;
        response["robotId"] = 0;
        response["command"] = QString();
        response["commandId"] = QString();
        response["message"] =
            QStringLiteral("invalid request json: %1")
                .arg(parseError.errorString());

        return QJsonDocument(response).toJson(QJsonDocument::Compact);
    }

    const QJsonObject request = requestDoc.object();

    const int robotId = request.value("robotId").toInt(0);
    const QString commandId = request.value("commandId").toString();
    const QString command = request.value("command").toString();

    response["ok"] = true;
    response["code"] = 0;
    response["robotId"] = robotId;
    response["commandId"] = commandId;
    response["command"] = command;
    response["message"] =
        QStringLiteral("zmq dry-run command ok: %1").arg(command);

    return QJsonDocument(response).toJson(QJsonDocument::Compact);
}
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

#ifndef ENABLE_ZEROMQ_TRANSPORT
    qCritical() << "[ZmqCommandDryRunServer]"
                << "ZeroMQ transport is disabled at build time";
    return 1;
#else
    const QString endpoint =
        app.arguments().size() >= 2
            ? app.arguments().at(1)
            : QStringLiteral("tcp://*:5557");

    try {
        zmq::context_t context(1);
        zmq::socket_t socket(context, zmq::socket_type::rep);

        socket.set(zmq::sockopt::linger, 0);
        socket.bind(endpoint.toStdString());

        qInfo() << "[ZmqCommandDryRunServer] listening =" << endpoint;

        while (true) {
            zmq::message_t requestMessage;

            const auto recvResult =
                socket.recv(requestMessage, zmq::recv_flags::none);

            if (!recvResult.has_value()) {
                qWarning() << "[ZmqCommandDryRunServer] recv failed";
                continue;
            }

            const QByteArray requestPayload(
                static_cast<const char*>(requestMessage.data()),
                static_cast<int>(requestMessage.size()));

            qInfo() << "[ZmqCommandDryRunServer] request ="
                    << requestPayload;

            const QByteArray responsePayload =
                buildResponse(requestPayload);

            socket.send(
                zmq::buffer(responsePayload.constData(),
                            static_cast<size_t>(responsePayload.size())),
                zmq::send_flags::none);

            qInfo() << "[ZmqCommandDryRunServer] response ="
                    << responsePayload;
        }

    } catch (const zmq::error_t& error) {
        qCritical() << "[ZmqCommandDryRunServer] ZeroMQ error:"
                    << error.what();
        return 2;
    }

    return 0;
#endif
}