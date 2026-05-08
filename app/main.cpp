#ifndef MONITORING_GUI_MODE
#define MONITORING_GUI_MODE 1
#endif

#if MONITORING_GUI_MODE
#include "gui/mainwindow.h"
#include <QApplication>
#else
#include <QCoreApplication>
#include <QTimer>
#include <iostream>
#include "monitoring/FairinoMonitorService.h"
#endif

int main(int argc, char *argv[])
{
#if MONITORING_GUI_MODE
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
#else
    QCoreApplication a(argc, argv);

    monitoring::FairinoMonitorService svc;
    monitoring::FairinoMonitorService::Options opt;
    opt.poll_period_ms = 200;     // 200ms마다 폴링
    opt.robot_id = 0;
    opt.logger_level = 1;
    opt.enable_reconnect = true;

    const std::string ip = "192.168.57.121";

    // 콜백: 업데이트마다 출력(너무 잦으면 로그 폭발 주의)
    svc.setCallback([](const monitoring::FairinoMonitorService::SnapshotWithMeta& meta){
        static uint64_t last_cmd_seq = 0;
        const auto& s = meta.snapshot;  // 스냅샷 추출

        if (!s.connected) {
            std::cout << "[MON] disconnected"
                      << " poll_err=" << s.last_poll_error
                      << " code=" << s.last_poll_error_code
                      << "\n";
            return;
        }
        // 명령 상태는 command_sequence_number가 바뀔 때만 출력
        if (s.command_sequence_number != last_cmd_seq) {
            last_cmd_seq = s.command_sequence_number;

            std::cout << "[CMD_STATE] seq=" << s.command_sequence_number
                      << " name=" << s.last_command_name
                      << " ok=" << s.last_command_ok
                      << " code=" << s.last_command_error_code
                      << " err=" << s.last_command_error
                      << "\n";
        }
        // 연결된 경우, 주요 상태 출력
        std::cout << "[MON] [Seq=" << meta.sequence_number << "] "
                  << "q(deg)="
                  << s.joint_pos_deg[0] << ", "
                  << s.joint_pos_deg[1] << ", "
                  << s.joint_pos_deg[2] << ", "
                  << s.joint_pos_deg[3] << ", "
                  << s.joint_pos_deg[4] << ", "
                  << s.joint_pos_deg[5] << "\n";
        // 온도/드라이버 토크는 유효한 경우에만 출력
        if (s.temperature_valid) {
            std::cout << "[TEMP] "
                      << s.driver_temperature[0] << ", "
                      << s.driver_temperature[1] << ", "
                      << s.driver_temperature[2] << ", "
                      << s.driver_temperature[3] << ", "
                      << s.driver_temperature[4] << ", "
                      << s.driver_temperature[5] << "\n";
        }
        // 드라이버 토크는 유효한 경우에만 출력
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

    // (선택) 정적 정보 한번 출력(연결 후)
    svc.queryAndPrintStaticInfo();

    // latest()를 주기적으로 읽어서 출력하는 방식(콜백 대신 사용 가능)
    QTimer t;
    QObject::connect(&t, &QTimer::timeout, [&](){
        auto s = svc.latest();
        std::cout << "[TICK] motion_done=" << int(s.motion_done)
                  << " e-stop=" << int(s.emergency_stop)
                  << " queue=" << s.motion_queue_len
                  << "\n";
    });
    t.start(1000); // 1초마다

    // 종료 처리(CTRL+C는 콘솔에서)
    QObject::connect(&a, &QCoreApplication::aboutToQuit, [&](){
        svc.stop();
    });
/*
    QTimer::singleShot(1000, &a, [&]() {
        auto res = svc.startJointJogEx(
                         1,      // J1
                         true,   // true:+,false:-
                         5.0f,   // vel %
                         10.0f,  // acc %
                         0.5f    // max distance deg
                         );
        std::cout << "[CMD] seq=" << res.sequence_number
                  << " name=" << res.name
                  << " ok=" << res.ok
                  << " code=" << res.code
                  << " msg=" << res.message
                  << "\n";
    });

    QTimer::singleShot(2000, &a, [&]() {
        auto res = svc.stopJointJogEx();
        std::cout << "[CMD] seq=" << res.sequence_number
                  << " name=" << res.name
                  << " ok=" << res.ok
                  << " code=" << res.code
                  << " msg=" << res.message
                  << "\n";
    });
*/
    QTimer::singleShot(2500, &a, [&]() {
        svc.stop();
        a.quit();
    });

    return a.exec();
#endif
}
