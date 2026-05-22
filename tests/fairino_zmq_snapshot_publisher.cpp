// tests/fairino_zmq_snapshot_publisher.cpp

#include "monitoring/FairinoMonitorService.h"
#include "monitoring/RobotSnapshot.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>

#ifdef ENABLE_ZEROMQ_TRANSPORT
#include <zmq.hpp>
#endif

#include <algorithm>
#include <cmath>
#include <array>
#include <atomic>
#include <csignal>
#include <initializer_list>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace
{
std::atomic_bool g_running { true };

void handleSignal(int)
{
    g_running = false;
}

template <typename T, std::size_t N>
QJsonArray toJsonArray(const std::array<T, N>& values)
{
    QJsonArray array;

    for (const auto& value : values) {
        array.append(static_cast<double>(value));
    }

    return array;
}


template <typename T, std::size_t N>
double maxAbsArray(const std::array<T, N>& values)
{
    double maxValue = 0.0;

    for (const auto& value : values) {
        maxValue = std::max(maxValue, std::abs(static_cast<double>(value)));
    }

    return maxValue;
}

template <typename T, std::size_t N>
double maxArray(const std::array<T, N>& values)
{
    double maxValue = static_cast<double>(values.front());

    for (const auto& value : values) {
        maxValue = std::max(maxValue, static_cast<double>(value));
    }

    return maxValue;
}

QString toIsoTimestamp(const std::chrono::system_clock::time_point& timestamp)
{
    const auto millis =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            timestamp.time_since_epoch()).count();

    return QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(millis))
        .toString(Qt::ISODateWithMs);
}

QByteArray buildSnapshotPayload(int robotId,
                                const monitoring::FairinoMonitorService::SnapshotWithMeta& data)
{
    const monitoring::RobotSnapshot& s = data.snapshot;

    QJsonObject snapshot;
    snapshot["messageType"] = QStringLiteral("snapshot");
    snapshot["schemaVersion"] = 1;

    snapshot["robotId"] = robotId;
    snapshot["robotName"] = QStringLiteral("FR%1").arg(robotId);
    snapshot["model"] = robotId == 1
                            ? QStringLiteral("FR10")
                            : QStringLiteral("FR5");

    snapshot["connected"] = s.connected;
    snapshot["running"] = s.motion_done == 0 ? true : false;

    snapshot["jointPositions"] = toJsonArray(s.joint_pos_deg);
    snapshot["tcpPose"] = toJsonArray(s.tcp_pose);
//    snapshot["torques"] = toJsonArray(s.joint_torque);
    snapshot["torques"] =
        s.driver_torque_valid
            ? toJsonArray(s.driver_torque)
            : toJsonArray(s.joint_torque);

    snapshot["torqueSource"] =
        s.driver_torque_valid
            ? QStringLiteral("driver_torque")
            : QStringLiteral("joint_torque");

    snapshot["driverTemperatures"] = toJsonArray(s.driver_temperature);

    snapshot["robotState"] =
        s.emergency_stop ? QStringLiteral("ESTOP")
                         : QStringLiteral("RUN");

    snapshot["errorText"] =
        QString::fromStdString(s.last_error.empty()
                                   ? s.last_poll_error
                                   : s.last_error);

    if (snapshot["errorText"].toString().isEmpty()) {
        snapshot["errorText"] = QStringLiteral("0");
    }

    snapshot["sequenceState"] = QStringLiteral("SDK");
    snapshot["interlockState"] =
        s.emergency_stop ? QStringLiteral("NG") : QStringLiteral("OK");

    snapshot["timestamp"] = toIsoTimestamp(data.timestamp);
    snapshot["sequenceNumber"] =
        static_cast<double>(data.sequence_number);

    return QJsonDocument(snapshot).toJson(QJsonDocument::Compact);
}

void logSnapshotDiagnostics(
    int robotId,
    const monitoring::FairinoMonitorService::SnapshotWithMeta& data)
{
    const monitoring::RobotSnapshot& s = data.snapshot;

    // 로그 과다 방지: 약 1초 주기
    if (data.sequence_number % 5 != 0) {
        return;
    }

    qInfo() << "[FairinoZmqPublisher][Diag]"
            << "robotId =" << robotId
            << "seq =" << static_cast<qulonglong>(data.sequence_number)
            << "connected =" << s.connected
            << "motion_done =" << s.motion_done
            << "sdk_com_state =" << s.sdk_com_state
            << "emergency_stop =" << s.emergency_stop
            << "temperature_valid =" << s.temperature_valid
            << "driver_torque_valid =" << s.driver_torque_valid
            << "max_joint_torque =" << maxAbsArray(s.joint_torque)
            << "max_driver_torque =" << maxAbsArray(s.driver_torque)
            << "max_driver_temperature =" << maxArray(s.driver_temperature)
            << "last_temp_error =" << s.last_temperature_error
            << "last_driver_torque_error =" << s.last_driver_torque_error
            << "last_poll_error_code =" << s.last_poll_error_code
            << "last_poll_error ="
            << QString::fromStdString(s.last_poll_error);
}

