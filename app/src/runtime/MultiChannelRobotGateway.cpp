#include "MultiChannelRobotGateway.h"

#include "RemoteMessageTypes.h"
#include "RemoteSnapshotParser.h"

#include <QDateTime>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <initializer_list>

MultiChannelRobotGateway::MultiChannelRobotGateway(QObject* parent)
    : IRobotGateway(parent)
{
    connect(&m_pollTimer, &QTimer::timeout,
            this, &MultiChannelRobotGateway::pollSnapshots);
}

bool MultiChannelRobotGateway::start()
{
    if (m_pollTimer.isActive())
        return true;

    pollSnapshots();

    m_pollTimer.start(1000);
    qDebug() << "[Gateway] MultiChannelRobotGateway started mode =" << sourceModeName();

    return true;
}

void MultiChannelRobotGateway::stop()
{
    m_pollTimer.stop();
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

QString MultiChannelRobotGateway::sourceModeName() const
{
    switch (m_sourceMode) {
    case GatewaySourceMode::Dummy:
        return QStringLiteral("dummy");

    case GatewaySourceMode::Remote:
        return QStringLiteral("remote");
    }

    return QStringLiteral("unknown");
}

bool MultiChannelRobotGateway::setSourceModeName(const QString& modeName)
{
    const QString normalized = modeName.trimmed().toLower();

    GatewaySourceMode nextMode;

    if (normalized == QStringLiteral("dummy")) {
        nextMode = GatewaySourceMode::Dummy;
    } else if (normalized == QStringLiteral("remote")) {
        nextMode = GatewaySourceMode::Remote;
    } else {
        qWarning() << "[Gateway] invalid source mode:" << modeName;
        return false;
    }

    if (m_sourceMode == nextMode)
        return true;

    const bool wasActive = m_pollTimer.isActive();

    if (wasActive)
        m_pollTimer.stop();

    m_sourceMode = nextMode;

    qDebug() << "[Gateway] source mode changed =" << sourceModeName();

    if (wasActive) {
        pollSnapshots();
        m_pollTimer.start(1000);
    }

    return true;
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
    // Dry-run path:
    // 실제 ZeroMQ 수신 전, Remote Snapshot JSON -> Parser -> snapshotUpdated 흐름 검증용.
    // 기본 sourceMode는 Dummy이므로 일반 실행에는 영향 없음.

    auto makeArray = [](std::initializer_list<double> values) {
        QJsonArray array;
        for (double value : values) {
            array.append(value);
        }
        return array;
    };

    ++m_sequence;

    for (int robotId = 1; robotId <= 2; ++robotId) {
        QJsonObject object;

        object[RemoteMessage::Field::MessageType] =
            RemoteMessage::Type::Snapshot;

        object[RemoteMessage::Field::SchemaVersion] = 1;

        object[RemoteMessage::Field::RobotId] = robotId;
        object[RemoteMessage::Field::RobotName] =
            QStringLiteral("Robot %1").arg(robotId);
        object[RemoteMessage::Field::Model] =
            robotId == 1 ? QStringLiteral("FR10") : QStringLiteral("FR5");

        object[RemoteMessage::Field::Connected] = true;
        object[RemoteMessage::Field::Running] = true;

        object[RemoteMessage::Field::JointPositions] =
            makeArray({ 0.0, 10.0, 20.0, 30.0, 40.0, 50.0 });

        object[RemoteMessage::Field::TcpPose] =
            makeArray({ 100.0, 200.0, 300.0, 180.0, 0.0, 90.0 });

        object[RemoteMessage::Field::Torques] =
            makeArray({ 10.1, 9.8, 14.5, 11.2, 8.4, 10.9 });

        object[RemoteMessage::Field::DriverTemperatures] =
            makeArray({ 40.1, 41.2, 42.3, 43.4, 44.5, 45.6 });

        object[RemoteMessage::Field::RobotState] = QStringLiteral("RUN");
        object[RemoteMessage::Field::ErrorText] = QStringLiteral("0");
        object[RemoteMessage::Field::SequenceState] = QStringLiteral("IDLE");
        object[RemoteMessage::Field::InterlockState] = QStringLiteral("OK");

        object[RemoteMessage::Field::Timestamp] =
            QDateTime::currentDateTime().toString(Qt::ISODateWithMs);

        object[RemoteMessage::Field::SequenceNumber] =
            static_cast<double>(m_sequence);

        const QByteArray payload =
            QJsonDocument(object).toJson(QJsonDocument::Compact);

        const auto parsed = RemoteSnapshotParser::parseJson(payload);

        if (!parsed.ok) {
            emit errorOccurred(robotId, parsed.error);
            qWarning() << "[Gateway] remote snapshot dry-run parse failed:"
                       << parsed.error;
            continue;
        }

        emit snapshotUpdated(parsed.robotId, parsed.snapshot);
    }
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

