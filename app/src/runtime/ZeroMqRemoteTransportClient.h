#pragma once

#include "IRemoteTransportClient.h"

#include <QByteArray>

class ZeroMqRemoteTransportClient : public IRemoteTransportClient
{
    Q_OBJECT

public:
    explicit ZeroMqRemoteTransportClient(QObject* parent = nullptr);

    void configure(const RemoteTransportConfig& config) override;

    bool start() override;
    void stop() override;
    bool isRunning() const override;

    void sendCommand(const QByteArray& requestPayload) override;

private:
    RemoteTransportConfig m_config;
    bool m_running = false;
};
