#include "MultiChannelRobotGateway.h"

#include <QDateTime>
#include <QDebug>

MultiChannelRobotGateway::MultiChannelRobotGateway(QObject* parent)
    : IRobotGateway(parent)
{
    connect(&m_dummyTimer, &QTimer::timeout,
            this, &MultiChannelRobotGateway::publishDummySnapshot);
}

bool MultiChannelRobotGateway::start()
{
    if (m_dummyTimer.isActive())
        return true;

    publishDummySnapshot();

    m_dummyTimer.start(1000);
    qDebug() << "[Gateway] MultiChannelRobotGateway started";
    return true;
}

void MultiChannelRobotGateway::stop()
{
    m_dummyTimer.stop();
}

bool MultiChannelRobotGateway::isConnected(int robotId) const
{
    Q_UNUSED(robotId)
    return true;
}

void MultiChannelRobotGateway::setManualMode(int robotId)
{
    emit commandFinished(robotId, "setManualMode", true, 0, "dummy manual mode");
}

void MultiChannelRobotGateway::setAutoMode(int robotId)
{
    emit commandFinished(robotId, "setAutoMode", true, 0, "dummy auto mode");
}

void MultiChannelRobotGateway::clearError(int robotId)
{
    emit commandFinished(robotId, "clearError", true, 0, "dummy clear error");
}

void MultiChannelRobotGateway::startJointJog(int robotId, int joint, bool positive)
{
    emit commandFinished(robotId,
                         QString("startJointJog J%1 %2")
                             .arg(joint)
                             .arg(positive ? "+" : "-"),
                         true,
                         0,
                         "dummy jog start");
}

void MultiChannelRobotGateway::stopJointJog(int robotId)
{
    emit commandFinished(robotId, "stopJointJog", true, 0, "dummy jog stop");
}

void MultiChannelRobotGateway::publishDummySnapshot()
{
    ++m_sequence;

    for (int robotId = 1; robotId <= 2; ++robotId) {
        UnifiedRobotSnapshot s;
        s.robotId = robotId;
        s.connected = true;
        s.running = true;
        s.jointPositions = { 0.0, 10.0, 20.0, 30.0, 40.0, 50.0 };
        s.tcpPose = { 100.0, 200.0, 300.0, 180.0, 0.0, 90.0 };
        s.torques = { 10.1, 9.8, 14.5, 11.2, 8.4, 10.9 };
        s.driverTemperatures = { 40.1, 41.2, 42.3, 43.4, 44.5, 45.6 };
        s.robotState = "RUN";
        s.errorText = "0";
        s.sequenceState = "IDLE";
        s.interlockState = "OK";
        s.timestamp = QDateTime::currentDateTime();
        s.sequenceNumber = m_sequence;

        emit snapshotUpdated(robotId, s.toMap());
    }
}
