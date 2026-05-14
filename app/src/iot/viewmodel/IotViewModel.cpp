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
    updateRobotModelFromSnapshot(robotId, snapshot);
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
        torque = thresholdData.value("torque").toMap();
    }

    if (temp.isEmpty()) {
        temp = fallback.value("temperature").toMap();
    }

    if (torque.isEmpty()) {
        torque = fallback.value("torque").toMap();
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
