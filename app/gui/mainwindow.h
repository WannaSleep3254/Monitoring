#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <cstdint>

#include "monitoring/FairinoMonitorService.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QPushButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void setupConnections();

    void handleConnectClicked();
    void handleEnableClicked();
    void handleClearErrorClicked();
#if false
    void startJointJogDebug(int joint, bool positive);
    void stopJointJogDebug();
#endif
    void startJointJog(int joint, bool positive);
    void stopJointJog();

    void bindJointJogButton(QPushButton* button, int joint, bool positive);

    void updateSnapshot(const monitoring::RobotSnapshot& snapshot);
    void appendCommandResult(
        const monitoring::FairinoMonitorService::CommandResult& result);

    void appendLog(const QString& text);

private:
    Ui::MainWindow *ui;

    monitoring::FairinoMonitorService service_;

    bool connected_ = false;
#if false
    uint64_t debug_command_sequence_ = 0;
#endif
    uint64_t last_command_sequence_ = 0;
};
#endif // MAINWINDOW_H
