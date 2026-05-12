#ifndef MONITORING_QML_MODE
#define MONITORING_QML_MODE 1
#endif

#if MONITORING_QML_MODE

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QUrl>
#include <QDebug>
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QQmlApplicationEngine engine;

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
