#include "IotDatabase.h"

#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QStandardPaths>
#include <QDebug>

IotDatabase::IotDatabase(const QString& connectionName)
    : m_connectionName(connectionName)
{
}

IotDatabase::~IotDatabase()
{
    close();
}

bool IotDatabase::open(const QString& databasePath)
{
    const QString path = databasePath.isEmpty()
    ? defaultDatabasePath()
    : databasePath;

    const QFileInfo info(path);
    QDir dir(info.absolutePath());

    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            m_lastError = QString("Failed to create database directory: %1")
            .arg(info.absolutePath());
            qWarning() << "[IotDatabase]" << m_lastError;
            return false;
        }
    }

    QSqlDatabase db;

    if (QSqlDatabase::contains(m_connectionName)) {
        db = QSqlDatabase::database(m_connectionName);
    } else {
        db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    }

    db.setDatabaseName(path);

    if (!db.open()) {
        m_lastError = db.lastError().text();
        qWarning() << "[IotDatabase] open failed:" << m_lastError;
        return false;
    }

    m_databasePath = path;
    m_lastError.clear();

    qDebug() << "[IotDatabase] opened:" << m_databasePath;

    return true;
}

void IotDatabase::close()
{
    if (!QSqlDatabase::contains(m_connectionName))
        return;

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);

    if (db.isOpen()) {
        db.close();
    }
}

bool IotDatabase::isOpen() const
{
    if (!QSqlDatabase::contains(m_connectionName))
        return false;

    return QSqlDatabase::database(m_connectionName).isOpen();
}

QSqlDatabase IotDatabase::database() const
{
    return QSqlDatabase::database(m_connectionName);
}

QString IotDatabase::databasePath() const
{
    return m_databasePath;
}

QString IotDatabase::lastError() const
{
    return m_lastError;
}

QString IotDatabase::defaultDatabasePath() const
{
#if false // legacy path for testing
    QString basePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    if (basePath.isEmpty()) {
        basePath = QDir::currentPath();
    }

    return QDir(basePath).filePath("iot_monitoring.db");
#else   // organization 적용 후 경로: .../Gachisoft/MonitoringApp/iot_monitoring.db ->  .../MonitoringApp/iot_monitoring.db
    QString roamingPath =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    if (roamingPath.isEmpty()) {
        roamingPath = QDir::currentPath();
    }

    QDir dir(roamingPath);

    // organization 적용 후: .../Gachisoft/MonitoringApp
    // 기존 경로로 되돌림: .../MonitoringApp
    if (dir.dirName() == "MonitoringApp") {
        dir.cdUp();

        if (dir.dirName() == "Gachisoft") {
            dir.cdUp();
        }

        return QDir(dir.filePath("MonitoringApp")).filePath("iot_monitoring.db");
    }

    return QDir(roamingPath).filePath("iot_monitoring.db");
#endif
}
