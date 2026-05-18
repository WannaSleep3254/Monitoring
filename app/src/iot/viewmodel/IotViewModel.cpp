#include "IotViewModel.h"

#include "runtime/IRobotGateway.h"

#include "iot/storage/IotDatabase.h"
#include "iot/storage/IotSchema.h"
#include "iot/storage/IotThresholdRepository.h"
#include "iot/storage/IotHistoryRepository.h"

#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QStandardPaths>
#include <QDebug>

#include <algorithm>

IotViewModel::IotViewModel(QObject* parent)
    : QObject(parent)
{
    initializeDefaultModels();

    m_robotThresholds = {
        defaultThreshold(),
        defaultThreshold()
    };
}

bool IotViewModel::initialize()
{
    m_database = std::make_unique<IotDatabase>();

    if (!m_database->open()) {
        m_lastError = m_database->lastError();
        emit lastErrorChanged();
        return false;
    }

    IotSchema schema(m_database->database());

    if (!schema.createTables()) {
        m_lastError = schema.lastError();
        emit lastErrorChanged();
        return false;
    }

    IotThresholdRepository repo(m_database->database());

    m_robotThresholds = repo.loadRobotThresholds(2, defaultThreshold());

    emit robotThresholdsChanged();

    qDebug() << "[IoTViewModel] initialized with SQLite";

#if false   // SQLite에 테스트 알람 데이터 삽입 (디버깅용)
    IotHistoryRepository historyRepo(m_database->database());

    QVariantMap testAlarm;
    testAlarm["robotId"] = 1;
    testAlarm["axis"] = "J6";
    testAlarm["metric"] = "temperature";
    testAlarm["value"] = 73.0;
    testAlarm["normalMax"] = 55.0;
    testAlarm["warningMax"] = 60.0;
    testAlarm["alarmMax"] = 70.0;
    testAlarm["level"] = "ALARM";
    testAlarm["message"] = "J6 온도 알람 테스트";
    testAlarm["source"] = "test";

    historyRepo.insertAlarm(testAlarm);
#endif

#if false   // SQLite에 테스트 조치 데이터 삽입 (디버깅용)
    IotHistoryRepository historyRepo(m_database->database());

    QVariantMap testAction;
    testAction["alarmId"] = 2;
    testAction["robotId"] = 1;
    testAction["status"] = "확인중";
    testAction["actionContent"] = "J3 축 부하 상태 확인 시작";
    testAction["operatorName"] = "작업자 A";
    testAction["memo"] = "테스트 조치 이력";

    historyRepo.insertAction(testAction);
#endif

    return true;
}

void IotViewModel::setRobotGateway(IRobotGateway* gateway)
{
    if (m_gateway == gateway)
        return;

    if (m_gateway) {
        disconnect(m_gateway, nullptr, this, nullptr);
    }

    m_gateway = gateway;

    if (!m_gateway)
        return;

    connect(m_gateway,
            &IRobotGateway::snapshotUpdated,
            this,
            &IotViewModel::onSnapshotUpdated);
}

QVariantList IotViewModel::robotModels() const
{
    return m_robotModels;
}

QVariantList IotViewModel::robotThresholds() const
{
    return m_robotThresholds;
}

QString IotViewModel::lastError() const
{
    return m_lastError;
}

bool IotViewModel::saveThreshold(int robotIndex, const QVariantMap& thresholdData)
{
    const int targetIndex = robotIndex - 1;

    if (targetIndex < 0) {
        m_lastError = "Invalid robot index";
        emit lastErrorChanged();
        return false;
    }

    while (m_robotThresholds.size() <= targetIndex) {
        m_robotThresholds.push_back(defaultThreshold());
    }

    const QVariantMap normalized = normalizeThreshold(thresholdData);

    // 1. 메모리 갱신
    m_robotThresholds[targetIndex] = normalized;

    // 2. SQLite 저장
    if (m_database && m_database->isOpen()) {
        IotThresholdRepository repo(m_database->database());

        const bool temperatureOk =
            repo.saveThreshold(robotIndex,
                               "temperature",
                               normalized.value("temperature").toMap(),
                               "°C");

        const bool torqueOk =
            repo.saveThreshold(robotIndex,
                               "torque",
                               normalized.value("torque").toMap(),
                               "raw");

        if (!temperatureOk || !torqueOk) {
            m_lastError = repo.lastError();
            emit lastErrorChanged();

            qWarning() << "[IoTViewModel] threshold SQLite save failed:"
                       << m_lastError;

            return false;
        }
    }

    emit robotThresholdsChanged();

    qDebug() << "[IoTViewModel] threshold saved, robot =" << robotIndex;

    return true;
}

