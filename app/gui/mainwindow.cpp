#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QPushButton>
#include <QDebug>
#include <QString>
#include <QMetaObject>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 출력창은 로그 표시 전용
    ui->plainTextEdit_CommandResult->setReadOnly(true);

    // Joint position 표시창은 값 표시 전용
    ui->lineEdit_J1->setReadOnly(true);
    ui->lineEdit_J2->setReadOnly(true);
    ui->lineEdit_J3->setReadOnly(true);
    ui->lineEdit_J4->setReadOnly(true);
    ui->lineEdit_J5->setReadOnly(true);
    ui->lineEdit_J6->setReadOnly(true);

    setupConnections();

    appendLog("[GUI] MainWindow initialized");
}

MainWindow::~MainWindow()
{
    service_.stop();
    qDebug() << "[GUI] MainWindow destroyed";
    delete ui;
}

void MainWindow::setupConnections()
{
    // ----------------------------
    // Connection buttons
    // ----------------------------
    connect(ui->pushButton_Connect, &QPushButton::clicked,
            this, &MainWindow::handleConnectClicked);

    connect(ui->pushButton_Enable, &QPushButton::clicked,
            this, &MainWindow::handleEnableClicked);

    connect(ui->pushButton_ClearError, &QPushButton::clicked,
            this, &MainWindow::handleClearErrorClicked);

    // ----------------------------
    // Manual Jog buttons
    // pressed  : start jog
    // released : stop jog
    // ----------------------------
#if true
    bindJointJogButton(ui->pushButton_J1Minus, 1, false);
    bindJointJogButton(ui->pushButton_J1Plus,  1, true);
#else
    connect(ui->pushButton_J1Minus, &QPushButton::pressed, this, [this]() {
        startJointJog(1, false);
    });

    connect(ui->pushButton_J1Minus, &QPushButton::released,
            this, &MainWindow::stopJointJog);

    connect(ui->pushButton_J1Plus, &QPushButton::pressed, this, [this]() {
        startJointJog(1, true);
    });

    connect(ui->pushButton_J1Plus, &QPushButton::released,
            this, &MainWindow::stopJointJog);
#endif
#if true
    bindJointJogButton(ui->pushButton_J2Minus, 2, false);
    bindJointJogButton(ui->pushButton_J2Plus,  2, true);
#else
    connect(ui->pushButton_J2Minus, &QPushButton::pressed, this, [this]() {
        startJointJog(2, false);
    });

    connect(ui->pushButton_J2Minus, &QPushButton::released,
            this, &MainWindow::stopJointJog);

    connect(ui->pushButton_J2Plus, &QPushButton::pressed, this, [this]() {
        startJointJog(2, true);
    });

    connect(ui->pushButton_J2Plus, &QPushButton::released,
            this, &MainWindow::stopJointJog);
#endif
#if true
    bindJointJogButton(ui->pushButton_J3Minus, 3, false);
    bindJointJogButton(ui->pushButton_J3Plus,  3, true);
#else
    connect(ui->pushButton_J3Minus, &QPushButton::pressed, this, [this]() {
        startJointJog(3, false);
    });

    connect(ui->pushButton_J3Minus, &QPushButton::released,
            this, &MainWindow::stopJointJog);

    connect(ui->pushButton_J3Plus, &QPushButton::pressed, this, [this]() {
        startJointJog(3, true);
    });

    connect(ui->pushButton_J3Plus, &QPushButton::released,
            this, &MainWindow::stopJointJog);
#endif
#if true
    bindJointJogButton(ui->pushButton_J4Minus, 4, false);
    bindJointJogButton(ui->pushButton_J4Plus,  4, true);
#else
    connect(ui->pushButton_J4Minus, &QPushButton::pressed, this, [this]() {
        startJointJog(4, false);
    });

    connect(ui->pushButton_J4Minus, &QPushButton::released,
            this, &MainWindow::stopJointJog);

    connect(ui->pushButton_J4Plus, &QPushButton::pressed, this, [this]() {
        startJointJog(4, true);
    });

    connect(ui->pushButton_J4Plus, &QPushButton::released,
            this, &MainWindow::stopJointJog);
#endif
#if true
    bindJointJogButton(ui->pushButton_J5Minus, 5, false);
    bindJointJogButton(ui->pushButton_J5Plus,  5, true);
#else
    connect(ui->pushButton_J5Minus, &QPushButton::pressed, this, [this]() {
        startJointJog(5, false);
    });

    connect(ui->pushButton_J5Minus, &QPushButton::released,
            this, &MainWindow::stopJointJog);

    connect(ui->pushButton_J5Plus, &QPushButton::pressed, this, [this]() {
        startJointJog(5, true);
    });

    connect(ui->pushButton_J5Plus, &QPushButton::released,
            this, &MainWindow::stopJointJog);
#endif
#if true
    bindJointJogButton(ui->pushButton_J6Minus, 6, false);
    bindJointJogButton(ui->pushButton_J6Plus,  6, true);
#else
    connect(ui->pushButton_J6Minus, &QPushButton::pressed, this, [this]() {
        startJointJog(6, false);
    });

    connect(ui->pushButton_J6Minus, &QPushButton::released,
            this, &MainWindow::stopJointJog);

    connect(ui->pushButton_J6Plus, &QPushButton::pressed, this, [this]() {
        startJointJog(6, true);
    });

    connect(ui->pushButton_J6Plus, &QPushButton::released,
            this, &MainWindow::stopJointJog);
#endif
    appendLog("[GUI] Signal connections initialized");
}

