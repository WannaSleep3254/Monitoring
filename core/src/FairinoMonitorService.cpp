#include "monitoring/FairinoMonitorService.h"

#include "robot.h"   // ✅ 오직 cpp에서만
#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

namespace monitoring {

struct FairinoMonitorService::Impl
{
    FRRobot robot{};
    Options opt{};
    std::string ip;

    std::atomic<bool> running{false};
    std::thread th;

    mutable std::mutex mtx;
    RobotSnapshot last{};

    SnapshotCallback cb;
    //추가
    uint64_t polling_sequence = 0;

    // 내부 유틸: 배열 복사
    template<typename T, size_t N>
    static void copy6(std::array<T,N>& dst, const T src[6])
    {
        for (int i=0;i<6;i++) dst[i] = src[i];
    }

    static void copy6d(std::array<double,6>& dst, const double src[6])
    {
        for (int i=0;i<6;i++) dst[i] = src[i];
    }

    bool connect()
    {
        robot.LoggerInit();
        robot.SetLoggerLevel(opt.logger_level);

        // SDK 버전 (fr_test와 동일)
        char sdkVersion[64] = {0};
        int rtn = robot.GetSDKVersion(sdkVersion);
        if (rtn == 0) {
            std::cout << "SDK-Version : " << sdkVersion << "\n\n";
        }

        // 연결
        rtn = robot.RPC(ip.c_str());
        std::cout << "Connect : " << (rtn==0 ? "true" : "false") << "\n";
        if (rtn != 0) return false;

        if (opt.enable_reconnect) {
            robot.SetReConnectParam(true, opt.reconnect_timeout_ms, opt.reconnect_interval_ms);
        }

        return true;
    }

    void disconnect()
    {
        robot.CloseRPC();
    }