bool publishSnapshot(zmq::socket_t& socket,
                     int robotId,
                     const QByteArray& payload)
{
    const QString topic = QStringLiteral("robot/%1").arg(robotId);
    const QByteArray topicPayload = topic.toUtf8();

    const auto topicResult =
        socket.send(zmq::buffer(topicPayload.constData(),
                                static_cast<size_t>(topicPayload.size())),
                    zmq::send_flags::sndmore);

    if (!topicResult.has_value()) {
        qWarning() << "[FairinoZmqPublisher] topic send failed"
                   << "robotId =" << robotId;
        return false;
    }

    const auto payloadResult =
        socket.send(zmq::buffer(payload.constData(),
                                static_cast<size_t>(payload.size())),
                    zmq::send_flags::none);

    if (!payloadResult.has_value()) {
        qWarning() << "[FairinoZmqPublisher] payload send failed"
                   << "robotId =" << robotId;
        return false;
    }

    return true;
}
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

#ifndef ENABLE_ZEROMQ_TRANSPORT
    qCritical() << "[FairinoZmqPublisher]"
                << "ZeroMQ transport is disabled at build time";
    return 1;
#else
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    const QString endpoint =
        app.arguments().size() >= 2
            ? app.arguments().at(1)
            : QStringLiteral("tcp://*:5556");

    const QString robot1Ip =
        app.arguments().size() >= 3
            ? app.arguments().at(2)
            : QStringLiteral("192.168.57.121");

    const QString robot2Ip =
        app.arguments().size() >= 4
            ? app.arguments().at(3)
            : QStringLiteral("192.168.57.122");

    qInfo() << "[FairinoZmqPublisher] endpoint =" << endpoint;
    qInfo() << "[FairinoZmqPublisher] robot1Ip =" << robot1Ip;
    qInfo() << "[FairinoZmqPublisher] robot2Ip =" << robot2Ip;

    try {
        zmq::context_t context(1);
        zmq::socket_t socket(context, zmq::socket_type::pub);

        socket.set(zmq::sockopt::linger, 0);
        socket.bind(endpoint.toStdString());

        std::mutex publishMutex;

        monitoring::FairinoMonitorService robot1;
        monitoring::FairinoMonitorService robot2;

        monitoring::FairinoMonitorService::Options opt1;
        opt1.robot_id = 1;
        opt1.poll_period_ms = 200;
        opt1.enable_reconnect = true;

        monitoring::FairinoMonitorService::Options opt2;
        opt2.robot_id = 2;
        opt2.poll_period_ms = 200;
        opt2.enable_reconnect = true;

        robot1.setCallback(
            [&](const monitoring::FairinoMonitorService::SnapshotWithMeta& data) {
                logSnapshotDiagnostics(1, data);

                const QByteArray payload = buildSnapshotPayload(1, data);

                std::lock_guard<std::mutex> lock(publishMutex);
                publishSnapshot(socket, 1, payload);
            });

        robot2.setCallback(
            [&](const monitoring::FairinoMonitorService::SnapshotWithMeta& data) {
                logSnapshotDiagnostics(2, data);

                const QByteArray payload = buildSnapshotPayload(2, data);

                std::lock_guard<std::mutex> lock(publishMutex);
                publishSnapshot(socket, 2, payload);
            });

        const bool robot1Started =
            robot1.start(robot1Ip.toStdString(), opt1);

        const bool robot2Started =
            robot2.start(robot2Ip.toStdString(), opt2);

        qInfo() << "[FairinoZmqPublisher] robot1Started =" << robot1Started;
        qInfo() << "[FairinoZmqPublisher] robot2Started =" << robot2Started;

        if (!robot1Started && !robot2Started) {
            qCritical() << "[FairinoZmqPublisher]"
                        << "all robot connections failed";
            return 2;
        }

        qInfo() << "[FairinoZmqPublisher] publishing started";

        while (g_running) {
            QThread::msleep(100);
        }

        qInfo() << "[FairinoZmqPublisher] stopping...";

        robot1.stop();
        robot2.stop();

        socket.close();
        context.shutdown();
        context.close();

        qInfo() << "[FairinoZmqPublisher] stopped";
        return 0;

    } catch (const zmq::error_t& error) {
        qCritical() << "[FairinoZmqPublisher] ZeroMQ error:"
                    << error.what();
        return 3;
    }
#endif
}
