#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <QVariantMap>

class RemoteSnapshotParser
{
public:
    struct Result
    {
        bool ok = false;
        int robotId = 0;
        QVariantMap snapshot;
        QString error;
    };

    static Result parseJson(const QByteArray& payload);
    static Result parseObject(const QJsonObject& object);

private:
    static QVariantList normalizedNumberArray(const QJsonObject& object,
                                              const QString& fieldName,
                                              int expectedSize = 6);

    static QDateTime parseTimestamp(const QJsonObject& object);
};