    // fr_test의 while(true) 내용을 1회 실행으로 분리
    void pollOnce()
    {
        RobotSnapshot s{};
        s.connected = true;
        int rtn = 0;

        // joint pos deg
        JointPos j_deg{};
        rtn=robot.GetActualJointPosDegree(opt.robot_id, &j_deg);
/*
        if (rtn != 0) {
            s.connected = false;
            s.last_error_code = rtn;
            s.last_error = "GetActualJointPosDegree failed: code="
                           + std::to_string(rtn);
            goto error_exit;  // 조기 종료
        }
*/
        for (int i=0;i<6;i++) s.joint_pos_deg[i] = j_deg.jPos[i];

        // joint speeds deg
        float jointSpeed[6] = {0};
        rtn=robot.GetActualJointSpeedsDegree(opt.robot_id, jointSpeed);
        for (int i=0;i<6;i++) s.joint_speed_deg[i] = jointSpeed[i];

        // joint acc deg
        float jointAcc[6] = {0};
        rtn=robot.GetActualJointAccDegree(opt.robot_id, jointAcc);
        for (int i=0;i<6;i++) s.joint_acc_deg[i] = jointAcc[i];

        // TCP composite speed (target/actual)
        float tcp_speed = 0.0f;
        float ori_speed = 0.0f;
        rtn=robot.GetTargetTCPCompositeSpeed(opt.robot_id, &tcp_speed, &ori_speed);
        s.target_tcp_speed_comp = tcp_speed;
        s.target_ori_speed_comp = ori_speed;

        rtn=robot.GetActualTCPCompositeSpeed(opt.robot_id, &tcp_speed, &ori_speed);
        s.actual_tcp_speed_comp = tcp_speed;
        s.actual_ori_speed_comp = ori_speed;

        // TCP speed 6dof (target/actual)
        float targetSpeed[6] = {0};
        rtn=robot.GetTargetTCPSpeed(opt.robot_id, targetSpeed);
        for (int i=0;i<6;i++) s.target_tcp_speed_6[i] = targetSpeed[i];

        float actualSpeed[6] = {0};
        rtn=robot.GetActualTCPSpeed(opt.robot_id, actualSpeed);
        for (int i=0;i<6;i++) s.actual_tcp_speed_6[i] = actualSpeed[i];

        // TCP pose
        DescPose tcp{};
        rtn=robot.GetActualTCPPose(opt.robot_id, &tcp);
        s.tcp_pose = { tcp.tran.x, tcp.tran.y, tcp.tran.z, tcp.rpy.rx, tcp.rpy.ry, tcp.rpy.rz };

        // Flange pose
        DescPose flange{};
        rtn=robot.GetActualToolFlangePose(opt.robot_id, &flange);
        s.flange_pose = { flange.tran.x, flange.tran.y, flange.tran.z, flange.rpy.rx, flange.rpy.ry, flange.rpy.rz };

        // tcp/wobj num
        int id = 0;
        rtn=robot.GetActualTCPNum(opt.robot_id, &id);
        s.tcp_num = id;

        rtn=robot.GetActualWObjNum(opt.robot_id, &id);
        s.wobj_num = id;

        // torques
        float jtorque[6] = {0};
        rtn=robot.GetJointTorques(opt.robot_id, jtorque);
        for (int i=0;i<6;i++) s.joint_torque[i] = jtorque[i];

        // system clock
        float t_ms = 0.0f;
        rtn=robot.GetSystemClock(&t_ms);
        s.system_clock_ms = t_ms;

        // joint config
        int config = 0;
        rtn=robot.GetRobotCurJointsConfig(&config);
        s.joints_config = config;

        // motion done
        uint8_t motionDone = 0;
        rtn=robot.GetRobotMotionDone(&motionDone);
        s.motion_done = motionDone;

        // queue length
        int len = 0;
        rtn=robot.GetMotionQueueLength(&len);
        s.motion_queue_len = len;

        // emergency stop
        uint8_t emergState = 0;
        rtn=robot.GetRobotEmergencyStopState(&emergState);
        s.emergency_stop = emergState;

        // sdk com state
        int comstate = 0;
        rtn=robot.GetSDKComState(&comstate);
        s.sdk_com_state = comstate;

        // safety stop state
        uint8_t si0 = 0, si1 = 0;
        rtn=robot.GetSafetyStopState(&si0, &si1);
        s.safety_si0 = si0;
        s.safety_si1 = si1;

        // temperatures / driver torque
        double temp[6] = {0};
        rtn=robot.GetJointDriverTemperature(temp);
        for (int i=0;i<6;i++) s.driver_temperature[i] = temp[i];

        double torque[6] = {0};
        rtn=robot.GetJointDriverTorque(torque);
        for (int i=0;i<6;i++) s.driver_torque[i] = torque[i];

        // (원본 fr_test 마지막) realtime pkg
        // ROBOT_STATE_PKG pkg{};
        // robot.GetRobotRealTimeState(&pkg);
        // -> 필요하면 RobotSnapshot에 pkg에서 뽑아낼 항목을 추가해 확장

        // 최신값 저장
        {
            std::lock_guard<std::mutex> lk(mtx);
            last = s;
        }

        // 콜백
//error_exit: // 공통 저장 로직 (정상/에러 모두)
#if false
        if (cb) cb(s);
#else
        SnapshotCallback temp_cb;
        {
            std::lock_guard<std::mutex> lk(mtx);
            last = s;
            temp_cb = cb;
        }
        if (temp_cb) {
            // 콜백이 오래 걸릴 수 있으므로 락 밖에서 호출
            SnapshotWithMeta meta{
                s,                                      // ✅ RobotSnapshot
                std::chrono::system_clock::now(),     // ✅ 타임스탬프
                polling_sequence++                  // ✅ 시퀀스
            };
            temp_cb(meta);  // ✅ SnapshotWithMeta 전달
        }
#endif
    }

