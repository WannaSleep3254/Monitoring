//#include "gui/mainwindow.h"

//#include <QApplication>
#include <QCoreApplication>
#include <QTimer>
#include <iostream>
#include "monitoring/FairinoMonitorService.h"

//#include "robot.h"
//#include <thread>
//#include <QDebug>
int main(int argc, char *argv[])
{
//    QApplication a(argc, argv);
    //MainWindow w;
    //w.show();
//    fr_test();
//    return a.exec();
    QCoreApplication a(argc, argv);
#if false
    const char* ip = "192.168.57.121";

    FRRobot robot1;
    FRRobot robot2;

    qDebug() << "[TEST 1] robot1 RPC...\n";
    int ret1 = robot1.RPC(ip);
    qDebug() << "robot1.RPC ret = " << ret1 << "\n";

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    qDebug() << "[TEST 2] robot2 RPC...\n";
    int ret2 = robot2.RPC(ip);
    qDebug() << "robot2.RPC ret = " << ret2 << "\n";

    qDebug() << "[RESULT]\n";
    qDebug() << "robot1 = " << ret1 << "\n";
    qDebug() << "robot2 = " << ret2 << "\n";

    robot1.CloseRPC();
    robot2.CloseRPC();
#else

    monitoring::FairinoMonitorService svc;
    monitoring::FairinoMonitorService::Options opt;
    opt.poll_period_ms = 200;     // 200ms마다 폴링
    opt.robot_id = 0;
    opt.logger_level = 1;
    opt.enable_reconnect = true;

    const std::string ip = "192.168.57.121";

    // 콜백: 업데이트마다 출력(너무 잦으면 로그 폭발 주의)
    svc.setCallback([](const monitoring::FairinoMonitorService::SnapshotWithMeta& meta){
        const auto& s = meta.snapshot;  // 스냅샷 추출

        if (!s.connected) {
            std::cout << "[MON] disconnected, err=" << s.last_error << "\n";
            return;
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

    QTimer::singleShot(1000, &a, [&]() {
        std::cout << "[CMD] startJog = "
                  << svc.startJog(
                         0,      // joint jog
                         1,      // J1
                         0,      // 1:+,0:-
                         5.0f,   // vel %
                         10.0f,  // acc %
                         5.5f    // max distance deg
                         )
                  << "\n";
    });

    QTimer::singleShot(2000, &a, [&]() {
        std::cout << "[CMD] stopJog = "
                  << svc.stopJog(1)   // joint stop
                  << "\n";
    });

    QTimer::singleShot(2500, &a, [&]() {
        svc.stop();
        a.quit();
    });
#endif

    return a.exec();
}
