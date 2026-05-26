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
    // Q_PROPERTY는 QML에서 바인딩과 알림을 위해 사용됩니다. 각 프로퍼티는 getter 메서드와 변경 알림 시그널을 정의합니다.
    // robotModels는 로봇별 상태와 예측 정보를 포함하는 모델 리스트입니다. QML에서는 이 리스트를 바인딩하여 대시보드에 표시합니다.
    Q_PROPERTY(QVariantList robotModels
                    READ robotModels
                    NOTIFY robotModelsChanged)
    // 임계값은 robotId별로 저장되며, QML에서는 리스트 형태로 전체 임계값을 관리합니다. 실제로는 robotId를 키로 하는 맵이 내부적으로 관리됩니다.
    Q_PROPERTY(QVariantList robotThresholds
                    READ robotThresholds
                    NOTIFY robotThresholdsChanged)
    // historyRows는 알람과 조치 이력을 포함하는 리스트입니다. QML에서는 이 리스트를 바인딩하여 히스토리 테이블에 표시합니다.
    Q_PROPERTY(QVariantList historyRows
                    READ historyRows
                    NOTIFY historyRowsChanged)
    // lastExportPath는 CSV 내보내기 기능에서 마지막으로 사용된 파일 경로를 저장합니다. QML에서는 이 값을 바인딩하여 내보내기 후 사용자에게 경로를 표시할 수 있습니다.
    Q_PROPERTY(QString lastExportPath
                    READ lastExportPath
                    NOTIFY lastExportPathChanged)
    // lastError는 데이터베이스 작업이나 기타 오류 발생 시 마지막 에러 메시지를 저장합니다. QML에서는 이 값을 바인딩하여 오류 메시지를 사용자에게 표시할 수 있습니다.
    Q_PROPERTY(QString lastError
                    READ lastError
                    NOTIFY lastErrorChanged)
    // lastCleanupSummary는 오래된 히스토리 삭제 작업 후 요약 정보를 저장합니다. QML에서는 이 값을 바인딩하여 삭제 결과를 사용자에게 표시할 수 있습니다.
    Q_PROPERTY(QVariantMap lastCommandResult
                    READ lastCommandResult
                    NOTIFY lastCommandResultChanged)
    // alarmHistoryInsertEnabled는 알람 발생 시 DB 이력 저장 여부를 제어하는 플래그입니다. QML에서는 이 값을 바인딩하여 사용자가 설정을 토글할 수 있습니다.
    Q_PROPERTY(QVariantMap lastCleanupSummary
                    READ lastCleanupSummary
                    NOTIFY lastCleanupSummaryChanged)
    // alarmCooldownSec는 동일한 알람의 반복 저장/표시 억제 시간을 초 단위로 설정하는 프로퍼티입니다. QML에서는 이 값을 바인딩하여 사용자가 조정할 수 있습니다.
    Q_PROPERTY(bool alarmHistoryInsertEnabled
                    READ alarmHistoryInsertEnabled
                    WRITE setAlarmHistoryInsertEnabled
                    NOTIFY alarmHistoryInsertEnabledChanged)
    // alarmCooldownSec는 동일한 알람의 반복 저장/표시 억제 시간을 초 단위로 설정하는 프로퍼티입니다. QML에서는 이 값을 바인딩하여 사용자가 조정할 수 있습니다.
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
    QVariantMap lastCommandResult() const;
    QVariantMap lastCleanupSummary() const;

    Q_INVOKABLE bool saveThreshold(int robotIndex, const QVariantMap& thresholdData);

    QVariantList historyRows() const;
    Q_INVOKABLE bool queryHistory(const QVariantMap& filter);

    QString lastExportPath() const;
    Q_INVOKABLE bool exportHistoryCsv(const QVariantList& rows);

    Q_INVOKABLE bool confirmAlarmAction(const QVariantMap& alarmRow);
    Q_INVOKABLE bool saveAction(const QVariantMap& actionData);
    Q_INVOKABLE bool deleteOldHistoryDays(int retentionDays);



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
    void lastCommandResultChanged();

    void lastCleanupSummaryChanged();

    void alarmHistoryInsertEnabledChanged();
    void alarmCooldownSecChanged();

private slots:
    void onSnapshotUpdated(int robotId, QVariantMap snapshot);
    void onCommandFinished(int robotId,
                           QString command,
                           bool ok,
                           int code,
                           QString message);

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
    QVariantMap m_lastCommandResult;

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
