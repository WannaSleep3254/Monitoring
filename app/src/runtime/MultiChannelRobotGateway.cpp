#include "MultiChannelRobotGateway.h"

#include <QDateTime>
#include <QDebug>

MultiChannelRobotGateway::MultiChannelRobotGateway(QObject* parent)
    : IRobotGateway(parent)
{
#if false
    connect(&m_dummyTimer, &QTimer::timeout,
            this, &MultiChannelRobotGateway::publishDummySnapshot);
#else
    connect(&m_pollTimer, &QTimer::timeout,
            this, &MultiChannelRobotGateway::pollSnapshots);
#endif
}

bool MultiChannelRobotGateway::start()
{
#if false
    if (m_dummyTimer.isActive())
        return true;

    publishDummySnapshot();

    m_dummyTimer.start(1000);
    qDebug() << "[Gateway] MultiChannelRobotGateway started";
#else
    if (m_pollTimer.isActive())
        return true;

    pollSnapshots();

    m_pollTimer.start(1000);
    qDebug() << "[Gateway] MultiChannelRobotGateway started";
#endif
    return true;
}

void MultiChannelRobotGateway::stop()
{
#if false
    m_dummyTimer.stop();
#else
    m_pollTimer.stop();
#endif
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
    // TODO:
    // Remote 모드에서는 GUI PC -> Robot PC command channel로 startJointJog 명령 전송.
    // Jog 명령은 button press/release 구조와 heartbeat timeout 안전정지가 필요함.

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

void MultiChannelRobotGateway::pollSnapshots()
{
    switch (m_sourceMode) {
    case GatewaySourceMode::Dummy:
        publishDummySnapshot();
        break;

    case GatewaySourceMode::Remote:
        pollRemoteSnapshots();
        break;
    }
}

void MultiChannelRobotGateway::pollRemoteSnapshots()
{
    // TODO:
    // Robot PC -> GUI PC 원격 모니터링 데이터 수신 구조로 확장 예정.
    //
    // 권장 구조:
    // - Monitoring channel: Robot PC -> GUI PC, ZeroMQ PUB/SUB
    // - Command channel: GUI PC -> Robot PC, ZeroMQ REQ/REP 또는 DEALER/ROUTER
    //
    // 수신된 payload는 UnifiedRobotSnapshot 호환 QVariantMap으로 변환 후
    // emit snapshotUpdated(robotId, snapshot) 호출.
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

