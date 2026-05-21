#pragma once

#include "IRemoteTransportClient.h"

#include <QByteArray>
#include <QTimer>

class DryRunRemoteTransportClient : public IRemoteTransportClient
{
    Q_OBJECT

public:
    explicit DryRunRemoteTransportClient(QObject* parent = nullptr);

    void configure(const RemoteTransportConfig& config) override;

    bool start() override;
    void stop() override;
    bool isRunning() const override;

    void sendCommand(const QByteArray& requestPayload) override;

private slots:
    void publishDryRunSnapshots();

private:
    QByteArray buildSnapshotPayload(int robotId);
    QByteArray buildCommandResponsePayload(const QByteArray& requestPayload,
                                           QString* errorMessage = nullptr) const;

    RemoteTransportConfig m_config;
    QTimer m_snapshotTimer;
    bool m_running = false;
    quint64 m_sequence = 0;
};
