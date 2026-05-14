#pragma once

#include "IRobotGateway.h"
#include "RobotRuntimeTypes.h"

#include <QTimer>

class MultiChannelRobotGateway : public IRobotGateway
{
    Q_OBJECT

public:
    explicit MultiChannelRobotGateway(QObject* parent = nullptr);

    bool start() override;
    void stop() override;

    bool isConnected(int robotId) const override;

    void setManualMode(int robotId) override;
    void setAutoMode(int robotId) override;
    void clearError(int robotId) override;

    void startJointJog(int robotId, int joint, bool positive) override;
    void stopJointJog(int robotId) override;

private slots:
    void publishDummySnapshot();

private:
    QTimer m_dummyTimer;
    quint64 m_sequence = 0;
};