QVariantList IotViewModel::historyRows() const
{
    return m_historyRows;
}

bool IotViewModel::queryHistory(const QVariantMap& filter)
{
    if (!m_database || !m_database->isOpen()) {
        m_lastError = "Database is not open";
        emit lastErrorChanged();
        return false;
    }

    const QString kind = filter.value("kind").toString();

    IotHistoryRepository repo(m_database->database());

    QVariantMap queryFilter = filter;
    queryFilter.remove("kind");

    QVariantList rows;
    QVariantList alarms;
    QVariantList actionsForRows;
    QVariantList actionsForState;

    const bool queryAlarm =
        kind.isEmpty() ||
        kind == "전체" ||
        kind == "알람 이력" ||
        kind == "alarm";

    const bool queryAction =
        kind.isEmpty() ||
        kind == "전체" ||
        kind == "조치 이력" ||
        kind == "action";

    if (queryAlarm) {
        alarms = repo.queryAlarms(queryFilter);

        if (!repo.lastError().isEmpty()) {
            m_lastError = repo.lastError();
            emit lastErrorChanged();
            return false;
        }

        // 알람 row의 최신 조치상태 반영용.
        // 검색어 조건은 제거해서, 알람이 검색 결과에 포함된 경우에도
        // 해당 알람의 최신 조치상태는 정상 반영되도록 한다.
        QVariantMap actionStateFilter = queryFilter;
        actionStateFilter.remove("searchText");
        actionStateFilter["limit"] = 5000;

        actionsForState = repo.queryActions(actionStateFilter);

        if (!repo.lastError().isEmpty()) {
            m_lastError = repo.lastError();
            emit lastErrorChanged();
            return false;
        }
    }

    if (queryAction) {
        actionsForRows = repo.queryActions(queryFilter);

        if (!repo.lastError().isEmpty()) {
            m_lastError = repo.lastError();
            emit lastErrorChanged();
            return false;
        }
    }

    QHash<qint64, QVariantMap> latestActionByAlarmId;

    for (const QVariant& item : actionsForState) {
        const QVariantMap action = item.toMap();
        const qint64 alarmId = action.value("alarmId").toLongLong();

        if (alarmId <= 0)
            continue;

        // queryActions()는 action_at DESC, id DESC 기준이므로
        // 먼저 들어온 항목을 최신 조치로 사용한다.
        if (!latestActionByAlarmId.contains(alarmId)) {
            latestActionByAlarmId.insert(alarmId, action);
        }
    }

    if (queryAlarm) {
        for (const QVariant& item : alarms) {
            const QVariantMap alarm = item.toMap();
            QVariantMap row = historyRowFromAlarm(alarm);

            const qint64 alarmId = alarm.value("id").toLongLong();

            if (latestActionByAlarmId.contains(alarmId)) {
                const QVariantMap latestAction =
                    latestActionByAlarmId.value(alarmId);

                const QString status =
                    latestAction.value("status").toString();

                const QString operatorName =
                    latestAction.value("operatorName").toString();

                const QString actionContent =
                    latestAction.value("actionContent").toString();

                const QString memo =
                    latestAction.value("memo").toString();

                if (!status.isEmpty())
                    row["actionStatus"] = status;

                if (!operatorName.isEmpty())
                    row["operatorName"] = operatorName;

                if (!actionContent.isEmpty())
                    row["actionContent"] = actionContent;

                if (!memo.isEmpty())
                    row["cause"] = memo;

                row["latestActionId"] = latestAction.value("id");
                row["latestActionAt"] = latestAction.value("actionAt");
            }

            rows.push_back(row);
        }
    }

    if (queryAction) {
        for (const QVariant& item : actionsForRows) {
            rows.push_back((item.toMap()));
        }
    }

    std::sort(rows.begin(), rows.end(),
              [](const QVariant& lhs, const QVariant& rhs) {
                  const QVariantMap left = lhs.toMap();
                  const QVariantMap right = rhs.toMap();

                  return left.value("sortTime").toString()
                         > right.value("sortTime").toString();
              });

    m_historyRows = rows;
    emit historyRowsChanged();

    qDebug() << "[IoTViewModel] history queried"
             << "kind =" << kind
             << "rows =" << m_historyRows.size();

    return true;
}