void MainWindow::bindJointJogButton(QPushButton* button, int joint, bool positive)
{
    if (!button) {
        qDebug() << "[GUI] bindJointJogButton failed. button is null"
                 << "joint =" << joint
                 << "positive =" << positive;
        return;
    }

    connect(button, &QPushButton::pressed, this, [this, joint, positive]() {
//        startJointJogDebug(joint, positive);
        startJointJog(joint, positive);
    });

    connect(button, &QPushButton::released,
//            this, &MainWindow::stopJointJogDebug);
            this, &MainWindow::stopJointJog);
}

void MainWindow::handleConnectClicked()
{
#if false
    ++debug_command_sequence_;

    if (connected_) {
        qDebug() << "[GUI][CONNECT] already connected";

        appendLog(QString("[CMD] seq=%1 name=Connect ok=1 msg=Already connected")
                      .arg(debug_command_sequence_));
        return;
    }
    connected_ = true;

    qDebug() << "[GUI][CONNECT] clicked";
    qDebug() << "[GUI][CONNECT] connected_ =" << connected_;

    appendLog(QString("[CMD] seq=%1 name=Connect ok=1")
                  .arg(debug_command_sequence_));
#else
    if (connected_) {
        appendLog("[MON] Already connected");
        return;
    }

    monitoring::FairinoMonitorService::Options opt;
    opt.poll_period_ms = 200;
    opt.robot_id = 0;
    opt.logger_level = 1;
    opt.enable_reconnect = true;

    service_.setCallback([this](const monitoring::FairinoMonitorService::SnapshotWithMeta& meta) {
        const auto snapshot = meta.snapshot;

        QMetaObject::invokeMethod(this, [this, snapshot]() {
            updateSnapshot(snapshot);
        }, Qt::QueuedConnection);
    });

    const std::string ip = "192.168.57.121";

    const bool ok = service_.start(ip, opt);
    connected_ = ok;

    if (!ok) {
        service_.stop();          // 실패한 연결 시도 정리
        connected_ = false;
        appendLog("[MON] Connect failed");
        return;
    }

    appendLog("[MON] Connected");

    // 선택 사항: 정적 정보 콘솔 출력
    service_.queryAndPrintStaticInfo();
#endif
}

void MainWindow::handleEnableClicked()
{
#if false
    ++debug_command_sequence_;

    if (!connected_) {
        qDebug() << "[GUI][ENABLE] failed. not connected";

        appendLog(QString("[CMD] seq=%1 name=Enable ok=0 msg=Not connected")
                      .arg(debug_command_sequence_));
        return;
    }

    qDebug() << "[GUI][ENABLE] clicked";

    appendLog(QString("[CMD] seq=%1 name=Enable ok=1 msg=Debug enable")
                  .arg(debug_command_sequence_));
#else
    if (!connected_) {
        appendLog("[CMD] Enable failed: Not connected");
        return;
    }

    auto result = service_.robotEnableEx(true);
    appendCommandResult(result);
#endif
}

void MainWindow::handleClearErrorClicked()
{
#if false
    ++debug_command_sequence_;

    if (!connected_) {
        qDebug() << "[GUI][CLEAR_ERROR] failed. not connected";

        appendLog(QString("[CMD] seq=%1 name=ClearError ok=0 msg=Not connected")
                      .arg(debug_command_sequence_));
        return;
    }

    qDebug() << "[GUI][CLEAR_ERROR] clicked";

    appendLog(QString("[CMD] seq=%1 name=ClearError ok=1 msg=Debug clear error")
                  .arg(debug_command_sequence_));
#else
    if (!connected_) {
        appendLog("[CMD] ClearError failed: Not connected");
        return;
    }

    auto result = service_.clearErrorEx();
    appendCommandResult(result);
#endif
}

