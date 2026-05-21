// tests/zmq_snapshot_dryrun_publisher.cpp

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

namespace
{
QJsonArray makeArray(std::initializer_list<double> values)
{
    QJsonArray array;

    for (double value : values) {
        array.append(value);
    }

    return array;
}

QByteArray buildSnapshot(int robotId, quint64 sequenceNumber)
{
    const double base = static_cast<double>(sequenceNumber % 360);

    QJsonObject snapshot;
    snapshot["messageType"] = QStringLiteral("snapshot");
    snapshot["schemaVersion"] = 1;
    snapshot["robotId"] = robotId;
    snapshot["robotName"] = QStringLiteral("FR%1").arg(robotId);
    snapshot["model"] = robotId == 1 ? QStringLiteral("FR10") : QStringLiteral("FR5");

    snapshot["connected"] = true;
    snapshot["running"] = true;

    snapshot["jointPositions"] =
        makeArray({
            base + robotId,
            base + 10.0,
            base + 20.0,
            base + 30.0,
            base + 40.0,
            base + 50.0
        });

    snapshot["tcpPose"] =
        makeArray({
            100.0 + robotId,
            200.0 + robotId,
            300.0 + robotId,
            180.0,
            0.0,
            90.0
        });

    snapshot["torques"] =
        makeArray({
            10.0 + robotId,
            11.0 + robotId,
            12.0 + robotId,
            13.0 + robotId,
            14.0 + robotId,
            15.0 + robotId
        });

    snapshot["driverTemperatures"] =
        makeArray({
            40.0 + robotId,
            41.0 + robotId,
            42.0 + robotId,
            43.0 + robotId,
            44.0 + robotId,
            45.0 + robotId
        });

    snapshot["robotState"] = QStringLiteral("RUN");
    snapshot["errorText"] = QStringLiteral("0");
    snapshot["sequenceState"] = QStringLiteral("IDLE");
    snapshot["interlockState"] = QStringLiteral("OK");

    snapshot["timestamp"] =
        QDateTime::currentDateTime().toString(Qt::ISODateWithMs);

    snapshot["sequenceNumber"] =
        static_cast<double>(sequenceNumber);

    return QJsonDocument(snapshot).toJson(QJsonDocument::Compact);
}
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

#ifndef ENABLE_ZEROMQ_TRANSPORT
    qCritical() << "[ZmqSnapshotDryRunPublisher]"
                << "ZeroMQ transport is disabled at build time";
    return 1;
#else
    const QString endpoint =
        app.arguments().size() >= 2
            ? app.arguments().at(1)
            : QStringLiteral("tcp://*:5556");

    const int intervalMs =
        app.arguments().size() >= 3
            ? app.arguments().at(2).toInt()
            : 500;

    try {
        zmq::context_t context(1);
        zmq::socket_t socket(context, zmq::socket_type::pub);

        socket.set(zmq::sockopt::linger, 0);
        socket.bind(endpoint.toStdString());

        qInfo() << "[ZmqSnapshotDryRunPublisher] publishing =" << endpoint
                << "intervalMs =" << intervalMs;

        quint64 sequenceNumber = 0;

        while (true) {
            ++sequenceNumber;

            for (int robotId = 1; robotId <= 2; ++robotId) {
                const QString topic =
                    QStringLiteral("robot/%1").arg(robotId);

                const QByteArray payload =
                    buildSnapshot(robotId, sequenceNumber);

                socket.send(
                    zmq::buffer(topic.toUtf8().constData(),
                                static_cast<size_t>(topic.toUtf8().size())),
                    zmq::send_flags::sndmore);

                socket.send(
                    zmq::buffer(payload.constData(),
                                static_cast<size_t>(payload.size())),
                    zmq::send_flags::none);

                qInfo() << "[ZmqSnapshotDryRunPublisher] topic ="
                        << topic
                        << "payload bytes ="
                        << payload.size();
            }

            QThread::msleep(static_cast<unsigned long>(intervalMs));
        }

    } catch (const zmq::error_t& error) {
        qCritical() << "[ZmqSnapshotDryRunPublisher] ZeroMQ error:"
                    << error.what();
        return 2;
    }

    return 0;
#endif
}