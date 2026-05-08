#include <QCoreApplication>
#include <QTimer>
#include <iostream>

#include "monitoring/FairinoMonitorService.h"

int main(int argc, char* argv[])
{
    QCoreApplication a(argc, argv);

    monitoring::FairinoMonitorService svc;
    monitoring::FairinoMonitorService::Options opt;
    opt.poll_period_ms = 200;
    opt.robot_id = 0;
    opt.logger_level = 1;
    opt.enable_reconnect = true;

    const std::string ip = "192.168.57.121";

    svc.setCallback([](const monitoring::FairinoMonitorService::SnapshotWithMeta& meta) {
        const auto& s = meta.snapshot;

        if (!s.connected) {
            std::cout << "[MON] disconnected"
                      << " poll_err=" << s.last_poll_error
                      << " code=" << s.last_poll_error_code
                      << "\n";
            return;
        }

        std::cout << "[MON] [Seq=" << meta.sequence_number << "] "
                  << "q(deg)="
                  << s.joint_pos_deg[0] << ", "
                  << s.joint_pos_deg[1] << ", "
                  << s.joint_pos_deg[2] << ", "
                  << s.joint_pos_deg[3] << ", "
                  << s.joint_pos_deg[4] << ", "
                  << s.joint_pos_deg[5] << "\n";

        if (s.temperature_valid && meta.sequence_number % 5 == 0) {
            std::cout << "[TEMP] "
                      << s.driver_temperature[0] << ", "
                      << s.driver_temperature[1] << ", "
                      << s.driver_temperature[2] << ", "
                      << s.driver_temperature[3] << ", "
                      << s.driver_temperature[4] << ", "
                      << s.driver_temperature[5] << "\n";
        }

        if (s.driver_torque_valid) {
            std::cout << "[DRV_TORQUE] "
                      << s.driver_torque[0] << ", "
                      << s.driver_torque[1] << ", "
                      << s.driver_torque[2] << ", "
                      << s.driver_torque[3] << ", "
                      << s.driver_torque[4] << ", "
                      << s.driver_torque[5] << "\n";
        }
    });

    if (!svc.start(ip, opt)) {
        std::cerr << "svc.start() failed\n";
        return -1;
    }

    svc.queryAndPrintStaticInfo();

    QTimer t;
    QObject::connect(&t, &QTimer::timeout, [&]() {
        auto s = svc.latest();

        std::cout << "[TICK] motion_done=" << int(s.motion_done)
                  << " e-stop=" << int(s.emergency_stop)
                  << " queue=" << s.motion_queue_len
                  << "\n";
    });
    t.start(1000);

    QObject::connect(&a, &QCoreApplication::aboutToQuit, [&]() {
        svc.stop();
    });

    QTimer::singleShot(5000, &a, [&]() {
        svc.stop();
        a.quit();
    });

    return a.exec();
}