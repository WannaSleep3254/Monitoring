#include "IotHistoryRepository.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>
#include <QStringList>

namespace
{
QString nowIso()
{
    return QDateTime::currentDateTime().toString(Qt::ISODate);
}

int boundedLimit(int limit)
{
    if (limit <= 0)
        return 100;

    if (limit > 5000)
        return 5000;

    return limit;
}

QString stringValue(const QVariantMap& map,
                    const QString& key1,
                    const QString& key2,
                    const QString& fallback = QString())
{
    if (map.contains(key1))
        return map.value(key1).toString();

    if (map.contains(key2))
        return map.value(key2).toString();

    return fallback;
}

int intValue(const QVariantMap& map,
             const QString& key1,
             const QString& key2,
             int fallback = 0)
{
    if (map.contains(key1))
        return map.value(key1).toInt();

    if (map.contains(key2))
        return map.value(key2).toInt();

    return fallback;
}

double doubleValue(const QVariantMap& map,
                   const QString& key1,
                   const QString& key2,
                   double fallback = 0.0)
{
    if (map.contains(key1))
        return map.value(key1).toDouble();

    if (map.contains(key2))
        return map.value(key2).toDouble();

    return fallback;
}

QVariantMap alarmFromQuery(const QSqlQuery& query)
{
    QVariantMap alarm;

    alarm["id"] = query.value(0).toLongLong();
    alarm["occurredAt"] = query.value(1).toString();
    alarm["robotId"] = query.value(2).toInt();
    alarm["axis"] = query.value(3).toString();
    alarm["metric"] = query.value(4).toString();
    alarm["value"] = query.value(5).toDouble();
    alarm["normalMax"] = query.value(6).toDouble();
    alarm["warningMax"] = query.value(7).toDouble();
    alarm["alarmMax"] = query.value(8).toDouble();
    alarm["level"] = query.value(9).toString();
    alarm["message"] = query.value(10).toString();
    alarm["source"] = query.value(11).toString();
    alarm["createdAt"] = query.value(12).toString();

    // QML 표시 편의용
    alarm["robotName"] = QString("Robot %1").arg(alarm["robotId"].toInt());

    return alarm;
}
}

IotHistoryRepository::IotHistoryRepository(const QSqlDatabase& database)
    : m_db(database)
{
}

bool IotHistoryRepository::insertAlarm(const QVariantMap& alarm)
{
    if (!m_db.isValid() || !m_db.isOpen()) {
        m_lastError = "Database is not open";
        qWarning() << "[IotHistoryRepository]" << m_lastError;
        return false;
    }

    const QString now = nowIso();

    const QString occurredAt =
        stringValue(alarm, "occurredAt", "occurred_at", now);

    const int robotId =
        intValue(alarm, "robotId", "robot_id", 0);

    const QString axis =
        stringValue(alarm, "axis", "axis", "-");

    const QString metric =
        stringValue(alarm, "metric", "metric", "");

    const double value =
        doubleValue(alarm, "value", "value", 0.0);

    const double normalMax =
        doubleValue(alarm, "normalMax", "normal_max", 0.0);

    const double warningMax =
        doubleValue(alarm, "warningMax", "warning_max", 0.0);

    const double alarmMax =
        doubleValue(alarm, "alarmMax", "alarm_max", 0.0);

    const QString level =
        stringValue(alarm, "level", "level", "WARNING");

    const QString source =
        stringValue(alarm, "source", "source", "auto");

    QString message =
        stringValue(alarm, "message", "message", "");

    if (robotId <= 0) {
        m_lastError = QString("Invalid robotId: %1").arg(robotId);
        qWarning() << "[IotHistoryRepository]" << m_lastError;
        return false;
    }

    if (metric != "temperature" && metric != "torque") {
        m_lastError = QString("Invalid metric: %1").arg(metric);
        qWarning() << "[IotHistoryRepository]" << m_lastError;
        return false;
    }

    if (message.isEmpty()) {
        message = QString("Robot %1 %2 %3 %4")
            .arg(robotId)
            .arg(axis)
            .arg(metric)
            .arg(level);
    }

    QSqlQuery query(m_db);

    query.prepare(
        "INSERT INTO iot_alarm_history "
        "(occurred_at, robot_id, axis, metric, value, "
        " normal_max, warning_max, alarm_max, level, message, source, created_at) "
        "VALUES "
        "(:occurred_at, :robot_id, :axis, :metric, :value, "
        " :normal_max, :warning_max, :alarm_max, :level, :message, :source, :created_at)"
        );

    query.bindValue(":occurred_at", occurredAt);
    query.bindValue(":robot_id", robotId);
    query.bindValue(":axis", axis);
    query.bindValue(":metric", metric);
    query.bindValue(":value", value);
    query.bindValue(":normal_max", normalMax);
    query.bindValue(":warning_max", warningMax);
    query.bindValue(":alarm_max", alarmMax);
    query.bindValue(":level", level);
    query.bindValue(":message", message);
    query.bindValue(":source", source);
    query.bindValue(":created_at", now);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        qWarning() << "[IotHistoryRepository] insertAlarm failed:"
                   << m_lastError;
        return false;
    }

    m_lastError.clear();

    qDebug() << "[IotHistoryRepository] alarm inserted"
             << "robotId =" << robotId
             << "axis =" << axis
             << "metric =" << metric
             << "value =" << value
             << "level =" << level;

    return true;
}

