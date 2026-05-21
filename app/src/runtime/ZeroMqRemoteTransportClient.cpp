#include "ZeroMqRemoteTransportClient.h"

#include <QDebug>

#ifdef ENABLE_ZEROMQ_TRANSPORT
#include <zmq.hpp>
#endif

ZeroMqRemoteTransportClient::ZeroMqRemoteTransportClient(QObject* parent)
    : IRemoteTransportClient(parent)
{
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
    // TODO:
    // - ZeroMQ context 생성
    // - SUB socket 생성
    // - snapshotEndpoint connect
    // - snapshotTopic subscribe
    // - REQ socket 생성
    // - commandEndpoint connect

    m_running = true;
    emit connectionStateChanged(true);

    qDebug() << "[ZeroMqTransport] started"
             << "snapshotEndpoint =" << m_config.snapshotEndpoint
             << "commandEndpoint =" << m_config.commandEndpoint
             << "topic =" << m_config.snapshotTopic;

    return true;
#endif
}

void ZeroMqRemoteTransportClient::stop()
{
    if (!m_running)
        return;

#ifdef ENABLE_ZEROMQ_TRANSPORT
    // TODO:
    // - SUB socket close
    // - REQ socket close
    // - worker thread stop
#endif

    m_running = false;
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
