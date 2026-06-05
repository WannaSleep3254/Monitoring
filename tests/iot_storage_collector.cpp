// tests/iot_storage_collector.cpp

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QString>

#ifdef ENABLE_ZEROMQ_TRANSPORT
#include <zmq.hpp>
#endif

#include <atomic>
#include <csignal>

namespace
{
std::atomic_bool g_running { true };

void handleSignal(int)
{
    g_running = false;
}

QString defaultDatabasePath()
{
    const QString dataDir =
        QDir::current().absoluteFilePath(QStringLiteral("data"));

    QDir().mkpath(dataDir);

    const QString fileName =
        QStringLiteral("iot_storage_%1.db")
            .arg(QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd")));

    return QDir(dataDir).absoluteFilePath(fileName);
}

bool ensureParentDirectory(const QString& filePath)
{
    const QFileInfo info(filePath);
    const QString parentPath = info.absolutePath();

    if (parentPath.isEmpty() || parentPath == QStringLiteral(".")) {
        return true;
    }

    return QDir().mkpath(parentPath);
}

bool execSql(QSqlQuery& query, const QString& sql)
{
    if (query.exec(sql)) {
        return true;
    }

    qCritical() << "[IoTStorageCollector] SQL failed"
                << query.lastError().text()
                << "sql =" << sql;
    return false;
}

bool initializeSchema(QSqlDatabase& db)
{
    QSqlQuery query(db);

    if (!execSql(query, QStringLiteral("PRAGMA busy_timeout = 5000"))) {
        return false;
    }

    execSql(query, QStringLiteral("PRAGMA journal_mode = WAL"));
    execSql(query, QStringLiteral("PRAGMA synchronous = NORMAL"));

    return execSql(
        query,
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS raw_messages ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "received_epoch_ms INTEGER NOT NULL,"
            "received_at TEXT NOT NULL,"
            "source TEXT NOT NULL,"
            "topic TEXT NOT NULL,"
            "robot_id INTEGER,"
            "message_type TEXT,"
            "schema_version INTEGER,"
            "sequence_number INTEGER,"
            "payload_bytes INTEGER NOT NULL,"
            "raw_json TEXT NOT NULL"
            ")")) &&
        execSql(
            query,
            QStringLiteral(
                "CREATE INDEX IF NOT EXISTS idx_raw_messages_received "
                "ON raw_messages(received_epoch_ms)")) &&
        execSql(
            query,
            QStringLiteral(
                "CREATE INDEX IF NOT EXISTS idx_raw_messages_topic_time "
                "ON raw_messages(topic, received_epoch_ms)")) &&
        execSql(
            query,
            QStringLiteral(
                "CREATE INDEX IF NOT EXISTS idx_raw_messages_robot_time "
                "ON raw_messages(robot_id, received_epoch_ms)")) &&
        execSql(
            query,
            QStringLiteral(
                "CREATE TABLE IF NOT EXISTS metric_samples ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                "raw_message_id INTEGER NOT NULL,"
                "sample_epoch_ms INTEGER NOT NULL,"
                "sample_at TEXT NOT NULL,"
                "received_epoch_ms INTEGER NOT NULL,"
                "topic TEXT NOT NULL,"
                "robot_id INTEGER NOT NULL,"
                "sequence_number INTEGER,"
                "metric_name TEXT NOT NULL,"
                "axis TEXT NOT NULL DEFAULT '',"
                "value_number REAL,"
                "value_text TEXT,"
                "source TEXT NOT NULL,"
                "FOREIGN KEY(raw_message_id) REFERENCES raw_messages(id)"
                ")")) &&
        execSql(
            query,
            QStringLiteral(
                "CREATE INDEX IF NOT EXISTS idx_metric_samples_metric_time "
                "ON metric_samples(metric_name, robot_id, sample_epoch_ms)")) &&
        execSql(
            query,
            QStringLiteral(
                "CREATE INDEX IF NOT EXISTS idx_metric_samples_robot_time "
                "ON metric_samples(robot_id, sample_epoch_ms)")) &&
        execSql(
            query,
            QStringLiteral(
                "CREATE INDEX IF NOT EXISTS idx_metric_samples_raw "
                "ON metric_samples(raw_message_id)"));
}

qint64 timestampEpochMs(const QJsonObject& object, qint64 fallbackEpochMs)
{
    const QJsonValue value = object.value(QStringLiteral("timestampEpochMs"));
    if (value.isDouble()) {
        return static_cast<qint64>(value.toDouble());
    }

    const QString timestamp = object.value(QStringLiteral("timestamp")).toString();
    if (!timestamp.isEmpty()) {
        const QDateTime parsed =
            QDateTime::fromString(timestamp, Qt::ISODateWithMs);
        if (parsed.isValid()) {
            return parsed.toMSecsSinceEpoch();
        }
    }

    return fallbackEpochMs;
}

bool insertMetric(QSqlDatabase& db,
                  qint64 rawMessageId,
                  qint64 sampleEpochMs,
                  const QString& sampleAt,
                  qint64 receivedEpochMs,
                  const QString& topic,
                  int robotId,
                  qint64 sequenceNumber,
                  const QString& metricName,
                  const QString& axis,
                  const QVariant& valueNumber,
                  const QVariant& valueText,
                  const QString& source)
{
    QSqlQuery query(db);
    query.prepare(
        QStringLiteral(
            "INSERT INTO metric_samples ("
            "raw_message_id,"
            "sample_epoch_ms,"
            "sample_at,"
            "received_epoch_ms,"
            "topic,"
            "robot_id,"
            "sequence_number,"
            "metric_name,"
            "axis,"
            "value_number,"
            "value_text,"
            "source"
            ") VALUES ("
            ":raw_message_id,"
            ":sample_epoch_ms,"
            ":sample_at,"
            ":received_epoch_ms,"
            ":topic,"
            ":robot_id,"
            ":sequence_number,"
            ":metric_name,"
            ":axis,"
            ":value_number,"
            ":value_text,"
            ":source"
            ")"));

    query.bindValue(QStringLiteral(":raw_message_id"), rawMessageId);
    query.bindValue(QStringLiteral(":sample_epoch_ms"), sampleEpochMs);
    query.bindValue(QStringLiteral(":sample_at"), sampleAt);
    query.bindValue(QStringLiteral(":received_epoch_ms"), receivedEpochMs);
    query.bindValue(QStringLiteral(":topic"), topic);
    query.bindValue(QStringLiteral(":robot_id"), robotId);
    query.bindValue(QStringLiteral(":sequence_number"), sequenceNumber);
    query.bindValue(QStringLiteral(":metric_name"), metricName);
    query.bindValue(QStringLiteral(":axis"),
                    axis.isNull() ? QStringLiteral("") : axis);
    query.bindValue(QStringLiteral(":value_number"), valueNumber);
    query.bindValue(QStringLiteral(":value_text"), valueText);
    query.bindValue(QStringLiteral(":source"), source);

    if (!query.exec()) {
        qWarning() << "[IoTStorageCollector] metric insert failed"
                   << query.lastError().text()
                   << "metric =" << metricName
                   << "axis =" << axis;
        return false;
    }

    query.finish();
    return true;
}

int insertArrayMetrics(QSqlDatabase& db,
                       qint64 rawMessageId,
                       qint64 sampleEpochMs,
                       const QString& sampleAt,
                       qint64 receivedEpochMs,
                       const QString& topic,
                       int robotId,
                       qint64 sequenceNumber,
                       const QJsonObject& object,
                       const QString& jsonField,
                       const QString& metricName,
                       const QStringList& axes,
                       const QString& source)
{
    const QJsonArray values = object.value(jsonField).toArray();
    int inserted = 0;

    const int count = std::min(values.size(), axes.size());
    for (int i = 0; i < count; ++i) {
        const QJsonValue value = values.at(i);
        if (!value.isDouble()) {
            continue;
        }

        if (insertMetric(db,
                         rawMessageId,
                         sampleEpochMs,
                         sampleAt,
                         receivedEpochMs,
                         topic,
                         robotId,
                         sequenceNumber,
                         metricName,
                         axes.at(i),
                         value.toDouble(),
                         QVariant(QVariant::String),
                         source)) {
            ++inserted;
        }
    }

    return inserted;
}

int insertSnapshotMetrics(QSqlDatabase& db,
                          qint64 rawMessageId,
                          qint64 receivedEpochMs,
                          const QString& topic,
                          const QJsonObject& object)
{
    const int robotId = object.value(QStringLiteral("robotId")).toInt(0);
    if (robotId <= 0) {
        return 0;
    }

    const qint64 sequenceNumber =
        static_cast<qint64>(
            object.value(QStringLiteral("sequenceNumber")).toDouble(0.0));
    const qint64 sampleEpochMs = timestampEpochMs(object, receivedEpochMs);
    const QString sampleAt =
        QDateTime::fromMSecsSinceEpoch(sampleEpochMs)
            .toString(Qt::ISODateWithMs);
    const QString source = QStringLiteral("snapshot");

    int inserted = 0;
    const QStringList jointAxes = {
        QStringLiteral("j1"), QStringLiteral("j2"), QStringLiteral("j3"),
        QStringLiteral("j4"), QStringLiteral("j5"), QStringLiteral("j6")
    };
    const QStringList poseAxes = {
        QStringLiteral("x"), QStringLiteral("y"), QStringLiteral("z"),
        QStringLiteral("rx"), QStringLiteral("ry"), QStringLiteral("rz")
    };

    inserted += insertArrayMetrics(db, rawMessageId, sampleEpochMs, sampleAt,
                                   receivedEpochMs, topic, robotId,
                                   sequenceNumber, object,
                                   QStringLiteral("jointPositions"),
                                   QStringLiteral("joint_position_deg"),
                                   jointAxes, source);
    inserted += insertArrayMetrics(db, rawMessageId, sampleEpochMs, sampleAt,
                                   receivedEpochMs, topic, robotId,
                                   sequenceNumber, object,
                                   QStringLiteral("tcpPose"),
                                   QStringLiteral("tcp_pose"),
                                   poseAxes, source);
    inserted += insertArrayMetrics(db, rawMessageId, sampleEpochMs, sampleAt,
                                   receivedEpochMs, topic, robotId,
                                   sequenceNumber, object,
                                   QStringLiteral("flangePose"),
                                   QStringLiteral("flange_pose"),
                                   poseAxes, source);
    inserted += insertArrayMetrics(db, rawMessageId, sampleEpochMs, sampleAt,
                                   receivedEpochMs, topic, robotId,
                                   sequenceNumber, object,
                                   QStringLiteral("torques"),
                                   QStringLiteral("torque"),
                                   jointAxes, source);
    inserted += insertArrayMetrics(db, rawMessageId, sampleEpochMs, sampleAt,
                                   receivedEpochMs, topic, robotId,
                                   sequenceNumber, object,
                                   QStringLiteral("driverTemperatures"),
                                   QStringLiteral("driver_temperature"),
                                   jointAxes, source);

    const auto addNumber = [&](const QString& name,
                               const QString& axis,
                               double value) {
        if (insertMetric(db, rawMessageId, sampleEpochMs, sampleAt,
                         receivedEpochMs, topic, robotId, sequenceNumber,
                         name, axis, value, QVariant(QVariant::String),
                         source)) {
            ++inserted;
        }
    };

    const auto addText = [&](const QString& name,
                             const QString& axis,
                             const QString& value) {
        if (value.isEmpty()) {
            return;
        }
        if (insertMetric(db, rawMessageId, sampleEpochMs, sampleAt,
                         receivedEpochMs, topic, robotId, sequenceNumber,
                         name, axis, QVariant(QVariant::Double), value,
                         source)) {
            ++inserted;
        }
    };

    if (object.contains(QStringLiteral("connected"))) {
        addNumber(QStringLiteral("connected"), QStringLiteral(""),
                  object.value(QStringLiteral("connected")).toBool() ? 1.0 : 0.0);
    }
    if (object.contains(QStringLiteral("safetySi0"))) {
        addNumber(QStringLiteral("safety_si"), QStringLiteral("si0"),
                  object.value(QStringLiteral("safetySi0")).toDouble());
    }
    if (object.contains(QStringLiteral("safetySi1"))) {
        addNumber(QStringLiteral("safety_si"), QStringLiteral("si1"),
                  object.value(QStringLiteral("safetySi1")).toDouble());
    }
    if (object.contains(QStringLiteral("safetyStop"))) {
        addNumber(QStringLiteral("safety_stop"), QStringLiteral(""),
                  object.value(QStringLiteral("safetyStop")).toBool() ? 1.0 : 0.0);
    }
    if (object.contains(QStringLiteral("robotDiValid"))) {
        addNumber(QStringLiteral("robot_di_valid"), QStringLiteral(""),
                  object.value(QStringLiteral("robotDiValid")).toBool() ? 1.0 : 0.0);
    }
    if (object.contains(QStringLiteral("lastRobotDiError"))) {
        addNumber(QStringLiteral("last_robot_di_error"), QStringLiteral(""),
                  object.value(QStringLiteral("lastRobotDiError")).toDouble());
    }
    if (object.contains(QStringLiteral("reconnecting"))) {
        addNumber(QStringLiteral("reconnecting"), QStringLiteral(""),
                  object.value(QStringLiteral("reconnecting")).toBool() ? 1.0 : 0.0);
    }
    if (object.contains(QStringLiteral("reconnectCount"))) {
        addNumber(QStringLiteral("reconnect_count"), QStringLiteral(""),
                  object.value(QStringLiteral("reconnectCount")).toDouble());
    }
    if (object.contains(QStringLiteral("lastRecoveryEpochMs"))) {
        addNumber(QStringLiteral("last_recovery_epoch_ms"), QStringLiteral(""),
                  object.value(QStringLiteral("lastRecoveryEpochMs")).toDouble());
    }
    if (object.contains(QStringLiteral("recoveryEventId"))) {
        addNumber(QStringLiteral("recovery_event_id"), QStringLiteral(""),
                  object.value(QStringLiteral("recoveryEventId")).toDouble());
    }

    const QJsonArray robotDi = object.value(QStringLiteral("robotDi")).toArray();
    for (int i = 0; i < robotDi.size(); ++i) {
        if (robotDi.at(i).isDouble()) {
            addNumber(QStringLiteral("robot_di"),
                      QStringLiteral("di%1").arg(i),
                      robotDi.at(i).toDouble());
        }
    }

    addText(QStringLiteral("robot_state"), QStringLiteral(""),
            object.value(QStringLiteral("robotState")).toString());
    addText(QStringLiteral("interlock_state"), QStringLiteral(""),
            object.value(QStringLiteral("interlockState")).toString());
    addText(QStringLiteral("torque_source"), QStringLiteral(""),
            object.value(QStringLiteral("torqueSource")).toString());
    addText(QStringLiteral("tool_mount_state"), QStringLiteral(""),
            object.value(QStringLiteral("toolMountState")).toString());
    addText(QStringLiteral("tool_state"), QStringLiteral(""),
            object.value(QStringLiteral("toolState")).toString());
    addText(QStringLiteral("clamp_state"), QStringLiteral("clamp1"),
            object.value(QStringLiteral("clamp1State")).toString());
    addText(QStringLiteral("clamp_state"), QStringLiteral("clamp2"),
            object.value(QStringLiteral("clamp2State")).toString());
    addText(QStringLiteral("error_text"), QStringLiteral(""),
            object.value(QStringLiteral("errorText")).toString());
    addText(QStringLiteral("last_recovery_message"), QStringLiteral(""),
            object.value(QStringLiteral("lastRecoveryMessage")).toString());

    return inserted;
}

bool insertRawMessage(QSqlDatabase& db,
                      const QString& source,
                      const QString& topic,
                      const QByteArray& payload,
                      int* insertedMetricCount)
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(payload, &parseError);

    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "[IoTStorageCollector] invalid JSON skipped"
                   << "topic =" << topic
                   << "error =" << parseError.errorString();
        return false;
    }

    const QJsonObject object = doc.object();
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    const QString nowIso =
        QDateTime::fromMSecsSinceEpoch(nowMs).toString(Qt::ISODateWithMs);
    const qint64 sequenceNumber =
        static_cast<qint64>(
            object.value(QStringLiteral("sequenceNumber")).toDouble(0.0));

    if (!db.transaction()) {
        qWarning() << "[IoTStorageCollector] transaction start failed"
                   << db.lastError().text();
        return false;
    }

    QSqlQuery query(db);
    query.prepare(
        QStringLiteral(
            "INSERT INTO raw_messages ("
            "received_epoch_ms,"
            "received_at,"
            "source,"
            "topic,"
            "robot_id,"
            "message_type,"
            "schema_version,"
            "sequence_number,"
            "payload_bytes,"
            "raw_json"
            ") VALUES ("
            ":received_epoch_ms,"
            ":received_at,"
            ":source,"
            ":topic,"
            ":robot_id,"
            ":message_type,"
            ":schema_version,"
            ":sequence_number,"
            ":payload_bytes,"
            ":raw_json"
            ")"));

    query.bindValue(QStringLiteral(":received_epoch_ms"), nowMs);
    query.bindValue(QStringLiteral(":received_at"), nowIso);
    query.bindValue(QStringLiteral(":source"), source);
    query.bindValue(QStringLiteral(":topic"), topic);
    query.bindValue(QStringLiteral(":robot_id"),
                    object.value(QStringLiteral("robotId")).toInt(0));
    query.bindValue(QStringLiteral(":message_type"),
                    object.value(QStringLiteral("messageType")).toString());
    query.bindValue(QStringLiteral(":schema_version"),
                    object.value(QStringLiteral("schemaVersion")).toInt(0));
    query.bindValue(QStringLiteral(":sequence_number"), sequenceNumber);
    query.bindValue(QStringLiteral(":payload_bytes"), payload.size());
    query.bindValue(QStringLiteral(":raw_json"),
                    QString::fromUtf8(payload));

    if (!query.exec()) {
        qWarning() << "[IoTStorageCollector] insert failed"
                   << query.lastError().text()
                   << "topic =" << topic;
        db.rollback();
        return false;
    }

    const qint64 rawMessageId = query.lastInsertId().toLongLong();
    query.finish();

    int metricCount = 0;
    if (object.value(QStringLiteral("messageType")).toString() ==
        QStringLiteral("snapshot")) {
        metricCount = insertSnapshotMetrics(db, rawMessageId, nowMs, topic, object);
    }

    if (!db.commit()) {
        qWarning() << "[IoTStorageCollector] transaction commit failed"
                   << db.lastError().text()
                   << "topic =" << topic;
        db.rollback();
        return false;
    }

    if (insertedMetricCount != nullptr) {
        *insertedMetricCount = metricCount;
    }

    return true;
}

