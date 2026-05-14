#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>

class IRobotGateway : public QObject
{
    Q_OBJECT

public:
    explicit IRobotGateway(QObject* parent = nullptr)
        : QObject(parent)
    {}

    virtual ~IRobotGateway() override = default;

    virtual bool start() = 0;
    virtual void stop() = 0;

    virtual bool isConnected(int robotId) const = 0;

    virtual void setManualMode(int robotId) = 0;
    virtual void setAutoMode(int robotId) = 0;
    virtual void clearError(int robotId) = 0;

    virtual void startJointJog(int robotId, int joint, bool positive) = 0;
    virtual void stopJointJog(int robotId) = 0;

signals:
    void snapshotUpdated(int robotId, QVariantMap snapshot);
    void commandFinished(int robotId, QString command, bool ok, int code, QString message);
    void connectionStateChanged(int robotId, bool connected);
    void errorOccurred(int robotId, QString message);
};
