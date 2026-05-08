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
        static uint64_t last_cmd_seq = 0;
        const auto& s = meta.snapshot;

        if (s.command_sequence_number != last_cmd_seq) {
            last_cmd_seq = s.command_sequence_number;

            std::cout << "[CMD_STATE] seq=" << s.command_sequence_number
                      << " name=" << s.last_command_name
                      << " ok=" << s.last_command_ok
                      << " code=" << s.last_command_error_code
                      << " err=" << s.last_command_error
                      << "\n";
        }

        if (!s.connected) {
            std::cout << "[MON] disconnected"
                      << " poll_err=" << s.last_poll_error
                      << " code=" << s.last_poll_error_code
                      << "\n";
            return;
        }

        std::cout << "[MON] [Seq=" << meta.sequence_number << "] "
                  << "J1=" << s.joint_pos_deg[0]
                  << " J2=" << s.joint_pos_deg[1]
                  << " J3=" << s.joint_pos_deg[2]
                  << " J4=" << s.joint_pos_deg[3]
                  << " J5=" << s.joint_pos_deg[4]
                  << " J6=" << s.joint_pos_deg[5]
                  << "\n";
    });

    if (!svc.start(ip, opt)) {
        std::cerr << "svc.start() failed\n";
        return -1;
    }

    svc.queryAndPrintStaticInfo();

    QObject::connect(&a, &QCoreApplication::aboutToQuit, [&]() {
        svc.stop();
    });

    QTimer::singleShot(900, &a, [&]() {
        auto s = svc.latest();
        std::cout << "[BEFORE JOG] J1=" << s.joint_pos_deg[0] << "\n";
    });

    QTimer::singleShot(1000, &a, [&]() {
        auto r = svc.startJointJogEx(
            1,       // J1
            true,    // + direction
            5.0f,    // velocity %
            10.0f,   // acceleration %
            0.5f     // max degree
        );

        std::cout << "[CMD] seq=" << r.sequence_number
                  << " name=" << r.name
                  << " ok=" << r.ok
                  << " code=" << r.code
                  << " msg=" << r.message
                  << "\n";
    });

    QTimer::singleShot(1300, &a, [&]() {
        auto r = svc.stopJointJogEx();

        std::cout << "[CMD] seq=" << r.sequence_number
                  << " name=" << r.name
                  << " ok=" << r.ok
                  << " code=" << r.code
                  << " msg=" << r.message
                  << "\n";
    });

    QTimer::singleShot(1700, &a, [&]() {
        auto s = svc.latest();
        std::cout << "[AFTER JOG] J1=" << s.joint_pos_deg[0] << "\n";
    });

    QTimer::singleShot(2500, &a, [&]() {
        svc.stop();
        a.quit();
    });

    return a.exec();
}