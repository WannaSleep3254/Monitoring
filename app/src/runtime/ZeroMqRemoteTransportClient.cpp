#include "ZeroMqRemoteTransportClient.h"

#include <QDebug>

#ifdef ENABLE_ZEROMQ_TRANSPORT
#include <zmq.hpp>
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
        m_snapshotSocket->connect(snapshotEndpoint);

        m_commandSocket->set(zmq::sockopt::linger, 0);
        m_commandSocket->set(zmq::sockopt::rcvtimeo, m_config.commandTimeoutMs);
        m_commandSocket->set(zmq::sockopt::sndtimeo, m_config.commandTimeoutMs);
        m_commandSocket->connect(commandEndpoint);

        m_running = true;
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
    if (!m_running) {
        emit errorOccurred(QStringLiteral("ZeroMQ transport is not running"));
        return;
    }

    // TODO:
    // - REQ socket으로 requestPayload 전송
    // - response 수신
    // - commandResponsePayloadReceived(responsePayload) emit

    qDebug() << "[ZeroMqTransport] command request placeholder =" << requestPayload;
#endif
}

void ZeroMqRemoteTransportClient::cleanupTransport()
{
#ifdef ENABLE_ZEROMQ_TRANSPORT
    resetSockets();
#endif

    m_running = false;
}

#ifdef ENABLE_ZEROMQ_TRANSPORT
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
