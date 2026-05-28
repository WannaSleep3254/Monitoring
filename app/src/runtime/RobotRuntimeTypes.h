#pragma once

#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QDateTime>
#include <QString>

struct UnifiedRobotSnapshot
{
    int robotId = 0;

    bool connected = false;
    bool running = false;

    QVariantList jointPositions;
    QVariantList tcpPose;

    QVariantList torques;
    QVariantList driverTemperatures;

    QString robotState;
    QString errorText;
    QString sequenceState;
    QString interlockState;

    QDateTime timestamp;
    quint64 sequenceNumber = 0;

    QVariantMap toMap() const
    {
        QVariantMap m;
        m["robotId"] = robotId;
        m["connected"] = connected;
        m["running"] = running;
        m["jointPositions"] = jointPositions;
        m["tcpPose"] = tcpPose;
        m["torques"] = torques;
        m["driverTemperatures"] = driverTemperatures;
        m["robotState"] = robotState;
        m["errorText"] = errorText;
        m["sequenceState"] = sequenceState;
        m["interlockState"] = interlockState;
        m["timestamp"] = timestamp;
        m["sequenceNumber"] = QVariant::fromValue(sequenceNumber);
        return m;
    }
};
