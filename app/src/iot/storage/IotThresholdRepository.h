#pragma once

#include <QSqlDatabase>
#include <QVariantMap>
#include <QVariantList>
#include <QString>

class IotThresholdRepository
{
public:
    explicit IotThresholdRepository(const QSqlDatabase& database);

    bool saveThreshold(int robotId,
                       const QString& metric,
                       const QVariantMap& threshold,
                       const QString& unit);

    QVariantMap loadThreshold(int robotId,
                              const QString& metric,
                              const QVariantMap& fallback);

    QVariantList loadRobotThresholds(int robotCount,
                                     const QVariantMap& defaultThreshold);

    QString lastError() const;

private:
    bool isValidMetric(const QString& metric) const;
    QVariantMap normalizeThreshold(const QVariantMap& threshold,
                                   const QVariantMap& fallback) const;

private:
    QSqlDatabase m_db;
    QString m_lastError;
};
