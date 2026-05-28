#pragma once

#include <QString>

namespace RemoteMessage
{
    namespace Type
    {
        inline const QString Snapshot        = QStringLiteral("snapshot");
        inline const QString CommandRequest  = QStringLiteral("commandRequest");
        inline const QString CommandResponse = QStringLiteral("commandResponse");
        inline const QString CommandEvent    = QStringLiteral("commandEvent");
    }

    namespace Field
    {
        inline const QString MessageType     = QStringLiteral("messageType");
        inline const QString SchemaVersion   = QStringLiteral("schemaVersion");

        inline const QString RobotId         = QStringLiteral("robotId");
        inline const QString RobotName       = QStringLiteral("robotName");
        inline const QString Model           = QStringLiteral("model");

        inline const QString Connected       = QStringLiteral("connected");
        inline const QString Running         = QStringLiteral("running");

        inline const QString JointPositions  = QStringLiteral("jointPositions");
        inline const QString TcpPose         = QStringLiteral("tcpPose");
        inline const QString Torques         = QStringLiteral("torques");
        inline const QString DriverTemperatures = QStringLiteral("driverTemperatures");

        inline const QString RobotState      = QStringLiteral("robotState");
        inline const QString ErrorText       = QStringLiteral("errorText");
        inline const QString SequenceState   = QStringLiteral("sequenceState");
        inline const QString InterlockState  = QStringLiteral("interlockState");

        inline const QString Timestamp       = QStringLiteral("timestamp");
        inline const QString SequenceNumber  = QStringLiteral("sequenceNumber");

        inline const QString CommandId       = QStringLiteral("commandId");
        inline const QString Command         = QStringLiteral("command");
        inline const QString Params          = QStringLiteral("params");
        inline const QString Operator        = QStringLiteral("operator");
        inline const QString TimeoutMs       = QStringLiteral("timeoutMs");

        inline const QString Ok              = QStringLiteral("ok");
        inline const QString Code            = QStringLiteral("code");
        inline const QString Message         = QStringLiteral("message");

        inline const QString Event           = QStringLiteral("event");
        inline const QString Level           = QStringLiteral("level");
    }

    namespace Command
    {
        inline const QString SetManualMode   = QStringLiteral("setManualMode");
        inline const QString SetAutoMode     = QStringLiteral("setAutoMode");
        inline const QString ClearError      = QStringLiteral("clearError");

        inline const QString Home            = QStringLiteral("home");
        inline const QString Start           = QStringLiteral("start");
        inline const QString Stop            = QStringLiteral("stop");

        inline const QString StartJointJog   = QStringLiteral("startJointJog");
        inline const QString StopJointJog    = QStringLiteral("stopJointJog");
        inline const QString JogHeartbeat    = QStringLiteral("jogHeartbeat");
    }

    namespace Param
    {
        inline const QString Joint           = QStringLiteral("joint");
        inline const QString Direction       = QStringLiteral("direction");
        inline const QString Speed           = QStringLiteral("speed");
        inline const QString JogSessionId    = QStringLiteral("jogSessionId");
    }

    namespace Event
    {
        inline const QString JogStarted      = QStringLiteral("jogStarted");
        inline const QString JogStopped      = QStringLiteral("jogStopped");
        inline const QString JogTimeoutStop  = QStringLiteral("jogTimeoutStop");
        inline const QString CommandRejected = QStringLiteral("commandRejected");
        inline const QString RobotDisconnected = QStringLiteral("robotDisconnected");
        inline const QString InterlockChanged  = QStringLiteral("interlockChanged");
    }

    enum class ResponseCode
    {
        Ok = 0,

        UnknownCommand = 100,
        InvalidParameter = 101,
        RobotNotFound = 102,

        RobotDisconnected = 200,
        NotManualMode = 201,
        InterlockNotOk = 202,
        EmergencyStop = 203,

        SdkCommandFailed = 300,
        SdkTimeout = 301,

        CommandBusy = 400,

        InternalError = 500
    };

    inline int toInt(ResponseCode code)
    {
        return static_cast<int>(code);
    }

} // namespace RemoteMessage