    void loop()
    {
        while (running.load())
        {
            try {
                pollOnce();
            } catch (...) {
                // 예외 방어 (SDK가 예외를 던지지 않는다고 가정해도 안전)
                RobotSnapshot s{};
                s.connected = false;
                s.last_error = "Exception in pollOnce()";
                {
                    std::lock_guard<std::mutex> lk(mtx);
                    last = s;
                }
#if false
                if (cb) cb(s);
#else
                SnapshotCallback temp_cb;
                {
                    std::lock_guard<std::mutex> lk(mtx);
                    last = s;
                    temp_cb = cb;
                }

                if (temp_cb) {
                    SnapshotWithMeta meta{
                        s,
                        std::chrono::system_clock::now(),
                        polling_sequence++  // ✅ 정확히 증가
                    };
                    temp_cb(meta);
                }
#endif
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(opt.poll_period_ms));
        }
    }

    bool queryStaticInfo()
    {
        // fr_test의 버전/하드웨어/펌웨어/설치각도 출력 부분
        char robotModel[64] = {0};
        char webversion[64] = {0};
        char controllerVersion[64] = {0};

        char ctrlBoxBoardversion[128] = {0};
        char driver1version[128] = {0};
        char driver2version[128] = {0};
        char driver3version[128] = {0};
        char driver4version[128] = {0};
        char driver5version[128] = {0};
        char driver6version[128] = {0};
        char endBoardversion[128] = {0};

        int rtn = robot.GetSoftwareVersion(robotModel, webversion, controllerVersion);
        std::printf("robotmodel is: %s, webversion is: %s, controllerVersion is: %s \n\n",
                    robotModel, webversion, controllerVersion);

        rtn = robot.GetHardwareVersion(ctrlBoxBoardversion, driver1version, driver2version, driver3version,
                                       driver4version, driver5version, driver6version, endBoardversion);
        std::printf("GetHardwareversion: %s, %s, %s, %s, %s, %s, %s, %s\n\n",
                    ctrlBoxBoardversion, driver1version, driver2version, driver3version,
                    driver4version, driver5version, driver6version, endBoardversion);

        rtn = robot.GetFirmwareVersion(ctrlBoxBoardversion, driver1version, driver2version, driver3version,
                                       driver4version, driver5version, driver6version, endBoardversion);
        std::printf("GetFirmwareversion: %s, %s, %s, %s, %s, %s, %s, %s\n\n",
                    ctrlBoxBoardversion, driver1version, driver2version, driver3version,
                    driver4version, driver5version, driver6version, endBoardversion);

        float yangle = 0.0f, zangle = 0.0f;
        robot.GetRobotInstallAngle(&yangle, &zangle);
        std::printf("yangle:%f,zangle:%f\n", yangle, zangle);

        (void)rtn;
        return true;
    }
};

FairinoMonitorService::FairinoMonitorService()
    : d(std::make_unique<Impl>())
{
}

FairinoMonitorService::~FairinoMonitorService()
{
    stop();
}

bool FairinoMonitorService::start(const std::string& ip, const Options& opt)
{
    if (d->running.load()) return true;

    d->ip = ip;
    d->opt = opt;

    if (!d->connect()) {
        // 연결 실패 상태 기록
        std::lock_guard<std::mutex> lk(d->mtx);
        d->last.connected = false;
        d->last.last_error = "RPC connect failed";
        return false;
    }

    d->running.store(true);
    d->th = std::thread([this]() { d->loop(); });
    return true;
}

void FairinoMonitorService::stop()
{
    if (!d->running.exchange(false)) return;

    if (d->th.joinable()) d->th.join();
    d->disconnect();

    std::lock_guard<std::mutex> lk(d->mtx);
    d->last.connected = false;
}

bool FairinoMonitorService::isRunning() const
{
    return d->running.load();
}

RobotSnapshot FairinoMonitorService::latest() const
{
    std::lock_guard<std::mutex> lk(d->mtx);
    return d->last;
}

void FairinoMonitorService::setCallback(SnapshotCallback cb)
{
    std::lock_guard<std::mutex> lk(d->mtx);
    d->cb = std::move(cb);
}

bool FairinoMonitorService::queryAndPrintStaticInfo()
{
    // start() 호출 전에 쓰면 연결이 안 되어있으니,
    // start 이후 또는 별도 연결 흐름이 필요합니다.
    // 여기서는 "이미 start로 연결된 상태"를 가정합니다.
    return d->queryStaticInfo();
}

} // namespace monitoring
