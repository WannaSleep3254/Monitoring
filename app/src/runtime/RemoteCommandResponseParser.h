#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QJsonObject>
#include <QString>

class RemoteCommandResponseParser
{
public:
    struct Result
    {
        bool parsed = false;

        QString commandId;
        int robotId = 0;
        QString command;

        bool ok = false;
        int code = -1;
        QString message;
        QDateTime timestamp;

        QString error;
    };

    static Result parseJson(const QByteArray& payload);
    static Result parseObject(const QJsonObject& object);
};
