#pragma once

#include <QSqlDatabase>
#include <QVariantMap>
#include <QVariantList>
#include <QString>

class IotHistoryRepository
{
public:
    explicit IotHistoryRepository(const QSqlDatabase& database);

    bool insertAlarm(const QVariantMap& alarm);

    QVariantList loadRecentAlarms(int limit);

    QVariantList queryAlarms(const QVariantMap& filter);

    bool deleteOldHistory(int retentionMonths);

    QString lastError() const;

private:
    QSqlDatabase m_db;
    QString m_lastError;
};