QString IotViewModel::lastExportPath() const
{
    return m_lastExportPath;
}

bool IotViewModel::exportHistoryCsv(const QVariantList& rows)
{
    if (rows.isEmpty()) {
        m_lastError = "No history rows to export";
        emit lastErrorChanged();
        qWarning() << "[IoTViewModel] CSV export failed:" << m_lastError;
        return false;
    }

    const QString filePath = defaultCsvExportPath();

    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = file.errorString();
        emit lastErrorChanged();

        qWarning() << "[IoTViewModel] CSV file open failed:"
                   << filePath
                   << m_lastError;

        return false;
    }

    QTextStream out(&file);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    out.setCodec("UTF-8");
#else
    out.setEncoding(QStringConverter::Utf8);
#endif

    // Excel 한글 깨짐 방지용 UTF-8 BOM
    out << QChar(0xFEFF);

    const QStringList headers = {
        "일시",
        "시간",
        "로봇",
        "구분",
        "축",
        "온도",
        "부하",
        "상태",
        "설명",
        "조치상태",
        "원인",
        "조치자",
        "조치내용",
        "기록방식"
    };

    out << headers.join(",") << "\n";

    for (const QVariant& item : rows) {
        const QVariantMap row = item.toMap();

        QString isoTime = row.value("sortTime").toString();
        if (isoTime.isEmpty())
            isoTime = row.value("occurredAt").toString();

        if (isoTime.isEmpty())
            isoTime = row.value("actionAt").toString();

        QStringList cols;
        cols << csvEscape(historyDateTimeText(isoTime));
        cols << csvEscape(row.value("time").toString());
        cols << csvEscape(row.value("robot").toString());
        cols << csvEscape(row.value("kind").toString());
        cols << csvEscape(row.value("axis").toString());
        cols << csvEscape(row.value("temp").toString());
        cols << csvEscape(row.value("torque").toString());
        cols << csvEscape(row.value("status").toString());
        cols << csvEscape(row.value("desc").toString());
        cols << csvEscape(row.value("actionStatus").toString());
        cols << csvEscape(row.value("cause").toString());
        cols << csvEscape(row.value("operatorName").toString());
        cols << csvEscape(row.value("actionContent").toString());
        cols << csvEscape(row.value("recordMode").toString());

        out << cols.join(",") << "\n";
    }

    file.close();

    m_lastExportPath = filePath;
    emit lastExportPathChanged();

    qDebug() << "[IoTViewModel] history CSV exported:"
             << filePath
             << "rows =" << rows.size();

    return true;
}

