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
    return createThresholdTables()
           && createHistoryTables();
}

bool IotSchema::createThresholdTables()
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

bool IotSchema::createHistoryTables()
{
    if (!m_db.isValid() || !m_db.isOpen()) {
        m_lastError = "Database is not open";
        qWarning() << "[IotSchema]" << m_lastError;
        return false;
    }

    QSqlQuery query(m_db);

    const QString alarmSql =
        "CREATE TABLE IF NOT EXISTS iot_alarm_history ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "occurred_at TEXT NOT NULL, "
        "robot_id INTEGER NOT NULL, "
        "axis TEXT NOT NULL, "
        "metric TEXT NOT NULL, "
        "value REAL NOT NULL, "
        "normal_max REAL NOT NULL, "
        "warning_max REAL NOT NULL, "
        "alarm_max REAL NOT NULL, "
        "level TEXT NOT NULL, "
        "message TEXT NOT NULL, "
        "source TEXT NOT NULL, "
        "created_at TEXT NOT NULL"
        ")";

    if (!query.exec(alarmSql)) {
        m_lastError = query.lastError().text();
        qWarning() << "[IotSchema] create iot_alarm_history failed:"
                   << m_lastError;
        return false;
    }

    const QString actionSql =
        "CREATE TABLE IF NOT EXISTS iot_action_history ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "alarm_id INTEGER, "
        "action_at TEXT NOT NULL, "
        "robot_id INTEGER NOT NULL, "
        "status TEXT NOT NULL, "
        "action_content TEXT NOT NULL, "
        "operator_name TEXT, "
        "memo TEXT, "
        "created_at TEXT NOT NULL"
        ")";

    if (!query.exec(actionSql)) {
        m_lastError = query.lastError().text();
        qWarning() << "[IotSchema] create iot_action_history failed:"
                   << m_lastError;
        return false;
    }

    const QString alarmOccurredAtIndexSql =
        "CREATE INDEX IF NOT EXISTS idx_iot_alarm_history_occurred_at "
        "ON iot_alarm_history(occurred_at)";

    if (!query.exec(alarmOccurredAtIndexSql)) {
        m_lastError = query.lastError().text();
        qWarning() << "[IotSchema] create alarm occurred_at index failed:"
                   << m_lastError;
        return false;
    }

    const QString alarmRobotMetricIndexSql =
        "CREATE INDEX IF NOT EXISTS idx_iot_alarm_history_robot_metric "
        "ON iot_alarm_history(robot_id, metric)";

    if (!query.exec(alarmRobotMetricIndexSql)) {
        m_lastError = query.lastError().text();
        qWarning() << "[IotSchema] create alarm robot metric index failed:"
                   << m_lastError;
        return false;
    }

    const QString alarmLevelIndexSql =
        "CREATE INDEX IF NOT EXISTS idx_iot_alarm_history_level "
        "ON iot_alarm_history(level)";

    if (!query.exec(alarmLevelIndexSql)) {
        m_lastError = query.lastError().text();
        qWarning() << "[IotSchema] create alarm level index failed:"
                   << m_lastError;
        return false;
    }

    const QString actionAtIndexSql =
        "CREATE INDEX IF NOT EXISTS idx_iot_action_history_action_at "
        "ON iot_action_history(action_at)";

    if (!query.exec(actionAtIndexSql)) {
        m_lastError = query.lastError().text();
        qWarning() << "[IotSchema] create action action_at index failed:"
                   << m_lastError;
        return false;
    }

    const QString actionAlarmIdIndexSql =
        "CREATE INDEX IF NOT EXISTS idx_iot_action_history_alarm_id "
        "ON iot_action_history(alarm_id)";

    if (!query.exec(actionAlarmIdIndexSql)) {
        m_lastError = query.lastError().text();
        qWarning() << "[IotSchema] create action alarm_id index failed:"
                   << m_lastError;
        return false;
    }

    m_lastError.clear();

    qDebug() << "[IotSchema] history tables ready";

    return true;
}

QString IotSchema::lastError() const
{
    return m_lastError;
}
