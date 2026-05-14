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

#include "runtime/MultiChannelRobotGateway.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QQmlApplicationEngine engine;

    // ============================================================
    // Runtime / Gateway
    // ============================================================
    auto* robotGateway = new MultiChannelRobotGateway(&engine);

    if (!robotGateway->start()) {
        qWarning() << "[Gateway] Failed to start MultiChannelRobotGateway";
    }

    // 아직 QML에서 직접 사용하지 않더라도, 이후 ViewModel 연결 전 테스트용으로 등록 가능
    engine.rootContext()->setContextProperty("robotGateway", robotGateway);

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