bool IotViewModel::confirmAlarmAction(const QVariantMap& alarmRow)
{
    if (!m_database || !m_database->isOpen()) {
        m_lastError = "Database is not open";
        emit lastErrorChanged();
        return false;
    }

    const qint64 alarmId = alarmRow.value("id").toLongLong();
    const int robotId = alarmRow.value("robotId").toInt();
    const QString axis = alarmRow.value("axis").toString();
    const QString desc = alarmRow.value("desc").toString();

    if (alarmId <= 0) {
        m_lastError = QString("Invalid alarm id: %1").arg(alarmId);
        emit lastErrorChanged();

        qWarning() << "[IoTViewModel] confirm alarm failed:"
                   << m_lastError;

        return false;
    }

    if (robotId <= 0) {
        m_lastError = QString("Invalid robot id: %1").arg(robotId);
        emit lastErrorChanged();

        qWarning() << "[IoTViewModel] confirm alarm failed:"
                   << m_lastError;

        return false;
    }

    IotHistoryRepository repo(m_database->database());

    // 동일 알람에 대해 이미 "확인중" 또는 "완료" 상태의 조치가 있는지 확인
    const QVariantList existingActions = repo.loadActionsByAlarmId(alarmId);

    if (!repo.lastError().isEmpty()) {
        m_lastError = repo.lastError();
        emit lastErrorChanged();
        return false;
    }

    for (const QVariant& item : existingActions) {
        const QVariantMap action = item.toMap();
        const QString status = action.value("status").toString();

        if (status == "확인중") {
            qDebug() << "[IoTViewModel] alarm confirm skipped, already confirmed"
                     << "alarmId =" << alarmId;
            return true;
        }

        if (status == "완료") {
            qDebug() << "[IoTViewModel] alarm confirm skipped, already completed"
                     << "alarmId =" << alarmId;
            return true;
        }
    }

    // 신규 확인 조치 이력 생성
    QVariantMap action;
    action["alarmId"] = alarmId;
    action["robotId"] = robotId;
    action["status"] = "확인중";
    action["actionContent"] = axis.isEmpty()
                                  ? "알람 확인"
                                  : QString("%1 알람 확인").arg(axis);
    action["operatorName"] = "작업자";
    action["memo"] = desc;

    if (!repo.insertAction(action)) {
        m_lastError = repo.lastError();
        emit lastErrorChanged();

        qWarning() << "[IoTViewModel] confirm alarm insert failed:"
                   << m_lastError;

        return false;
    }

    qDebug() << "[IoTViewModel] alarm confirmed"
             << "alarmId =" << alarmId
             << "robotId =" << robotId
             << "axis =" << axis;

    return true;
}

bool IotViewModel::saveAction(const QVariantMap& actionData)
{
    if (!m_database || !m_database->isOpen()) {
        m_lastError = "Database is not open";
        emit lastErrorChanged();
        return false;
    }

    const qint64 alarmId = actionData.value("alarmId").toLongLong();
    const int robotId = actionData.value("robotId").toInt();

    QString status = actionData.value("status").toString().trimmed();
    QString actionContent = actionData.value("actionContent").toString().trimmed();
    QString operatorName = actionData.value("operatorName").toString().trimmed();
    QString memo = actionData.value("memo").toString().trimmed();

    if (alarmId <= 0) {
        m_lastError = QString("Invalid alarm id: %1").arg(alarmId);
        emit lastErrorChanged();

        qWarning() << "[IoTViewModel] save action failed:"
                   << m_lastError;

        return false;
    }

    if (robotId <= 0) {
        m_lastError = QString("Invalid robot id: %1").arg(robotId);
        emit lastErrorChanged();

        qWarning() << "[IoTViewModel] save action failed:"
                   << m_lastError;

        return false;
    }

    if (status.isEmpty()) {
        status = "완료";
    }

    if (actionContent.isEmpty()) {
        m_lastError = "Action content is empty";
        emit lastErrorChanged();

        qWarning() << "[IoTViewModel] save action failed:"
                   << m_lastError;

        return false;
    }

    if (operatorName.isEmpty()) {
        operatorName = "작업자";
    }

    IotHistoryRepository repo(m_database->database());

    QVariantMap action;
    action["alarmId"] = alarmId;
    action["robotId"] = robotId;
    action["status"] = status;
    action["actionContent"] = actionContent;
    action["operatorName"] = operatorName;
    action["memo"] = memo;

    if (!repo.insertAction(action)) {
        m_lastError = repo.lastError();
        emit lastErrorChanged();

        qWarning() << "[IoTViewModel] save action insert failed:"
                   << m_lastError;

        return false;
    }

    qDebug() << "[IoTViewModel] action saved"
             << "alarmId =" << alarmId
             << "robotId =" << robotId
             << "status =" << status
             << "operator =" << operatorName;

    return true;
}

QString IotViewModel::csvEscape(const QString& value) const
{
    QString escaped = value;
    escaped.replace("\"", "\"\"");

    if (escaped.contains(",") ||
        escaped.contains("\"") ||
        escaped.contains("\n") ||
        escaped.contains("\r")) {
        escaped = "\"" + escaped + "\"";
    }

    return escaped;
}

