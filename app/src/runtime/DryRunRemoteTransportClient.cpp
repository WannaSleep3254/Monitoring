#include "DryRunRemoteTransportClient.h"

#include "RemoteMessageTypes.h"

#include <QDateTime>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

#include <initializer_list>

namespace
{
QJsonArray makeArray(std::initializer_list<double> values)
{
    QJsonArray array;

    for (double value : values) {
        array.append(value);
    }

    return array;
}

QString successMessageForCommand(const QString& command)
{
    if (command == RemoteMessage::Command::SetManualMode)
        return QStringLiteral("dry-run manual mode");

    if (command == RemoteMessage::Command::SetAutoMode)
        return QStringLiteral("dry-run auto mode");

    if (command == RemoteMessage::Command::ClearError)
        return QStringLiteral("dry-run clear error");

    if (command == RemoteMessage::Command::StartJointJog)
        return QStringLiteral("dry-run jog start");

    if (command == RemoteMessage::Command::StopJointJog)
        return QStringLiteral("dry-run jog stop");

    if (command == RemoteMessage::Command::JogHeartbeat)
        return QStringLiteral("dry-run jog heartbeat");

    return QStringLiteral("dry-run command ok");
}
}

DryRunRemoteTransportClient::DryRunRemoteTransportClient(QObject* parent)
    : IRemoteTransportClient(parent)
{
    connect(&m_snapshotTimer,
            &QTimer::timeout,
            this,
            &DryRunRemoteTransportClient::publishDryRunSnapshots);
}

void DryRunRemoteTransportClient::configure(const RemoteTransportConfig& config)
{
    m_config = config;
}

bool DryRunRemoteTransportClient::start()
{
    if (m_running)
        return true;

    m_running = true;

    publishDryRunSnapshots();
    m_snapshotTimer.start(1000);

    emit connectionStateChanged(true);

    qDebug() << "[DryRunTransport] started"
             << "snapshotEndpoint =" << m_config.snapshotEndpoint
             << "commandEndpoint =" << m_config.commandEndpoint
             << "topic =" << m_config.snapshotTopic;

    return true;
}

void DryRunRemoteTransportClient::stop()
{
    if (!m_running)
        return;

    m_snapshotTimer.stop();
    m_running = false;

    emit connectionStateChanged(false);

    qDebug() << "[DryRunTransport] stopped";
}

bool DryRunRemoteTransportClient::isRunning() const
{
    return m_running;
}

void DryRunRemoteTransportClient::sendCommand(const QByteArray& requestPayload)
{
    QString errorMessage;
    const QByteArray responsePayload =
        buildCommandResponsePayload(requestPayload, &errorMessage);

    if (!errorMessage.isEmpty()) {
        emit errorOccurred(errorMessage);
        qWarning() << "[DryRunTransport]" << errorMessage;
        return;
    }

    qDebug() << "[DryRunTransport] command request =" << requestPayload
             << "response =" << responsePayload;

    emit commandResponsePayloadReceived(responsePayload);
}

void DryRunRemoteTransportClient::publishDryRunSnapshots()
{
    if (!m_running)
        return;

    ++m_sequence;

    emit snapshotPayloadReceived(buildSnapshotPayload(1));
    emit snapshotPayloadReceived(buildSnapshotPayload(2));
}

QByteArray DryRunRemoteTransportClient::buildSnapshotPayload(int robotId)
{
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

    return QJsonDocument(object).toJson(QJsonDocument::Compact);
}

QByteArray DryRunRemoteTransportClient::buildCommandResponsePayload(
    const QByteArray& requestPayload,
    QString* errorMessage) const
{
    QJsonParseError parseError;
    const QJsonDocument requestDoc =
        QJsonDocument::fromJson(requestPayload, &parseError);

    if (parseError.error != QJsonParseError::NoError || !requestDoc.isObject()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Command request parse failed: %1")
                                .arg(parseError.errorString());
        }

        return {};
    }

    const QJsonObject requestObject = requestDoc.object();

    const int robotId =
        requestObject.value(RemoteMessage::Field::RobotId).toInt(0);

    const QString commandId =
        requestObject.value(RemoteMessage::Field::CommandId).toString();

    const QString command =
        requestObject.value(RemoteMessage::Field::Command).toString();

    if (robotId <= 0 || commandId.isEmpty() || command.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Invalid command request fields");
        }

        return {};
    }

    QJsonObject responseObject;

    responseObject[RemoteMessage::Field::MessageType] =
        RemoteMessage::Type::CommandResponse;

    responseObject[RemoteMessage::Field::SchemaVersion] = 1;
    responseObject[RemoteMessage::Field::CommandId] = commandId;
    responseObject[RemoteMessage::Field::RobotId] = robotId;
    responseObject[RemoteMessage::Field::Command] = command;

    responseObject[RemoteMessage::Field::Ok] = true;
    responseObject[RemoteMessage::Field::Code] =
        RemoteMessage::toInt(RemoteMessage::ResponseCode::Ok);

    responseObject[RemoteMessage::Field::Message] =
        successMessageForCommand(command);

    responseObject[RemoteMessage::Field::Timestamp] =
        QDateTime::currentDateTime().toString(Qt::ISODateWithMs);

    if (errorMessage) {
        errorMessage->clear();
    }

    return QJsonDocument(responseObject).toJson(QJsonDocument::Compact);
}
