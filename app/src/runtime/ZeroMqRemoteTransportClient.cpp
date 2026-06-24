#include "ZeroMqRemoteTransportClient.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

#ifdef ENABLE_ZEROMQ_TRANSPORT
#include <zmq.hpp>

#include <optional>
#include <chrono>
#endif

ZeroMqRemoteTransportClient::ZeroMqRemoteTransportClient(QObject* parent)
    : IRemoteTransportClient(parent)
{
}

ZeroMqRemoteTransportClient::~ZeroMqRemoteTransportClient()
{
    cleanupTransport();
}

void ZeroMqRemoteTransportClient::configure(const RemoteTransportConfig& config)
{
    m_config = config;
}

bool ZeroMqRemoteTransportClient::start()
{
    if (m_running)
        return true;

#ifndef ENABLE_ZEROMQ_TRANSPORT
    const QString message =
        QStringLiteral("ZeroMQ transport is disabled at build time");

    qWarning() << "[ZeroMqTransport]" << message;
    emit errorOccurred(message);
    emit connectionStateChanged(false);

    return false;
#else
    try {
        m_context = std::make_unique<zmq::context_t>(1);

        m_snapshotSocket =
            std::make_unique<zmq::socket_t>(*m_context, zmq::socket_type::sub);

        m_commandSocket =
            std::make_unique<zmq::socket_t>(*m_context, zmq::socket_type::req);

        const std::string snapshotEndpoint =
            m_config.snapshotEndpoint.toStdString();

        const std::string commandEndpoint =
            m_config.commandEndpoint.toStdString();

        const std::string snapshotTopic =
            m_config.snapshotTopic.toStdString();

        m_snapshotSocket->set(zmq::sockopt::subscribe, snapshotTopic);
        m_snapshotSocket->set(zmq::sockopt::rcvtimeo, 100);
        m_snapshotSocket->connect(snapshotEndpoint);

        m_commandSocket->set(zmq::sockopt::linger, 0);
        m_commandSocket->set(zmq::sockopt::rcvtimeo, m_config.commandTimeoutMs);
        m_commandSocket->set(zmq::sockopt::sndtimeo, m_config.commandTimeoutMs);
        m_commandSocket->connect(commandEndpoint);

        m_running = true;
        startSnapshotWorker();

        emit connectionStateChanged(true);

        qDebug() << "[ZeroMqTransport] started"
                 << "snapshotEndpoint =" << m_config.snapshotEndpoint
                 << "commandEndpoint =" << m_config.commandEndpoint
                 << "topic =" << m_config.snapshotTopic;

        return true;
    } catch (const zmq::error_t& error) {
        const QString message =
            QStringLiteral("ZeroMQ start failed: %1")
                .arg(QString::fromLocal8Bit(error.what()));

        qWarning() << "[ZeroMqTransport]" << message;

        resetSockets();

        emit errorOccurred(message);
        emit connectionStateChanged(false);

        return false;
    }
#endif
}

void ZeroMqRemoteTransportClient::stop()
{
    if (!m_running)
        return;

    cleanupTransport();

    emit connectionStateChanged(false);

    qDebug() << "[ZeroMqTransport] stopped";
}

bool ZeroMqRemoteTransportClient::isRunning() const
{
    return m_running;
}