QString IotViewModel::defaultCsvExportPath() const
{
    QString baseDir =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    if (baseDir.isEmpty()) {
        baseDir = QDir::homePath() + "/MonitoringApp";
    }

    QDir dir(baseDir);

    if (!dir.exists("exports")) {
        dir.mkpath("exports");
    }

    const QString fileName =
        QString("iot_history_%1.csv")
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));

    return dir.filePath("exports/" + fileName);
}

void IotViewModel::onSnapshotUpdated(int robotId, QVariantMap snapshot)
{
    updateRobotModelFromSnapshot(robotId, snapshot);
    evaluateAlarms(robotId, snapshot);
#if false
    qDebug() << "[IoTViewModel] snapshot received robot =" << robotId
             << "seq =" << snapshot.value("sequenceNumber").toULongLong();
#endif
}

QVariantMap IotViewModel::defaultThreshold() const
{
    QVariantMap temperature;
    temperature["normalMax"] = 55.0;
    temperature["warningMax"] = 60.0;
    temperature["alarmMax"] = 70.0;

    QVariantMap torque;
    torque["normalMax"] = 12.0;
    torque["warningMax"] = 14.0;
    torque["alarmMax"] = 16.0;

    QVariantMap threshold;
    threshold["temperature"] = temperature;
    threshold["torque"] = torque;

    return threshold;
}

QVariantMap IotViewModel::normalizeThreshold(const QVariantMap& thresholdData) const
{
    QVariantMap fallback = defaultThreshold();

    QVariantMap temp = thresholdData.value("temperature").toMap();
    QVariantMap torque = thresholdData.value("torque").toMap();

    if (torque.isEmpty()) {
        torque = fallback.value("torque").toMap();
    }

    if (temp.isEmpty()) {
        temp = fallback.value("temperature").toMap();
    }

    QVariantMap normalizedTemp;
    normalizedTemp["normalMax"] = temp.value("normalMax", 55.0).toDouble();
    normalizedTemp["warningMax"] = temp.value("warningMax", 60.0).toDouble();
    normalizedTemp["alarmMax"] = temp.value("alarmMax", 70.0).toDouble();

    QVariantMap normalizedTorque;
    normalizedTorque["normalMax"] = torque.value("normalMax", 12.0).toDouble();
    normalizedTorque["warningMax"] = torque.value("warningMax", 14.0).toDouble();
    normalizedTorque["alarmMax"] = torque.value("alarmMax", 16.0).toDouble();

    QVariantMap normalized;
    normalized["temperature"] = normalizedTemp;
    normalized["torque"] = normalizedTorque;

    return normalized;
}

void IotViewModel::initializeDefaultModels()
{
    auto makeRobot = [](const QString& name) {
        QVariantMap robot;
        robot["name"] = name;
        robot["running"] = false;

        QVariantList tempSeries;
        QVariantList torqueSeries;

        for (int i = 0; i < 6; ++i) {
            QVariantMap tempAxis;
            tempAxis["axis"] = QString("J%1").arg(i + 1);
            tempAxis["values"] = QVariantList{0, 0, 0, 0, 0, 0};
            tempSeries.push_back(tempAxis);

            QVariantMap torqueAxis;
            torqueAxis["axis"] = QString("J%1").arg(i + 1);
            torqueAxis["values"] = QVariantList{0, 0, 0, 0, 0, 0};
            torqueSeries.push_back(torqueAxis);
        }

        robot["tempSeries"] = tempSeries;
        robot["torqueSeries"] = torqueSeries;
        robot["alarms"] = QVariantList{};

        QVariantMap prediction;
        prediction["modelStatus"] = "대기";
        prediction["riskLevel"] = "정상";
        prediction["score"] = 0;
        prediction["causeText"] = "-";
        prediction["recommendedAction"] = "-";

        robot["prediction"] = prediction;

        return robot;
    };

    m_robotModels = {
        makeRobot("Robot 1 (FR10)"),
        makeRobot("Robot 2 (FR5)")
    };

    emit robotModelsChanged();
}

