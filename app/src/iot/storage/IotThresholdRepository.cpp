#include "IotThresholdRepository.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>

IotThresholdRepository::IotThresholdRepository(const QSqlDatabase& database)
    : m_db(database)
{
}

bool IotThresholdRepository::saveThreshold(int robotId,
                                           const QString& metric,
                                           const QVariantMap& threshold,
                                           const QString& unit)
{
    if (robotId <= 0) {
        m_lastError = QString("Invalid robotId: %1").arg(robotId);
        qWarning() << "[IotThresholdRepository]" << m_lastError;
        return false;
    }

    if (!isValidMetric(metric)) {
        m_lastError = QString("Invalid metric: %1").arg(metric);
        qWarning() << "[IotThresholdRepository]" << m_lastError;
        return false;
    }

    if (!m_db.isValid() || !m_db.isOpen()) {
        m_lastError = "Database is not open";
        qWarning() << "[IotThresholdRepository]" << m_lastError;
        return false;
    }

    const double normalMax  = threshold.value("normalMax", 0.0).toDouble();
    const double warningMax = threshold.value("warningMax", 0.0).toDouble();
    const double alarmMax   = threshold.value("alarmMax", 0.0).toDouble();

    QSqlQuery query(m_db);

    query.prepare(
        "INSERT INTO iot_threshold_settings "
        "(robot_id, metric, normal_max, warning_max, alarm_max, unit, updated_at) "
        "VALUES "
        "(:robot_id, :metric, :normal_max, :warning_max, :alarm_max, :unit, :updated_at) "
        "ON CONFLICT(robot_id, metric) DO UPDATE SET "
        "normal_max = excluded.normal_max, "
        "warning_max = excluded.warning_max, "
        "alarm_max = excluded.alarm_max, "
        "unit = excluded.unit, "
        "updated_at = excluded.updated_at"
        );

    query.bindValue(":robot_id", robotId);
    query.bindValue(":metric", metric);
    query.bindValue(":normal_max", normalMax);
    query.bindValue(":warning_max", warningMax);
    query.bindValue(":alarm_max", alarmMax);
    query.bindValue(":unit", unit);
    query.bindValue(":updated_at", QDateTime::currentDateTime().toString(Qt::ISODate));

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        qWarning() << "[IotThresholdRepository] saveThreshold failed:"
                   << m_lastError;
        return false;
    }

    m_lastError.clear();

    qDebug() << "[IotThresholdRepository] threshold saved"
             << "robotId =" << robotId
             << "metric =" << metric
             << "normal =" << normalMax
             << "warning =" << warningMax
             << "alarm =" << alarmMax
             << "unit =" << unit;

    return true;
}

QVariantMap IotThresholdRepository::loadThreshold(int robotId,
                                                  const QString& metric,
                                                  const QVariantMap& fallback)
{
    if (robotId <= 0) {
        m_lastError = QString("Invalid robotId: %1").arg(robotId);
        qWarning() << "[IotThresholdRepository]" << m_lastError;
        return fallback;
    }

    if (!isValidMetric(metric)) {
        m_lastError = QString("Invalid metric: %1").arg(metric);
        qWarning() << "[IotThresholdRepository]" << m_lastError;
        return fallback;
    }

    if (!m_db.isValid() || !m_db.isOpen()) {
        m_lastError = "Database is not open";
        qWarning() << "[IotThresholdRepository]" << m_lastError;
        return fallback;
    }

    QSqlQuery query(m_db);

    query.prepare(
        "SELECT normal_max, warning_max, alarm_max, unit "
        "FROM iot_threshold_settings "
        "WHERE robot_id = :robot_id AND metric = :metric"
        );

    query.bindValue(":robot_id", robotId);
    query.bindValue(":metric", metric);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        qWarning() << "[IotThresholdRepository] loadThreshold failed:"
                   << m_lastError;
        return fallback;
    }

    if (!query.next()) {
        m_lastError.clear();
        return fallback;
    }

    QVariantMap threshold;
    threshold["normalMax"] = query.value("normal_max").toDouble();
    threshold["warningMax"] = query.value("warning_max").toDouble();
    threshold["alarmMax"] = query.value("alarm_max").toDouble();
    threshold["unit"] = query.value("unit").toString();

    m_lastError.clear();

    return normalizeThreshold(threshold, fallback);
}

QVariantList IotThresholdRepository::loadRobotThresholds(int robotCount,
                                                         const QVariantMap& defaultThreshold)
{
    QVariantList thresholds;

    const QVariantMap defaultTemperature =
        defaultThreshold.value("temperature").toMap();

    const QVariantMap defaultTorque =
        defaultThreshold.value("torque").toMap();

    for (int robotId = 1; robotId <= robotCount; ++robotId) {
        QVariantMap robotThreshold;

        robotThreshold["temperature"] =
            loadThreshold(robotId, "temperature", defaultTemperature);

        robotThreshold["torque"] =
            loadThreshold(robotId, "torque", defaultTorque);

        thresholds.push_back(robotThreshold);
    }

    return thresholds;
}

QString IotThresholdRepository::lastError() const
{
    return m_lastError;
}

bool IotThresholdRepository::isValidMetric(const QString& metric) const
{
    return metric == "temperature" || metric == "torque";
}

QVariantMap IotThresholdRepository::normalizeThreshold(const QVariantMap& threshold,
                                                       const QVariantMap& fallback) const
{
    QVariantMap normalized;

    normalized["normalMax"] =
        threshold.value("normalMax",
                        fallback.value("normalMax", 0.0)).toDouble();

    normalized["warningMax"] =
        threshold.value("warningMax",
                        fallback.value("warningMax", 0.0)).toDouble();

    normalized["alarmMax"] =
        threshold.value("alarmMax",
                        fallback.value("alarmMax", 0.0)).toDouble();

    if (threshold.contains("unit")) {
        normalized["unit"] = threshold.value("unit").toString();
    } else if (fallback.contains("unit")) {
        normalized["unit"] = fallback.value("unit").toString();
    }

    return normalized;
}
