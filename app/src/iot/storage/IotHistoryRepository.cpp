#include "IotHistoryRepository.h"

IotHistoryRepository::IotHistoryRepository(const QSqlDatabase& database)
{

}

bool IotHistoryRepository::insertAlarm(const QVariantMap& alarm)
{
    return false;
}

QVariantList IotHistoryRepository::loadRecentAlarms(int limit)
{
    return QVariantList();
}

QVariantList IotHistoryRepository::queryAlarms(const QVariantMap& filter)
{
    return QVariantList();
}

bool IotHistoryRepository::deleteOldHistory(int retentionMonths)
{
    return false;
}

QString IotHistoryRepository::lastError() const
{
    return m_lastError;
}