void IotViewModel::updateRobotModelFromSnapshot(int robotId, const QVariantMap& snapshot)
{
    const int index = robotId - 1;

    if (index < 0 || index >= m_robotModels.size())
        return;

    QVariantMap robot = m_robotModels[index].toMap();

    robot["running"] = snapshot.value("running", false).toBool();

    QVariantList temperatures = snapshot.value("driverTemperatures").toList();
    QVariantList torques = snapshot.value("torques").toList();

    QVariantList tempSeries = robot.value("tempSeries").toList();
    QVariantList torqueSeries = robot.value("torqueSeries").toList();

    auto pushLatest = [](QVariantList series, const QVariantList& values) {
        for (int i = 0; i < series.size() && i < values.size(); ++i) {
            QVariantMap axis = series[i].toMap();
            QVariantList history = axis.value("values").toList();

            while (history.size() >= 6) {
                history.removeFirst();
            }

            history.push_back(values[i].toDouble());

            axis["values"] = history;
            series[i] = axis;
        }

        return series;
    };

    if (!temperatures.isEmpty()) {
        tempSeries = pushLatest(tempSeries, temperatures);
    }

    if (!torques.isEmpty()) {
        torqueSeries = pushLatest(torqueSeries, torques);
    }

    robot["tempSeries"] = tempSeries;
    robot["torqueSeries"] = torqueSeries;

    m_robotModels[index] = robot;

    emit robotModelsChanged();
}

void IotViewModel::evaluateAlarms(int robotId, const QVariantMap& snapshot)
{
    const QVariantMap threshold = thresholdForRobot(robotId);

    const QVariantList temperatures = snapshot.value("driverTemperatures").toList();
    const QVariantList torques = snapshot.value("torques").toList();

    const QVariantMap temperatureThreshold =
        threshold.value("temperature").toMap();

    const QVariantMap torqueThreshold =
        threshold.value("torque").toMap();

    evaluateMetricAlarms(robotId,
                         "temperature",
                         temperatures,
                         temperatureThreshold);

    evaluateMetricAlarms(robotId,
                         "torque",
                         torques,
                         torqueThreshold);
}

void IotViewModel::evaluateMetricAlarms(int robotId,
                                        const QString& metric,
                                        const QVariantList& values,
                                        const QVariantMap& threshold)
{
    if (values.isEmpty())
        return;

    if (threshold.isEmpty())
        return;

    const double normalMax =
        threshold.value("normalMax", 0.0).toDouble();

    const double warningMax =
        threshold.value("warningMax", 0.0).toDouble();

    const double alarmMax =
        threshold.value("alarmMax", 0.0).toDouble();

    Q_UNUSED(normalMax)

    const QDateTime now = QDateTime::currentDateTime();

    for (int i = 0; i < values.size(); ++i) {
        const double value = values[i].toDouble();

        const QString level = alarmLevel(value, warningMax, alarmMax);

        if (level == "NORMAL")
            continue;

        const QVariantMap alarm =
            makeAlarmMap(robotId,
                         i,
                         metric,
                         value,
                         threshold,
                         level);

        if (shouldSuppressAlarm(alarm, now))
            continue;

        if (m_database && m_database->isOpen()) {
            IotHistoryRepository repo(m_database->database());

            if (!repo.insertAlarm(alarm)) {
                m_lastError = repo.lastError();
                emit lastErrorChanged();

                qWarning() << "[IoTViewModel] alarm insert failed:"
                           << m_lastError;

                continue;
            }
        }

        rememberAlarm(alarm, now);
        appendAlarmToRobotModel(robotId, alarm);
    }
}

QString IotViewModel::alarmLevel(double value,
                                 double warningMax,
                                 double alarmMax) const
{
    if (value >= alarmMax)
        return "ALARM";

    if (value >= warningMax)
        return "WARNING";

    return "NORMAL";
}

QVariantMap IotViewModel::thresholdForRobot(int robotId) const
{
    const int index = robotId - 1;

    if (index < 0 || index >= m_robotThresholds.size())
        return defaultThreshold();

    QVariantMap threshold = m_robotThresholds[index].toMap();

    if (threshold.isEmpty())
        return defaultThreshold();

    return normalizeThreshold(threshold);
}