void printUsage()
{
    qInfo().noquote()
        << "Usage:\n"
        << "  iot_storage_collector [snapshotEndpoint] [topicPrefix] [databasePath]\n\n"
        << "Defaults:\n"
        << "  snapshotEndpoint = tcp://127.0.0.1:5556\n"
        << "  topicPrefix      = robot/\n"
        << "  databasePath     = ./data/iot_storage_YYYY-MM-DD.db\n\n"
        << "Example:\n"
        << "  iot_storage_collector tcp://192.168.57.100:5556 robot/ C:/Debug/IoTStorage/data/iot_storage.db";
}
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("iot_storage_collector"));

#ifndef ENABLE_ZEROMQ_TRANSPORT
    qCritical() << "[IoTStorageCollector]"
                << "ZeroMQ transport is disabled at build time";
    return 1;
#else
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    const QStringList args = app.arguments();
    if (args.contains(QStringLiteral("--help")) ||
        args.contains(QStringLiteral("-h"))) {
        printUsage();
        return 0;
    }

    const QString snapshotEndpoint =
        args.size() >= 2
            ? args.at(1)
            : QStringLiteral("tcp://127.0.0.1:5556");

    const QString topicPrefix =
        args.size() >= 3
            ? args.at(2)
            : QStringLiteral("robot/");

    const QString databasePath =
        args.size() >= 4
            ? args.at(3)
            : defaultDatabasePath();

    if (!ensureParentDirectory(databasePath)) {
        qCritical() << "[IoTStorageCollector]"
                    << "failed to create database directory"
                    << QFileInfo(databasePath).absolutePath();
        return 2;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"));
    db.setDatabaseName(databasePath);

    if (!db.open()) {
        qCritical() << "[IoTStorageCollector] database open failed"
                    << db.lastError().text()
                    << "path =" << databasePath;
        return 3;
    }

    if (!initializeSchema(db)) {
        db.close();
        return 4;
    }

    qInfo() << "[IoTStorageCollector] snapshotEndpoint =" << snapshotEndpoint;
    qInfo() << "[IoTStorageCollector] topicPrefix =" << topicPrefix;
    qInfo() << "[IoTStorageCollector] databasePath =" << databasePath;

    try {
        zmq::context_t context(1);
        zmq::socket_t socket(context, zmq::socket_type::sub);

        socket.set(zmq::sockopt::linger, 0);
        socket.set(zmq::sockopt::rcvtimeo, 250);
        socket.set(zmq::sockopt::subscribe, topicPrefix.toStdString());
        socket.connect(snapshotEndpoint.toStdString());

        qInfo() << "[IoTStorageCollector] started";

        quint64 receivedCount = 0;
        quint64 storedCount = 0;
        quint64 storedMetricCount = 0;

        while (g_running) {
            zmq::message_t topicMessage;
            const auto topicResult =
                socket.recv(topicMessage, zmq::recv_flags::none);

            if (!topicResult.has_value()) {
                continue;
            }

            zmq::message_t payloadMessage;
            const auto payloadResult =
                socket.recv(payloadMessage, zmq::recv_flags::none);

            if (!payloadResult.has_value()) {
                qWarning() << "[IoTStorageCollector]"
                           << "payload frame missing";
                continue;
            }

            const QString topic =
                QString::fromUtf8(
                    static_cast<const char*>(topicMessage.data()),
                    static_cast<int>(topicMessage.size()));

            const QByteArray payload(
                static_cast<const char*>(payloadMessage.data()),
                static_cast<int>(payloadMessage.size()));

            ++receivedCount;
            int insertedMetrics = 0;
            if (insertRawMessage(db,
                                 QStringLiteral("zmq_snapshot"),
                                 topic,
                                 payload,
                                 &insertedMetrics)) {
                ++storedCount;
                storedMetricCount += static_cast<quint64>(insertedMetrics);
            }

            if (storedCount == 1 || storedCount % 100 == 0) {
                qInfo() << "[IoTStorageCollector]"
                        << "received =" << receivedCount
                        << "stored =" << storedCount
                        << "metrics =" << storedMetricCount
                        << "topic =" << topic;
            }
        }

        qInfo() << "[IoTStorageCollector] stopping"
                << "received =" << receivedCount
                << "stored =" << storedCount
                << "metrics =" << storedMetricCount;

        socket.close();
        context.shutdown();
        context.close();
        db.close();

        qInfo() << "[IoTStorageCollector] stopped";
        return 0;

    } catch (const zmq::error_t& error) {
        qCritical() << "[IoTStorageCollector] ZeroMQ error:"
                    << error.what();
        db.close();
        return 5;
    }
#endif
}
