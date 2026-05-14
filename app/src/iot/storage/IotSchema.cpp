#include "IotSchema.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

IotSchema::IotSchema(const QSqlDatabase& database)
    : m_db(database)
{
}

bool IotSchema::createTables()
{
    if (!m_db.isValid() || !m_db.isOpen()) {
        m_lastError = "Database is not open";
        qWarning() << "[IotSchema]" << m_lastError;
        return false;
    }

    QSqlQuery query(m_db);

    const QString sql =
        "CREATE TABLE IF NOT EXISTS iot_threshold_settings ("
        "robot_id INTEGER NOT NULL, "
        "metric TEXT NOT NULL, "
        "normal_max REAL NOT NULL, "
        "warning_max REAL NOT NULL, "
        "alarm_max REAL NOT NULL, "
        "unit TEXT NOT NULL, "
        "updated_at TEXT NOT NULL, "
        "PRIMARY KEY(robot_id, metric)"
        ")";

    if (!query.exec(sql)) {
        m_lastError = query.lastError().text();
        qWarning() << "[IotSchema] create iot_threshold_settings failed:"
                   << m_lastError;
        return false;
    }

    m_lastError.clear();

    qDebug() << "[IotSchema] tables ready";

    return true;
}

QString IotSchema::lastError() const
{
    return m_lastError;
}