QVariantMap IotViewModel::makeAlarmMap(int robotId,
                                       int axisIndex,
                                       const QString& metric,
                                       double value,
                                       const QVariantMap& threshold,
                                       const QString& level) const
{
    const QString axis = QString("J%1").arg(axisIndex + 1);

    const double normalMax =
        threshold.value("normalMax", 0.0).toDouble();

    const double warningMax =
        threshold.value("warningMax", 0.0).toDouble();

    const double alarmMax =
        threshold.value("alarmMax", 0.0).toDouble();

    const QDateTime now = QDateTime::currentDateTime();
    const QString occurredAt = now.toString(Qt::ISODate);
    const QString displayTime = now.toString("HH:mm:ss");

    QString metricText;
    QString unitText;

    if (metric == "temperature") {
        metricText = "온도";
        unitText = "°C";
    } else if (metric == "torque") {
        metricText = "부하";
        unitText = "raw";
    } else {
        metricText = metric;
        unitText = "";
    }

    QString levelText;

    if (level == "ALARM") {
        levelText = "경고";
    } else if (level == "WARNING") {
        levelText = "주의";
    } else {
        levelText = "정상";
    }

    const QString message =
        QString("%1 %2 %3 %4 발생: %5%6")
            .arg(axis)
            .arg(metricText)
            .arg(levelText)
            .arg(level)
            .arg(value, 0, 'f', 1)
            .arg(unitText.isEmpty() ? "" : " " + unitText);

    QVariantMap alarm;
//    alarm["occurredAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    alarm["occurredAt"] = occurredAt;
    alarm["time"] = displayTime;
    alarm["robotId"] = robotId;
    alarm["robotName"] = QString("Robot %1").arg(robotId);
    alarm["axis"] = axis;
    alarm["metric"] = metric;
    alarm["metricText"] = metricText;
    alarm["value"] = value;
    alarm["normalMax"] = normalMax;
    alarm["warningMax"] = warningMax;
    alarm["alarmMax"] = alarmMax;
    alarm["level"] = level;
    alarm["levelText"] = levelText;
    alarm["message"] = message;
    alarm["source"] = "auto";
    alarm["unit"] = unitText;

    return alarm;
}

bool IotViewModel::shouldSuppressAlarm(const QVariantMap& alarm,
                                       const QDateTime& now) const
{
    const QString key = alarmKey(alarm);

    if (!m_lastAlarmTimes.contains(key))
        return false;

    const QDateTime lastTime = m_lastAlarmTimes.value(key);

    if (!lastTime.isValid())
        return false;

    const qint64 elapsedSec = lastTime.secsTo(now);

    return elapsedSec >= 0 && elapsedSec < m_alarmCooldownSec;
}

void IotViewModel::rememberAlarm(const QVariantMap& alarm,
                                 const QDateTime& now)
{
    const QString key = alarmKey(alarm);
    m_lastAlarmTimes[key] = now;
}

void IotViewModel::appendAlarmToRobotModel(int robotId,
                                           const QVariantMap& alarm)
{
    const int index = robotId - 1;

    if (index < 0 || index >= m_robotModels.size())
        return;

    QVariantMap robot = m_robotModels[index].toMap();

    QVariantList alarms = robot.value("alarms").toList();

    alarms.insert(0, alarm);

    while (alarms.size() > 20) {
        alarms.removeLast();
    }

    robot["alarms"] = alarms;

    QVariantMap prediction = robot.value("prediction").toMap();

    const QString level = alarm.value("level").toString();

    if (level == "ALARM") {
        prediction["modelStatus"] = "알람";
        prediction["riskLevel"] = "경고";
        prediction["score"] = 100;
        prediction["causeText"] = alarm.value("message").toString();
        prediction["recommendedAction"] = "즉시 축 상태 확인 및 점검 필요";
    } else if (level == "WARNING") {
        prediction["modelStatus"] = "주의";
        prediction["riskLevel"] = "주의";
        prediction["score"] = 60;
        prediction["causeText"] = alarm.value("message").toString();
        prediction["recommendedAction"] = "추세 확인 및 부하/온도 상태 점검";
    }

    robot["prediction"] = prediction;

    m_robotModels[index] = robot;

    emit robotModelsChanged();
}

