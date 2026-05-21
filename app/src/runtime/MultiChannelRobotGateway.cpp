#include "MultiChannelRobotGateway.h"
// 원격 제어 transport 구현체
#include "DryRunRemoteTransportClient.h"
#include "ZeroMqRemoteTransportClient.h"
// 원격 메시지 생성/파싱 유틸리티
#include "RemoteSnapshotParser.h"
#include "RemoteCommandBuilder.h"
#include "RemoteCommandResponseParser.h"
#include "RobotRuntimeTypes.h"

#include <QDateTime>
#include <QDebug>
#include <QSettings>

MultiChannelRobotGateway::MultiChannelRobotGateway(QObject* parent)
    : IRobotGateway(parent)
{
    connect(&m_pollTimer, &QTimer::timeout,
            this, &MultiChannelRobotGateway::pollSnapshots);
}

bool MultiChannelRobotGateway::start()
{
    if (m_sourceMode == GatewaySourceMode::Remote) {
        ensureRemoteTransport();
        configureRemoteTransport();

        if (!m_remoteTransport->start())
            return false;

        qDebug() << "[Gateway] MultiChannelRobotGateway started mode =" << sourceModeName();
        return true;
    }

    if (m_pollTimer.isActive())
        return true;

    publishDummySnapshot();

    m_pollTimer.start(1000);
    qDebug() << "[Gateway] MultiChannelRobotGateway started mode =" << sourceModeName();

    return true;
}

void MultiChannelRobotGateway::stop()
{
    m_pollTimer.stop();

    if (m_remoteTransport) {
        m_remoteTransport->stop();
    }
}

bool MultiChannelRobotGateway::isConnected(int robotId) const
{
    Q_UNUSED(robotId)

    if (m_sourceMode == GatewaySourceMode::Remote)
        return m_remoteConnected;

    return true;
}

void MultiChannelRobotGateway::setManualMode(int robotId)
{
    if (m_sourceMode == GatewaySourceMode::Remote) {
        const QByteArray request =
            RemoteCommandBuilder::buildSetManualMode(robotId);

        sendRemoteCommand(request);
        return;
    }

    emit commandFinished(robotId, "setManualMode", true, 0, "dummy manual mode");
}

void MultiChannelRobotGateway::setAutoMode(int robotId)
{
    if (m_sourceMode == GatewaySourceMode::Remote) {
        const QByteArray request =
            RemoteCommandBuilder::buildSetAutoMode(robotId);

        sendRemoteCommand(request);
        return;
    }

    emit commandFinished(robotId, "setAutoMode", true, 0, "dummy auto mode");
}

void MultiChannelRobotGateway::clearError(int robotId)
{
    if (m_sourceMode == GatewaySourceMode::Remote) {
        const QByteArray request =
            RemoteCommandBuilder::buildClearError(robotId);

        sendRemoteCommand(request);
        return;
    }

    emit commandFinished(robotId, "clearError", true, 0, "dummy clear error");
}

void MultiChannelRobotGateway::startJointJog(int robotId, int joint, bool positive)
{
    if (m_sourceMode == GatewaySourceMode::Remote) {
        constexpr double kDryRunJogSpeed = 10.0;

        const QByteArray request =
            RemoteCommandBuilder::buildStartJointJog(robotId,
                                                     joint,
                                                     positive,
                                                     kDryRunJogSpeed);

        sendRemoteCommand(request);
        return;
    }

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
    if (m_sourceMode == GatewaySourceMode::Remote) {
        const QByteArray request =
            RemoteCommandBuilder::buildStopJointJog(robotId);

        sendRemoteCommand(request);
        return;
    }

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

    const bool dummyWasActive = m_pollTimer.isActive();
    const bool remoteWasActive = m_remoteTransport && m_remoteTransport->isRunning();

    if (dummyWasActive)
        m_pollTimer.stop();

    if (remoteWasActive)
        m_remoteTransport->stop();

    m_sourceMode = nextMode;

    qDebug() << "[Gateway] source mode changed =" << sourceModeName();

    if (dummyWasActive || remoteWasActive) {
        start();
    }

    return true;
}

void MultiChannelRobotGateway::ensureRemoteTransport()
{
    if (m_remoteTransport)
        return;

    QSettings settings;

    const QString transportType =
        settings.value(QStringLiteral("runtime/remoteTransportType"),
                       QStringLiteral("dryrun"))
            .toString()
            .trimmed()
            .toLower();

    IRemoteTransportClient* transport = nullptr;

    if (transportType == QStringLiteral("zeromq")) {
        transport = new ZeroMqRemoteTransportClient(this);
    } else if (transportType == QStringLiteral("dryrun")) {
        transport = new DryRunRemoteTransportClient(this);
    } else {
        qWarning() << "[Gateway] invalid remoteTransportType:"
                   << transportType
                   << "fallback to dryrun";

        transport = new DryRunRemoteTransportClient(this);
    }

    qDebug() << "[Gateway] remote transport type =" << transportType;

    connect(transport,
            &IRemoteTransportClient::snapshotPayloadReceived,
            this,
            &MultiChannelRobotGateway::handleRemoteSnapshotPayload);

    connect(transport,
            &IRemoteTransportClient::commandResponsePayloadReceived,
            this,
            &MultiChannelRobotGateway::handleRemoteCommandResponsePayload);

    connect(transport,
            &IRemoteTransportClient::connectionStateChanged,
            this,
            &MultiChannelRobotGateway::handleRemoteConnectionStateChanged);

    connect(transport,
            &IRemoteTransportClient::errorOccurred,
            this,
            &MultiChannelRobotGateway::handleRemoteTransportError);

    m_remoteTransport = transport;
}

