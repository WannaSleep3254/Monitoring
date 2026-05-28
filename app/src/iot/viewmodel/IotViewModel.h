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
    // ============================================================
    // QML-exposed properties
    // ============================================================
    // Q_PROPERTY는 QML 바인딩을 위한 공개 상태입니다.
    // 각 프로퍼티는 getter와 NOTIFY signal을 통해 QML 화면에 상태 변화를 전달합니다.

    // 로봇별 실시간 상태, 온도/부하 그래프, 알람 정보를 포함하는 모델 리스트입니다.
    // RobotPanel / IoTPanel 등 QML 화면에서 로봇 상태 표시용으로 사용합니다.
    Q_PROPERTY(QVariantList robotModels
                    READ robotModels
                    NOTIFY robotModelsChanged)
    // 로봇별 온도/부하 임계값 리스트입니다.
    // 내부적으로는 robotId 기준으로 관리되며, QML에서는 임계값 설정 화면에 바인딩합니다.
    Q_PROPERTY(QVariantList robotThresholds
                    READ robotThresholds
                    NOTIFY robotThresholdsChanged)
    // 알람 및 작업자 조치 이력 조회 결과입니다.
    // QML의 이력 조회 테이블에 표시됩니다.
    Q_PROPERTY(QVariantList historyRows
                    READ historyRows
                    NOTIFY historyRowsChanged)
    // CSV 내보내기 후 마지막으로 생성된 파일 경로입니다.
    // QML에서 내보내기 완료 메시지 또는 파일 경로 표시용으로 사용합니다.
    Q_PROPERTY(QString lastExportPath
                    READ lastExportPath
                    NOTIFY lastExportPathChanged)
    // 최근 오류 메시지입니다.
    // DB 처리 실패, command 실패 등 사용자에게 표시할 마지막 오류 상태를 저장합니다.
    Q_PROPERTY(QString lastError
                    READ lastError
                    NOTIFY lastErrorChanged)
    // 마지막 원격 명령 실행 결과입니다.
    // RobotPanel의 "최근 명령 결과" 영역에 바인딩됩니다.
    // 예: robotId, command, ok, code, message, timestamp
    Q_PROPERTY(QVariantMap lastCommandResult
                    READ lastCommandResult
                    NOTIFY lastCommandResultChanged)
    // 오래된 히스토리 삭제 작업의 마지막 결과 요약입니다.
    // 예: 삭제 대상 기간, 삭제 건수, 성공 여부 등을 QML에 표시할 때 사용합니다.
    Q_PROPERTY(QVariantMap lastCleanupSummary
                    READ lastCleanupSummary
                    NOTIFY lastCleanupSummaryChanged)
    // 알람 발생 시 DB 이력 자동 저장 여부입니다.
    // false이면 화면에는 알람을 표시하지만 DB에는 자동 저장하지 않습니다.
    Q_PROPERTY(bool alarmHistoryInsertEnabled
                    READ alarmHistoryInsertEnabled
                    WRITE setAlarmHistoryInsertEnabled
                    NOTIFY alarmHistoryInsertEnabledChanged)
    // 동일 알람의 반복 저장/표시 억제 시간입니다.
    // robotId + axis + metric + level 기준으로 cooldown 시간 내 중복 알람을 억제합니다.
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
    // snapshot에 포함된 온도/부하 값을 임계값과 비교하여 알람을 평가합니다.
    void evaluateAlarms(int robotId, const QVariantMap& snapshot);
    // temperature, torque 등 축별 metric 값을 공통 로직으로 평가합니다.
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
    bool m_alarmHistoryInsertEnabled = true;
};
