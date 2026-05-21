#pragma once

#include "IRemoteTransportClient.h"

#include <QByteArray>

#include <memory>

#ifdef ENABLE_ZEROMQ_TRANSPORT
namespace zmq
{
class context_t;
class socket_t;
}
#endif

class ZeroMqRemoteTransportClient : public IRemoteTransportClient
{
    Q_OBJECT

public:
    explicit ZeroMqRemoteTransportClient(QObject* parent = nullptr);
    ~ZeroMqRemoteTransportClient() override;

    void configure(const RemoteTransportConfig& config) override;

    bool start() override;
    void stop() override;
    bool isRunning() const override;

    void sendCommand(const QByteArray& requestPayload) override;

private:
    void cleanupTransport();

#ifdef ENABLE_ZEROMQ_TRANSPORT
    void resetSockets();

    std::unique_ptr<zmq::context_t> m_context;
    std::unique_ptr<zmq::socket_t> m_snapshotSocket;
    std::unique_ptr<zmq::socket_t> m_commandSocket;
#endif

    RemoteTransportConfig m_config;
    bool m_running = false;
};