QVariantList IotHistoryRepository::loadRecentAlarms(int limit)
{
    QVariantList alarms;

    if (!m_db.isValid() || !m_db.isOpen()) {
        m_lastError = "Database is not open";
        qWarning() << "[IotHistoryRepository]" << m_lastError;
        return alarms;
    }

    QSqlQuery query(m_db);

    query.prepare(
        "SELECT "
        "id, occurred_at, robot_id, axis, metric, value, "
        "normal_max, warning_max, alarm_max, level, message, source, created_at "
        "FROM iot_alarm_history "
        "ORDER BY occurred_at DESC, id DESC "
        "LIMIT :limit"
        );

    query.bindValue(":limit", boundedLimit(limit));

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        qWarning() << "[IotHistoryRepository] loadRecentAlarms failed:"
                   << m_lastError;
        return alarms;
    }

    while (query.next()) {
        alarms.push_back(alarmFromQuery(query));
    }

    m_lastError.clear();

    return alarms;
}

QVariantList IotHistoryRepository::queryAlarms(const QVariantMap& filter)
{
    QVariantList alarms;

    if (!m_db.isValid() || !m_db.isOpen()) {
        m_lastError = "Database is not open";
        qWarning() << "[IotHistoryRepository]" << m_lastError;
        return alarms;
    }

    QStringList conditions;

    const int robotId = filter.value("robotId", 0).toInt();
    const QString metric = filter.value("metric").toString();
    const QString level = filter.value("level").toString();
    const QString from = filter.value("from").toString();
    const QString to = filter.value("to").toString();
    const QString searchText = filter.value("searchText").toString();
    const int limit = boundedLimit(filter.value("limit", 500).toInt());

    if (robotId > 0)
        conditions << "robot_id = :robot_id";

    if (!metric.isEmpty() && metric != "ALL" && metric != "전체")
        conditions << "metric = :metric";

    if (!level.isEmpty() && level != "ALL" && level != "전체")
        conditions << "level = :level";

    if (!from.isEmpty())
        conditions << "occurred_at >= :from";

    if (!to.isEmpty())
        conditions << "occurred_at <= :to";

    if (!searchText.isEmpty()) {
        conditions << "("
                      "message LIKE :search_text OR "
                      "axis LIKE :search_text OR "
                      "metric LIKE :search_text OR "
                      "level LIKE :search_text OR "
                      "source LIKE :search_text"
                      ")";
    }

    QString sql =
        "SELECT "
        "id, occurred_at, robot_id, axis, metric, value, "
        "normal_max, warning_max, alarm_max, level, message, source, created_at "
        "FROM iot_alarm_history ";

    if (!conditions.isEmpty()) {
        sql += "WHERE " + conditions.join(" AND ") + " ";
    }

    sql += "ORDER BY occurred_at DESC, id DESC LIMIT :limit";

    QSqlQuery query(m_db);
    query.prepare(sql);

    if (robotId > 0)
        query.bindValue(":robot_id", robotId);

    if (!metric.isEmpty() && metric != "ALL" && metric != "전체")
        query.bindValue(":metric", metric);

    if (!level.isEmpty() && level != "ALL" && level != "전체")
        query.bindValue(":level", level);

    if (!from.isEmpty())
        query.bindValue(":from", from);

    if (!to.isEmpty())
        query.bindValue(":to", to);

    if (!searchText.isEmpty())
        query.bindValue(":search_text", "%" + searchText + "%");

    query.bindValue(":limit", limit);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        qWarning() << "[IotHistoryRepository] queryAlarms failed:"
                   << m_lastError;
        return alarms;
    }

    while (query.next()) {
        alarms.push_back(alarmFromQuery(query));
    }

    m_lastError.clear();

    return alarms;
}

bool IotHistoryRepository::deleteOldHistory(int retentionMonths)
{
    if (!m_db.isValid() || !m_db.isOpen()) {
        m_lastError = "Database is not open";
        qWarning() << "[IotHistoryRepository]" << m_lastError;
        return false;
    }

    if (retentionMonths <= 0) {
        m_lastError = QString("Invalid retentionMonths: %1").arg(retentionMonths);
        qWarning() << "[IotHistoryRepository]" << m_lastError;
        return false;
    }

    const QString cutoff =
        QDateTime::currentDateTime()
            .addMonths(-retentionMonths)
            .toString(Qt::ISODate);

    QSqlQuery query(m_db);

    // 오래된 알람에 연결된 조치 이력 먼저 삭제
    query.prepare(
        "DELETE FROM iot_action_history "
        "WHERE alarm_id IN ("
        "    SELECT id FROM iot_alarm_history "
        "    WHERE occurred_at < :cutoff OR created_at < :cutoff"
        ")"
        );
    query.bindValue(":cutoff", cutoff);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        qWarning() << "[IotHistoryRepository] delete old linked actions failed:"
                   << m_lastError;
        return false;
    }

    // 독립적으로 오래된 조치 이력 삭제
    query.prepare(
        "DELETE FROM iot_action_history "
        "WHERE action_at < :cutoff OR created_at < :cutoff"
        );
    query.bindValue(":cutoff", cutoff);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        qWarning() << "[IotHistoryRepository] delete old actions failed:"
                   << m_lastError;
        return false;
    }

    // 오래된 알람 이력 삭제
    query.prepare(
        "DELETE FROM iot_alarm_history "
        "WHERE occurred_at < :cutoff OR created_at < :cutoff"
        );
    query.bindValue(":cutoff", cutoff);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        qWarning() << "[IotHistoryRepository] delete old alarms failed:"
                   << m_lastError;
        return false;
    }

    m_lastError.clear();

    qDebug() << "[IotHistoryRepository] old history deleted"
             << "retentionMonths =" << retentionMonths
             << "cutoff =" << cutoff;

    return true;
}

QString IotHistoryRepository::lastError() const
{
    return m_lastError;
}