void MainWindow::startJointJogDebug(int joint, bool positive)
{
    ++debug_command_sequence_;

    const QString direction = positive ? "+" : "-";

    if (!connected_) {
        qDebug() << "[GUI][JOG_START] failed. not connected"
                 << "joint =" << joint
                 << "direction =" << direction;

        appendLog(QString("[CMD] seq=%1 name=StartJointJog joint=J%2 dir=%3 ok=0 msg=Not connected")
                      .arg(debug_command_sequence_)
                      .arg(joint)
                      .arg(direction));
        return;
    }

    qDebug() << "[GUI][JOG_START]"
             << "joint =" << joint
             << "direction =" << direction;

    appendLog(QString("[CMD] seq=%1 name=StartJointJog joint=J%2 dir=%3 ok=1")
                  .arg(debug_command_sequence_)
                  .arg(joint)
                  .arg(direction));
}

void MainWindow::stopJointJogDebug()
{
    ++debug_command_sequence_;

    if (!connected_) {
        qDebug() << "[GUI][JOG_STOP] failed. not connected";

        appendLog(QString("[CMD] seq=%1 name=StopJointJog ok=0 msg=Not connected")
                      .arg(debug_command_sequence_));
        return;
    }

    qDebug() << "[GUI][JOG_STOP]";

    appendLog(QString("[CMD] seq=%1 name=StopJointJog ok=1")
                  .arg(debug_command_sequence_));
}

void MainWindow::startJointJog(int joint, bool positive)
{
    if (!connected_) {
        appendLog(QString("[CMD] StartJointJog failed: J%1 %2, Not connected")
                      .arg(joint)
                      .arg(positive ? "+" : "-"));
        return;
    }

    auto result = service_.startJointJogEx(
        joint,
        positive,
        5.0f,    // vel %
        10.0f,   // acc %
        0.5f     // max_deg
        );

    appendCommandResult(result);
}

void MainWindow::stopJointJog()
{
    if (!connected_) {
        appendLog("[CMD] StopJointJog skipped: Not connected");
        return;
    }

    auto result = service_.stopJointJogEx();
    appendCommandResult(result);
}

void MainWindow::appendLog(const QString& text)
{
    qDebug().noquote() << text;

    if (ui && ui->plainTextEdit_CommandResult) {
        ui->plainTextEdit_CommandResult->appendPlainText(text);
    }
}

void MainWindow::updateSnapshot(const monitoring::RobotSnapshot& s)
{
    if (!s.connected) {
        appendLog(QString("[MON] Disconnected: code=%1 err=%2")
                      .arg(s.last_poll_error_code)
                      .arg(QString::fromStdString(s.last_poll_error)));
        return;
    }

    ui->lineEdit_J1->setText(QString::number(s.joint_pos_deg[0], 'f', 3));
    ui->lineEdit_J2->setText(QString::number(s.joint_pos_deg[1], 'f', 3));
    ui->lineEdit_J3->setText(QString::number(s.joint_pos_deg[2], 'f', 3));
    ui->lineEdit_J4->setText(QString::number(s.joint_pos_deg[3], 'f', 3));
    ui->lineEdit_J5->setText(QString::number(s.joint_pos_deg[4], 'f', 3));
    ui->lineEdit_J6->setText(QString::number(s.joint_pos_deg[5], 'f', 3));

    if (s.command_sequence_number != last_command_sequence_) {
        last_command_sequence_ = s.command_sequence_number;

        appendLog(QString("[CMD_STATE] seq=%1 name=%2 ok=%3 code=%4 err=%5")
                      .arg(s.command_sequence_number)
                      .arg(QString::fromStdString(s.last_command_name))
                      .arg(s.last_command_ok)
                      .arg(s.last_command_error_code)
                      .arg(QString::fromStdString(s.last_command_error)));
    }
}

void MainWindow::appendCommandResult(
    const monitoring::FairinoMonitorService::CommandResult& result)
{
    appendLog(QString("[CMD] seq=%1 name=%2 ok=%3 code=%4 msg=%5")
                  .arg(result.sequence_number)
                  .arg(QString::fromStdString(result.name))
                  .arg(result.ok)
                  .arg(result.code)
                  .arg(QString::fromStdString(result.message)));
}