QString IotViewModel::alarmKey(const QVariantMap& alarm) const
{
    return QString("%1|%2|%3|%4")
        .arg(alarm.value("robotId").toInt())
        .arg(alarm.value("axis").toString(),
             alarm.value("metric").toString(),
             alarm.value("level").toString());
}

QVariantMap IotViewModel::historyRowFromAlarm(const QVariantMap& alarm) const
{
    const QString metric = alarm.value("metric").toString();
    const QString level = alarm.value("level").toString();
    const QString occurredAt = alarm.value("occurredAt").toString();

    const double value = alarm.value("value").toDouble();

    QVariantMap row;

    row["id"] = alarm.value("id");
    row["time"] = historyTimeText(occurredAt);
    row["occurredAt"] = occurredAt;
    row["sortTime"] = occurredAt;

    row["robot"] = QString("Robot %1").arg(alarm.value("robotId").toInt());
    row["robotId"] = alarm.value("robotId").toInt();

    row["kind"] = "알람";
    row["axis"] = alarm.value("axis").toString();

    row["temp"] = metric == "temperature"
                      ? QString::number(value, 'f', 1)
                      : "-";

    row["torque"] = metric == "torque"
                        ? QString::number(value, 'f', 1)
                        : "-";

    row["status"] = historyStatusText(level);
    row["level"] = level;

    row["desc"] = alarm.value("message").toString();
    row["actionStatus"] = "요청";
    row["cause"] = alarm.value("message").toString();
    row["operatorName"] = "시스템";
    row["actionContent"] = level == "ALARM"
                               ? "즉시 축 상태 확인 및 점검 필요"
                               : "추세 확인 및 부하/온도 상태 점검";

    row["recordMode"] = "GUI 자동 기록";
    row["source"] = alarm.value("source").toString();

    return row;
}


QVariantMap IotViewModel::historyRowFromAction(const QVariantMap& action) const
{
    const QString actionAt = action.value("actionAt").toString();

    QVariantMap row;

    row["id"] = action.value("id");
    row["alarmId"] = action.value("alarmId");

    row["time"] = historyTimeText(actionAt);
    row["actionAt"] = actionAt;
    row["sortTime"] = actionAt;

    const int robotId = action.value("robotId").toInt();

    row["robot"] = QString("Robot %1").arg(robotId);
    row["robotId"] = robotId;

    row["kind"] = "조치";
    row["axis"] = "-";
    row["temp"] = "-";
    row["torque"] = "-";

    row["status"] = "조치"; //"정상";
    row["desc"] = action.value("actionContent").toString();

    row["actionStatus"] = action.value("status").toString();
    row["cause"] = action.value("memo").toString().isEmpty()
                       ? "-"
                       : action.value("memo").toString();

    row["operatorName"] = action.value("operatorName").toString().isEmpty()
                              ? "-"
                              : action.value("operatorName").toString();

    row["actionContent"] = action.value("actionContent").toString();
    row["recordMode"] = "작업자 수동 입력";

    return row;
}

QString IotViewModel::historyTimeText(const QString& isoTime) const
{
    const QDateTime dt = QDateTime::fromString(isoTime, Qt::ISODate);

    if (dt.isValid())
        return dt.toString("HH:mm:ss");

    if (isoTime.size() >= 19)
        return isoTime.mid(11, 8);

    return "-";
}

QString IotViewModel::historyDateTimeText(const QString& isoTime) const
{
    const QDateTime dt = QDateTime::fromString(isoTime, Qt::ISODate);

    if (dt.isValid())
        return dt.toString("yyyy-MM-dd HH:mm:ss");

    if (isoTime.size() >= 19)
        return isoTime.left(10) + " " + isoTime.mid(11, 8);

    return "-";
}

QString IotViewModel::historyStatusText(const QString& level) const
{
    if (level == "ALARM")
        return "경고";

    if (level == "WARNING" || level == "WARN")
        return "주의";

    return "정상";
}

