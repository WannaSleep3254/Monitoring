#pragma once

#include <QSqlDatabase>
#include <QString>

class IotSchema
{
public:
    explicit IotSchema(const QSqlDatabase& database);

    bool createTables();
    bool createThresholdTables();
    bool createHistoryTables();

    QString lastError() const;

private:
    QSqlDatabase m_db;
    QString m_lastError;
};
