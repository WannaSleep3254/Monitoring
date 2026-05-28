#pragma once

#include <QObject>
#include <QByteArray>
#include <QString>

struct RemoteTransportConfig
{
    QString snapshotEndpoint;
    QString commandEndpoint;
    QString snapshotTopic;

    int commandTimeoutMs = 1000;
};

class IRemoteTransportClient : public QObject
{
    Q_OBJECT

public:
    explicit IRemoteTransportClient(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

    ~IRemoteTransportClient() override = default;

    virtual void configure(const RemoteTransportConfig& config) = 0;

    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual bool isRunning() const = 0;

    virtual void sendCommand(const QByteArray& requestPayload) = 0;

signals:
    void snapshotPayloadReceived(const QByteArray& payload);
    void commandResponsePayloadReceived(const QByteArray& payload);

    void connectionStateChanged(bool connected);
    void errorOccurred(const QString& message);
};