void MultiChannelRobotGateway::configureRemoteTransport()
{
    if (!m_remoteTransport)
        return;

    QSettings settings;

    const QString transportType =
        settings.value(QStringLiteral("runtime/remoteTransportType"),
                       QStringLiteral("dryrun"))
            .toString()
            .trimmed()
            .toLower();

    RemoteTransportConfig config;

    if (transportType == QStringLiteral("zeromq")) {
        config.snapshotEndpoint =
            settings.value(QStringLiteral("runtime/snapshotEndpoint"),
                           QStringLiteral("tcp://127.0.0.1:5556"))
                .toString();

        config.commandEndpoint =
            settings.value(QStringLiteral("runtime/commandEndpoint"),
                           QStringLiteral("tcp://127.0.0.1:5557"))
                .toString();

        config.snapshotTopic =
            settings.value(QStringLiteral("runtime/snapshotTopic"),
                           QStringLiteral("robot/"))
                .toString();

        config.commandTimeoutMs =
            settings.value(QStringLiteral("runtime/commandTimeoutMs"), 1000)
                .toInt();
    } else {
        config.snapshotEndpoint = QStringLiteral("dryrun://snapshot");
        config.commandEndpoint = QStringLiteral("dryrun://command");
        config.snapshotTopic = QStringLiteral("robot/");
        config.commandTimeoutMs = 1000;
    }

    m_remoteTransport->configure(config);

    qDebug() << "[Gateway] remote transport configured"
             << "type =" << transportType
             << "snapshotEndpoint =" << config.snapshotEndpoint
             << "commandEndpoint =" << config.commandEndpoint
             << "topic =" << config.snapshotTopic
             << "timeoutMs =" << config.commandTimeoutMs;
}

void MultiChannelRobotGateway::handleRemoteSnapshotPayload(const QByteArray& payload)
{
    const auto parsed = RemoteSnapshotParser::parseJson(payload);

    if (!parsed.ok) {
        const QString error =
            QStringLiteral("Remote snapshot parse failed: %1")
                .arg(parsed.error);

        qWarning() << "[Gateway]" << error;
        emit errorOccurred(0, error);
        return;
    }

    emit snapshotUpdated(parsed.robotId, parsed.snapshot);
}

void MultiChannelRobotGateway::handleRemoteCommandResponsePayload(const QByteArray& payload)
{
    const auto parsed =
        RemoteCommandResponseParser::parseJson(payload);

    if (!parsed.parsed) {
        const QString error =
            QStringLiteral("Remote command response parse failed: %1")
                .arg(parsed.error);

        qWarning() << "[Gateway]" << error;
        emit errorOccurred(0, error);
        return;
    }

    emit commandFinished(parsed.robotId,
                         parsed.command,
                         parsed.ok,
                         parsed.code,
                         parsed.message);
}

void MultiChannelRobotGateway::handleRemoteTransportError(const QString& message)
{
    qWarning() << "[Gateway] remote transport error:" << message;
    emit errorOccurred(0, message);
}

void MultiChannelRobotGateway::sendRemoteCommand(const QByteArray& requestPayload)
{
    if (m_sourceMode != GatewaySourceMode::Remote) {
        emit errorOccurred(0, QStringLiteral("Remote command requested in non-remote mode"));
        return;
    }

    ensureRemoteTransport();

    if (!m_remoteTransport->isRunning()) {
        const QString error = QStringLiteral("Remote transport is not running");
        qWarning() << "[Gateway]" << error;
        emit errorOccurred(0, error);
        return;
    }

    m_remoteTransport->sendCommand(requestPayload);
}

void MultiChannelRobotGateway::handleRemoteConnectionStateChanged(bool connected)
{
    m_remoteConnected = connected;

    emit connectionStateChanged(1, connected);
    emit connectionStateChanged(2, connected);

    qDebug() << "[Gateway] remote transport connected =" << connected;
}

void MultiChannelRobotGateway::pollSnapshots()
{
    switch (m_sourceMode) {
    case GatewaySourceMode::Dummy:
        publishDummySnapshot();
        break;

    case GatewaySourceMode::Remote:
        // Remote mode는 IRemoteTransportClient의 snapshotPayloadReceived signal 기반.
        break;
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

