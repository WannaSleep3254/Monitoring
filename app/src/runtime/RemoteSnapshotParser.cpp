#include "RemoteSnapshotParser.h"

#include "RemoteMessageTypes.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonValue>
#include <QVariant>

RemoteSnapshotParser::Result RemoteSnapshotParser::parseJson(const QByteArray& payload)
{
    Result result;

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(payload, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        result.error = QStringLiteral("JSON parse error: %1")
        .arg(parseError.errorString());
        return result;
    }

    if (!doc.isObject()) {
        result.error = QStringLiteral("JSON root is not an object");
        return result;
    }

    return parseObject(doc.object());
}

RemoteSnapshotParser::Result RemoteSnapshotParser::parseObject(const QJsonObject& object)
{
    Result result;

    const QString messageType =
        object.value(RemoteMessage::Field::MessageType).toString();

    if (messageType != RemoteMessage::Type::Snapshot) {
        result.error = QStringLiteral("Invalid messageType: %1").arg(messageType);
        return result;
    }

    const int robotId = object.value(RemoteMessage::Field::RobotId).toInt(0);

    if (robotId <= 0) {
        result.error = QStringLiteral("Invalid or missing robotId");
        return result;
    }

    QVariantMap snapshot;
    snapshot[RemoteMessage::Field::RobotId] = robotId;

    snapshot[RemoteMessage::Field::Connected] =
        object.value(RemoteMessage::Field::Connected).toBool(false);

    snapshot[RemoteMessage::Field::Running] =
        object.value(RemoteMessage::Field::Running).toBool(false);

    snapshot[RemoteMessage::Field::JointPositions] =
        normalizedNumberArray(object, RemoteMessage::Field::JointPositions);

    snapshot[RemoteMessage::Field::TcpPose] =
        normalizedNumberArray(object, RemoteMessage::Field::TcpPose);

    snapshot[RemoteMessage::Field::Torques] =
        normalizedNumberArray(object, RemoteMessage::Field::Torques);

    snapshot[RemoteMessage::Field::DriverTemperatures] =
        normalizedNumberArray(object, RemoteMessage::Field::DriverTemperatures);

    snapshot[RemoteMessage::Field::RobotState] =
        object.value(RemoteMessage::Field::RobotState).toString(QStringLiteral("-"));

    snapshot[RemoteMessage::Field::ErrorText] =
        object.value(RemoteMessage::Field::ErrorText).toString(QStringLiteral("0"));

    snapshot[RemoteMessage::Field::SequenceState] =
        object.value(RemoteMessage::Field::SequenceState).toString(QStringLiteral("-"));

    snapshot[RemoteMessage::Field::InterlockState] =
        object.value(RemoteMessage::Field::InterlockState).toString(QStringLiteral("-"));

    snapshot[RemoteMessage::Field::Timestamp] = parseTimestamp(object);

    const double sequenceValue =
        object.value(RemoteMessage::Field::SequenceNumber).toDouble(0.0);

    snapshot[RemoteMessage::Field::SequenceNumber] =
        QVariant::fromValue(static_cast<qulonglong>(sequenceValue));

    result.ok = true;
    result.robotId = robotId;
    result.snapshot = snapshot;

    return result;
}

QVariantList RemoteSnapshotParser::normalizedNumberArray(const QJsonObject& object,
                                                         const QString& fieldName,
                                                         int expectedSize)
{
    QVariantList values;

    const QJsonArray array = object.value(fieldName).toArray();

    for (int i = 0; i < expectedSize; ++i) {
        if (i < array.size()) {
            values.push_back(array.at(i).toDouble(0.0));
        } else {
            values.push_back(0.0);
        }
    }

    return values;
}

QDateTime RemoteSnapshotParser::parseTimestamp(const QJsonObject& object)
{
    const QString timestampText =
        object.value(RemoteMessage::Field::Timestamp).toString();

    if (!timestampText.isEmpty()) {
        QDateTime timestamp =
            QDateTime::fromString(timestampText, Qt::ISODateWithMs);

        if (!timestamp.isValid()) {
            timestamp = QDateTime::fromString(timestampText, Qt::ISODate);
        }

        if (timestamp.isValid()) {
            return timestamp;
        }
    }

    return QDateTime::currentDateTime();
}
