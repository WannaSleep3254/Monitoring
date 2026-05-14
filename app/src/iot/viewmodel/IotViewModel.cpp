#include "IotViewModel.h"

#include "runtime/IRobotGateway.h"

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

    m_robotThresholds[targetIndex] = normalizeThreshold(thresholdData);

    emit robotThresholdsChanged();

    qDebug() << "[IoTViewModel] threshold saved, robot =" << robotIndex;

    return true;
}

void IotViewModel::onSnapshotUpdated(int robotId, QVariantMap snapshot)
{
    qDebug() << "[IoTViewModel] snapshot received robot =" << robotId
             << "seq =" << snapshot.value("sequenceNumber").toULongLong();

    updateRobotModelFromSnapshot(robotId, snapshot);
}

QVariantMap IotViewModel::defaultThreshold() const
{
    QVariantMap temperature;
    temperature["normalMax"] = 55.0;
    temperature["warningMax"] = 60.0;
    temperature["alarmMax"] = 70.0;

    QVariantMap current;
    current["normalMax"] = 12.0;
    current["warningMax"] = 14.0;
    current["alarmMax"] = 16.0;

    QVariantMap threshold;
    threshold["temperature"] = temperature;
    threshold["current"] = current;

    return threshold;
}

QVariantMap IotViewModel::normalizeThreshold(const QVariantMap& thresholdData) const
{
    QVariantMap fallback = defaultThreshold();

    QVariantMap temp = thresholdData.value("temperature").toMap();
    QVariantMap current = thresholdData.value("current").toMap();

    if (current.isEmpty()) {
        current = thresholdData.value("torque").toMap();
    }

    if (temp.isEmpty()) {
        temp = fallback.value("temperature").toMap();
    }

    if (current.isEmpty()) {
        current = fallback.value("current").toMap();
    }

    QVariantMap normalizedTemp;
    normalizedTemp["normalMax"] = temp.value("normalMax", 55.0).toDouble();
    normalizedTemp["warningMax"] = temp.value("warningMax", 60.0).toDouble();
    normalizedTemp["alarmMax"] = temp.value("alarmMax", 70.0).toDouble();

    QVariantMap normalizedCurrent;
    normalizedCurrent["normalMax"] = current.value("normalMax", 12.0).toDouble();
    normalizedCurrent["warningMax"] = current.value("warningMax", 14.0).toDouble();
    normalizedCurrent["alarmMax"] = current.value("alarmMax", 16.0).toDouble();

    QVariantMap normalized;
    normalized["temperature"] = normalizedTemp;
    normalized["current"] = normalizedCurrent;

    return normalized;
}

void IotViewModel::initializeDefaultModels()
{
    auto makeRobot = [](const QString& name) {
        QVariantMap robot;
        robot["name"] = name;
        robot["running"] = false;

        QVariantList tempSeries;
        QVariantList currentSeries;

        for (int i = 0; i < 6; ++i) {
            QVariantMap tempAxis;
            tempAxis["axis"] = QString("J%1").arg(i + 1);
            tempAxis["values"] = QVariantList{0, 0, 0, 0, 0, 0};
            tempSeries.push_back(tempAxis);

            QVariantMap currentAxis;
            currentAxis["axis"] = QString("J%1").arg(i + 1);
            currentAxis["values"] = QVariantList{0, 0, 0, 0, 0, 0};
            currentSeries.push_back(currentAxis);
        }

        robot["tempSeries"] = tempSeries;
        robot["currentSeries"] = currentSeries;
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
    QVariantList currentSeries = robot.value("currentSeries").toList();

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
        currentSeries = pushLatest(currentSeries, torques);
    }

    robot["tempSeries"] = tempSeries;
    robot["currentSeries"] = currentSeries;

    m_robotModels[index] = robot;

    emit robotModelsChanged();
}
