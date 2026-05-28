#include "RemoteCommandResponseParser.h"

#include "RemoteMessageTypes.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonParseError>

RemoteCommandResponseParser::Result
RemoteCommandResponseParser::parseJson(const QByteArray& payload)
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

RemoteCommandResponseParser::Result
RemoteCommandResponseParser::parseObject(const QJsonObject& object)
{
    Result result;

    const QString messageType =
        object.value(RemoteMessage::Field::MessageType).toString();

    if (messageType != RemoteMessage::Type::CommandResponse) {
        result.error = QStringLiteral("Invalid messageType: %1")
                           .arg(messageType);
        return result;
    }

    result.commandId =
        object.value(RemoteMessage::Field::CommandId).toString();

    result.robotId =
        object.value(RemoteMessage::Field::RobotId).toInt(0);

    result.command =
        object.value(RemoteMessage::Field::Command).toString();

    result.ok =
        object.value(RemoteMessage::Field::Ok).toBool(false);

    result.code =
        object.value(RemoteMessage::Field::Code).toInt(-1);

    result.message =
        object.value(RemoteMessage::Field::Message).toString();

    const QString timestampText =
        object.value(RemoteMessage::Field::Timestamp).toString();

    if (!timestampText.isEmpty()) {
        result.timestamp =
            QDateTime::fromString(timestampText, Qt::ISODateWithMs);

        if (!result.timestamp.isValid()) {
            result.timestamp =
                QDateTime::fromString(timestampText, Qt::ISODate);
        }
    }

    if (!result.timestamp.isValid()) {
        result.timestamp = QDateTime::currentDateTime();
    }

    if (result.commandId.isEmpty()) {
        result.error = QStringLiteral("Missing commandId");
        return result;
    }

    if (result.robotId <= 0) {
        result.error = QStringLiteral("Invalid or missing robotId");
        return result;
    }

    if (result.command.isEmpty()) {
        result.error = QStringLiteral("Missing command");
        return result;
    }

    result.parsed = true;
    return result;
}
