#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QString>
#include <QHash>
#include <QDateTime>

#include <memory>

class IotDatabase;
class IRobotGateway;

class IotViewModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantList robotModels READ robotModels NOTIFY robotModelsChanged)
    Q_PROPERTY(QVariantList robotThresholds READ robotThresholds NOTIFY robotThresholdsChanged)
    Q_PROPERTY(QVariantList historyRows READ historyRows NOTIFY historyRowsChanged)
    Q_PROPERTY(QString lastExportPath READ lastExportPath NOTIFY lastExportPathChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(QVariantMap lastCleanupSummary READ lastCleanupSummary NOTIFY lastCleanupSummaryChanged)
    Q_PROPERTY(bool alarmHistoryInsertEnabled
                    READ alarmHistoryInsertEnabled
                    WRITE setAlarmHistoryInsertEnabled
                    NOTIFY alarmHistoryInsertEnabledChanged)

    Q_PROPERTY(int alarmCooldownSec
                   READ alarmCooldownSec
                   WRITE setAlarmCooldownSec
                   NOTIFY alarmCooldownSecChanged)

public:
    explicit IotViewModel(QObject* parent = nullptr);

    Q_INVOKABLE bool initialize();

    void setRobotGateway(IRobotGateway* gateway);

    QVariantList robotModels() const;
    QVariantList robotThresholds() const;
    QString lastError() const;

    Q_INVOKABLE bool saveThreshold(int robotIndex, const QVariantMap& thresholdData);

    QVariantList historyRows() const;
    Q_INVOKABLE bool queryHistory(const QVariantMap& filter);

    QString lastExportPath() const;
    Q_INVOKABLE bool exportHistoryCsv(const QVariantList& rows);

    Q_INVOKABLE bool confirmAlarmAction(const QVariantMap& alarmRow);
    Q_INVOKABLE bool saveAction(const QVariantMap& actionData);
    Q_INVOKABLE bool deleteOldHistoryDays(int retentionDays);

    QVariantMap lastCleanupSummary() const;

    bool alarmHistoryInsertEnabled() const;
    void setAlarmHistoryInsertEnabled(bool enabled);

    int alarmCooldownSec() const;
    void setAlarmCooldownSec(int seconds);

public: // Command invokers
    Q_INVOKABLE void setRobotManualMode(int robotId);
    Q_INVOKABLE void setRobotAutoMode(int robotId);
    Q_INVOKABLE void clearRobotError(int robotId);

    Q_INVOKABLE void startRobotJointJog(int robotId, int joint, bool positive);
    Q_INVOKABLE void stopRobotJointJog(int robotId);
    Q_INVOKABLE void sendRobotJogHeartbeat(int robotId);

signals:
    void robotModelsChanged();
    void robotThresholdsChanged();
    void historyRowsChanged();
    void lastExportPathChanged();
    void lastErrorChanged();

    void lastCleanupSummaryChanged();

    void alarmHistoryInsertEnabledChanged();
    void alarmCooldownSecChanged();

private slots:
    void onSnapshotUpdated(int robotId, QVariantMap snapshot);

private:
    QVariantMap defaultThreshold() const;
    QVariantMap normalizeThreshold(const QVariantMap& thresholdData) const;
    void initializeDefaultModels();
    void updateRobotModelFromSnapshot(int robotId, const QVariantMap& snapshot);

    // ============================================================
    // Alarm evaluation
    // ============================================================
    // temperature/torque  임계값 비교
    void evaluateAlarms(int robotId, const QVariantMap& snapshot);
    // 온도와 부하/토크를 공통 처리
    void evaluateMetricAlarms(int robotId,
                              const QString& metric,
                              const QVariantList& values,
                              const QVariantMap& threshold);

    QString alarmLevel(double value,
                       double warningMax,
                       double alarmMax) const;

    QVariantMap thresholdForRobot(int robotId) const;

    QVariantMap makeAlarmMap(int robotId,
                             int axisIndex,
                             const QString& metric,
                             double value,
                             const QVariantMap& threshold,
                             const QString& level) const;

    bool shouldSuppressAlarm(const QVariantMap& alarm,
                             const QDateTime& now) const;

    void rememberAlarm(const QVariantMap& alarm,
                       const QDateTime& now);

    void appendAlarmToRobotModel(int robotId,
                                 const QVariantMap& alarm);

    QString alarmKey(const QVariantMap& alarm) const;

    QVariantMap historyRowFromAlarm(const QVariantMap& alarm) const;
    QVariantMap historyRowFromAction(const QVariantMap& action) const;

    QString historyTimeText(const QString& isoTime) const;
    QString historyDateTimeText(const QString& isoTime) const;
    QString historyStatusText(const QString& level) const;

    QString csvEscape(const QString& value) const;
    QString defaultCsvExportPath() const;


private:
    IRobotGateway* m_gateway = nullptr;

    QVariantList m_robotModels;
    QVariantList m_robotThresholds;
    QString m_lastError;

    std::unique_ptr<IotDatabase> m_database;

    // 동일 알람 반복 저장 방지용
    // 운영 기본값: 동일 robotId + axis + metric + level 알람은 300초 내 중복 저장/표시 억제
    QHash<QString, QDateTime> m_lastAlarmTimes;
    int m_alarmCooldownSec = 300;   //60;

    QVariantList m_historyRows;

    QString m_lastExportPath;

    QVariantMap m_lastCleanupSummary;

    // false: 대시보드에는 알람을 표시하지만 DB 이력에는 자동 저장하지 않음
    // true : 임계값 초과 알람을 DB 이력에도 저장
    bool m_alarmHistoryInsertEnabled = false;
};
