#pragma once

#include <QSqlDatabase>
#include <QString>

class IotSchema
{
public:
    explicit IotSchema(const QSqlDatabase& database);

    bool createTables();

    QString lastError() const;

private:
    bool createThresholdTables();
    bool createHistoryTables();

private:
    QSqlDatabase m_db;
    QString m_lastError;
};
