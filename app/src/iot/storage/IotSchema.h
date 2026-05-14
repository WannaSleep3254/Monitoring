#pragma once

#include <QSqlDatabase>
#include <QString>

class IotDatabase
{
public:
    explicit IotDatabase(const QString& connectionName = "iot_connection");
    ~IotDatabase();

    bool open(const QString& databasePath = QString());
    void close();

    bool isOpen() const;
    QSqlDatabase database() const;

    QString databasePath() const;
    QString lastError() const;

private:
    QString defaultDatabasePath() const;

private:
    QString m_connectionName;
    QString m_databasePath;
    QString m_lastError;
};