void ZeroMqRemoteTransportClient::sendCommand(const QByteArray& requestPayload)
{
#ifndef ENABLE_ZEROMQ_TRANSPORT
    Q_UNUSED(requestPayload)

    emit errorOccurred(
        QStringLiteral("ZeroMQ transport is disabled at build time"));

    return;
#else
    if (!m_running || !m_commandSocket) {
        emit errorOccurred(QStringLiteral("ZeroMQ transport is not running"));
        return;
    }

    try {
        zmq::message_t requestMessage(
            requestPayload.constData(),
            static_cast<size_t>(requestPayload.size()));

        const auto sendResult =
            m_commandSocket->send(requestMessage, zmq::send_flags::none);

        if (!sendResult.has_value()) {
            const QString message =
                QStringLiteral("ZeroMQ command send timed out");

            qWarning() << "[ZeroMqTransport]" << message;
            emit errorOccurred(message);
            return;
        }

        zmq::message_t responseMessage;
        const auto recvResult =
            m_commandSocket->recv(responseMessage, zmq::recv_flags::none);

        if (!recvResult.has_value()) {
            const QString message =
                QStringLiteral("ZeroMQ command response timed out");

            qWarning() << "[ZeroMqTransport]" << message;
            emit errorOccurred(message);
            return;
        }

        const QByteArray responsePayload(
            static_cast<const char*>(responseMessage.data()),
            static_cast<int>(responseMessage.size()));

        emit commandResponsePayloadReceived(responsePayload);

        const QJsonDocument responseDoc = QJsonDocument::fromJson(responsePayload);
        const QString responseCommand = responseDoc.isObject()
            ? responseDoc.object().value(QStringLiteral("command")).toString()
            : QString();
        // gantryReadPosition is high-frequency UI polling; keep the response path
        // active but suppress the repetitive transport debug line.
        if (responseCommand != QStringLiteral("gantryReadPosition")) {
            qDebug() << "[ZeroMqTransport] command response received bytes ="
                     << responsePayload.size();
        }

    } catch (const zmq::error_t& error) {
        const QString message =
            QStringLiteral("ZeroMQ command failed: %1")
                .arg(QString::fromLocal8Bit(error.what()));

        qWarning() << "[ZeroMqTransport]" << message;
        emit errorOccurred(message);
    }
#endif
}

void ZeroMqRemoteTransportClient::cleanupTransport()
{
#ifdef ENABLE_ZEROMQ_TRANSPORT
    stopSnapshotWorker();
    resetSockets();
#endif

    m_running = false;
}

#ifdef ENABLE_ZEROMQ_TRANSPORT
void ZeroMqRemoteTransportClient::startSnapshotWorker()
{
    if (m_snapshotThreadRunning)
        return;

    m_snapshotThreadRunning = true;

    m_snapshotThread =
        std::thread([this]() {
            snapshotReceiveLoop();
        });
}

void ZeroMqRemoteTransportClient::stopSnapshotWorker()
{
    if (!m_snapshotThreadRunning)
        return;

    m_snapshotThreadRunning = false;

    if (m_snapshotThread.joinable()) {
        m_snapshotThread.join();
    }
}

void ZeroMqRemoteTransportClient::snapshotReceiveLoop()
{
    qDebug() << "[ZeroMqTransport] snapshot worker started";

    while (m_snapshotThreadRunning) {
        try {
            if (!m_snapshotSocket) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            zmq::message_t topicMessage;
            const auto topicResult =
                m_snapshotSocket->recv(topicMessage, zmq::recv_flags::none);

            if (!topicResult.has_value()) {
                continue; // timeout
            }

            zmq::message_t payloadMessage;
            const auto payloadResult =
                m_snapshotSocket->recv(payloadMessage, zmq::recv_flags::none);

            if (!payloadResult.has_value()) {
                emit errorOccurred(
                    QStringLiteral("ZeroMQ snapshot payload frame missing"));
                continue;
            }

            const QByteArray topicPayload(
                static_cast<const char*>(topicMessage.data()),
                static_cast<int>(topicMessage.size()));

            const QByteArray snapshotPayload(
                static_cast<const char*>(payloadMessage.data()),
                static_cast<int>(payloadMessage.size()));

            Q_UNUSED(topicPayload)

            emit snapshotPayloadReceived(snapshotPayload);

        } catch (const zmq::error_t& error) {
            if (!m_snapshotThreadRunning)
                break;

            const QString message =
                QStringLiteral("ZeroMQ snapshot receive failed: %1")
                    .arg(QString::fromLocal8Bit(error.what()));

            qWarning() << "[ZeroMqTransport]" << message;
            emit errorOccurred(message);

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    qDebug() << "[ZeroMqTransport] snapshot worker stopped";
}

void ZeroMqRemoteTransportClient::resetSockets()
{
    try {
        if (m_snapshotSocket) {
            m_snapshotSocket->close();
        }

        if (m_commandSocket) {
            m_commandSocket->close();
        }

        if (m_context) {
            m_context->shutdown();
            m_context->close();
        }
    } catch (const zmq::error_t& error) {
        qWarning() << "[ZeroMqTransport] reset failed:"
                   << QString::fromLocal8Bit(error.what());
    }

    m_snapshotSocket.reset();
    m_commandSocket.reset();
    m_context.reset();
}
#endif
