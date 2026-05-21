#include "RemoteCommandBuilder.h"

#include "RemoteMessageTypes.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

namespace
{
constexpr int kDefaultTimeoutMs = 1000;
constexpr int kJogTimeoutMs = 500;
constexpr int kJogHeartbeatTimeoutMs = 300;
constexpr int kHomeTimeoutMs = 3000;
}

QByteArray RemoteCommandBuilder::buildSetManualMode(int robotId,
                                                    const QString& operatorName)
{
    return buildCommand(robotId,
                        RemoteMessage::Command::SetManualMode,
                        QVariantMap{},
                        operatorName,
                        kDefaultTimeoutMs);
}

QByteArray RemoteCommandBuilder::buildSetAutoMode(int robotId,
                                                  const QString& operatorName)
{
    return buildCommand(robotId,
                        RemoteMessage::Command::SetAutoMode,
                        QVariantMap{},
                        operatorName,
                        kDefaultTimeoutMs);
}

QByteArray RemoteCommandBuilder::buildClearError(int robotId,
                                                 const QString& operatorName)
{
    return buildCommand(robotId,
                        RemoteMessage::Command::ClearError,
                        QVariantMap{},
                        operatorName,
                        kDefaultTimeoutMs);
}

QByteArray RemoteCommandBuilder::buildHome(int robotId,
                                           const QString& operatorName)
{
    return buildCommand(robotId,
                        RemoteMessage::Command::Home,
                        QVariantMap{},
                        operatorName,
                        kHomeTimeoutMs);
}

QByteArray RemoteCommandBuilder::buildStart(int robotId,
                                            const QString& operatorName)
{
    return buildCommand(robotId,
                        RemoteMessage::Command::Start,
                        QVariantMap{},
                        operatorName,
                        kDefaultTimeoutMs);
}

QByteArray RemoteCommandBuilder::buildStop(int robotId,
                                           const QString& operatorName)
{
    return buildCommand(robotId,
                        RemoteMessage::Command::Stop,
                        QVariantMap{},
                        operatorName,
                        kDefaultTimeoutMs);
}

QByteArray RemoteCommandBuilder::buildStartJointJog(int robotId,
                                                    int joint,
                                                    bool positive,
                                                    double speed,
                                                    const QString& operatorName)
{
    QVariantMap params;
    params[RemoteMessage::Param::Joint] = joint;
    params[RemoteMessage::Param::Direction] = positive
                                                  ? QStringLiteral("+")
                                                  : QStringLiteral("-");
    params[RemoteMessage::Param::Speed] = speed;

    return buildCommand(robotId,
                        RemoteMessage::Command::StartJointJog,
                        params,
                        operatorName,
                        kJogTimeoutMs);
}

QByteArray RemoteCommandBuilder::buildStopJointJog(int robotId,
                                                   const QString& operatorName)
{
    return buildCommand(robotId,
                        RemoteMessage::Command::StopJointJog,
                        QVariantMap{},
                        operatorName,
                        kJogTimeoutMs);
}

QByteArray RemoteCommandBuilder::buildJogHeartbeat(int robotId,
                                                   const QString& jogSessionId,
                                                   const QString& operatorName)
{
    QVariantMap params;
    params[RemoteMessage::Param::JogSessionId] = jogSessionId;

    return buildCommand(robotId,
                        RemoteMessage::Command::JogHeartbeat,
                        params,
                        operatorName,
                        kJogHeartbeatTimeoutMs);
}

QByteArray RemoteCommandBuilder::buildCommand(int robotId,
                                              const QString& command,
                                              const QVariantMap& params,
                                              const QString& operatorName,
                                              int timeoutMs)
{
    QJsonObject object;

    object[RemoteMessage::Field::MessageType] =
        RemoteMessage::Type::CommandRequest;

    object[RemoteMessage::Field::SchemaVersion] = 1;
    object[RemoteMessage::Field::CommandId] = makeCommandId();

    object[RemoteMessage::Field::RobotId] = robotId;
    object[RemoteMessage::Field::Command] = command;
    object[RemoteMessage::Field::Params] = QJsonObject::fromVariantMap(params);

    if (!operatorName.trimmed().isEmpty()) {
        object[RemoteMessage::Field::Operator] = operatorName.trimmed();
    }

    object[RemoteMessage::Field::Timestamp] =
        QDateTime::currentDateTime().toString(Qt::ISODateWithMs);

    object[RemoteMessage::Field::TimeoutMs] = timeoutMs;

    return QJsonDocument(object).toJson(QJsonDocument::Compact);
}

QString RemoteCommandBuilder::makeCommandId()
{
    return QStringLiteral("cmd-%1")
    .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}
