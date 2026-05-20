#ifndef MONITORING_QML_MODE
#define MONITORING_QML_MODE 1
#endif

#if MONITORING_QML_MODE

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>
#include <QDebug>
#include <QCoreApplication>
#include <QSettings>

#include "runtime/MultiChannelRobotGateway.h"
#include "iot/viewmodel/IotViewModel.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName("Gachisoft");
    QCoreApplication::setOrganizationDomain("gachisoft.co.kr");
    QCoreApplication::setApplicationName("MonitoringApp");

    QQmlApplicationEngine engine;
    // ============================================================
    // Runtime / Gateway
    // ============================================================
    auto* robotGateway = new MultiChannelRobotGateway(&engine);
    auto* iotViewModel = new IotViewModel(&engine);
    // ============================================================
    // Runtime / Gateway source mode 설정 (dummy, remote)
    // ============================================================
    QSettings settings;
    const QString gatewaySourceMode =
        settings.value(QStringLiteral("runtime/gatewaySourceMode"),
                       QStringLiteral("dummy")).toString();

    if (!robotGateway->setSourceModeName(gatewaySourceMode)) {
        qWarning() << "[Gateway] invalid configured source mode:"
                   << gatewaySourceMode
                   << "fallback to dummy";

        robotGateway->setSourceModeName(QStringLiteral("dummy"));
    }
    // ============================================================
    iotViewModel->setRobotGateway(robotGateway);
    if (!iotViewModel->initialize()) {
        qWarning() << "[IoTViewModel] initialize failed:"
                   << iotViewModel->lastError();
    }

    engine.rootContext()->setContextProperty("iotViewModel", iotViewModel);
    if (!robotGateway->start()) {
        qWarning() << "[Gateway] Failed to start MultiChannelRobotGateway";
    }
    const QUrl url(QStringLiteral("qrc:/qml/Main.qml"));

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl) {
                qCritical() << "[QML] Failed to load:" << url;
                QCoreApplication::exit(-1);
            }
        },
        Qt::QueuedConnection
        );

    engine.load(url);

    return app.exec();
}

#else

#include "gui/mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow w;
    w.show();

    return app.exec();
}

#endif
