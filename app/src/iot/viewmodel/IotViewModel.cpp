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

    // 현재는 알람 이력만 구현됨
    if (kind == "조치 이력" || kind == "action") {
        m_historyRows.clear();
        emit historyRowsChanged();
        return true;
    }

    IotHistoryRepository repo(m_database->database());

    QVariantMap queryFilter = filter;

    // QML에서 넘어온 kind는 Repository SQL에는 직접 사용하지 않음
    queryFilter.remove("kind");

    const QVariantList alarms = repo.queryAlarms(queryFilter);

    if (!repo.lastError().isEmpty()) {
        m_lastError = repo.lastError();
        emit lastErrorChanged();
        return false;
    }

    QVariantList rows;
    rows.reserve(alarms.size());

    for (const QVariant& item : alarms) {
        rows.push_back(historyRowFromAlarm(item.toMap()));
    }

    m_historyRows = rows;
    emit historyRowsChanged();

    qDebug() << "[IoTViewModel] history queried, rows =" << m_historyRows.size();

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

        QStringList cols;
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

QString IotViewModel::historyTimeText(const QString& isoTime) const
{
    const QDateTime dt = QDateTime::fromString(isoTime, Qt::ISODate);

    if (dt.isValid())
        return dt.toString("HH:mm:ss");

    if (isoTime.size() >= 19)
        return isoTime.mid(11, 8);

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
