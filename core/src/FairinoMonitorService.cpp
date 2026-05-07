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

//    mutable std::mutex mtx;
    std::mutex sdk_mutex;           // SDK 호출 보호
    mutable std::mutex state_mutex; // latest/callback 보호

    RobotSnapshot last{};
    SnapshotCallback cb;
    //추가
    uint64_t polling_sequence = 0;

    std::chrono::steady_clock::time_point last_fast_poll{};
    std::chrono::steady_clock::time_point last_torque_poll{};
    std::chrono::steady_clock::time_point last_temp_poll{};
    std::chrono::steady_clock::time_point last_extra_poll{};

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
        std::lock_guard<std::mutex> lock(sdk_mutex);

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

        auto now = std::chrono::steady_clock::now();
        last_fast_poll = now;
        last_torque_poll = now;
        last_temp_poll = now;
        last_extra_poll = now;

        return true;
    }

    void disconnect()
    {
        std::lock_guard<std::mutex> lock(sdk_mutex);
        robot.CloseRPC();
    }
    // 빠르게 읽어야 하는 상태 (joint pos 등)만 별도 함수로 분리, pollOnce에서 필요 시 호출
    bool readFastState(RobotSnapshot& s)
    {
        int rtn = 0;

        JointPos j_deg{};
        rtn = robot.GetActualJointPosDegree(opt.robot_id, &j_deg);
        if (rtn != 0) {
            s.connected = false;
            s.last_error_code = rtn;
            s.last_error = "GetActualJointPosDegree failed: code=" + std::to_string(rtn);
            return false;
        }

        for (int i = 0; i < 6; ++i) {
            s.joint_pos_deg[i] = j_deg.jPos[i];
        }

        return true;
    }
    // 온도/드라이버 토크는 상대적으로 느리게 변할 것으로 예상되어 별도 함수로 분리, pollOnce에서 필요 시 호출
    void readDriverTemperature(RobotSnapshot& s)
    {
        double temp[6] = {0};

        int rtn = robot.GetJointDriverTemperature(temp);
        if (rtn != 0) {
            s.temperature_valid = false;
            s.last_temperature_error = rtn;
            return;
        }

        for (int i = 0; i < 6; ++i) {
            s.driver_temperature[i] = temp[i];
        }

        s.temperature_valid = true;
        s.last_temperature_error = 0;
    }
    // 드라이버 토크 읽기도 별도 함수로 분리, 필요 시 pollOnce에서 호출
    void readDriverTorque(RobotSnapshot& s)
    {
        double torque[6] = {0};

        int rtn = robot.GetJointDriverTorque(torque);
        if (rtn != 0) {
            s.driver_torque_valid = false;
            s.last_driver_torque_error = rtn;
            return;
        }

        for (int i = 0; i < 6; ++i) {
            s.driver_torque[i] = torque[i];
        }

        s.driver_torque_valid = true;
        s.last_driver_torque_error = 0;
    }
    // 기타 상태 읽기도 별도 함수로 분리, 필요 시 pollOnce에서 호출
    void readExtraStatus(RobotSnapshot& s)
    {
        uint8_t motionDone = 0;
        if (robot.GetRobotMotionDone(&motionDone) == 0) {
            s.motion_done = motionDone;
        }

        int len = 0;
        if (robot.GetMotionQueueLength(&len) == 0) {
            s.motion_queue_len = len;
        }

        uint8_t emergState = 0;
        if (robot.GetRobotEmergencyStopState(&emergState) == 0) {
            s.emergency_stop = emergState;
        }

        int comstate = 0;
        if (robot.GetSDKComState(&comstate) == 0) {
            s.sdk_com_state = comstate;
        }

        uint8_t si0 = 0;
        uint8_t si1 = 0;
        if (robot.GetSafetyStopState(&si0, &si1) == 0) {
            s.safety_si0 = si0;
            s.safety_si1 = si1;
        }
    }

    // 스냅샷 저장 + 콜백 호출 (공통 로직 분리, pollOnce과 에러 처리에서 모두 사용)
    void publishSnapshot(RobotSnapshot s)
    {
        const auto system_now = std::chrono::system_clock::now();
        const auto steady_now = std::chrono::steady_clock::now();

        SnapshotCallback temp_cb;

        {
            std::lock_guard<std::mutex> lock(state_mutex);

            const auto seq = polling_sequence++;

            s.system_timestamp = system_now;
            s.steady_timestamp = steady_now;
            s.sequence_number = seq;

            last = s;
            temp_cb = cb;
        }

        if (temp_cb) {
            SnapshotWithMeta meta{
                s,
                s.system_timestamp,
                s.steady_timestamp,
                s.sequence_number
            };

            temp_cb(meta);
        }
    }
    // fr_test의 while(true) 내용을 1회 실행으로 분리
    void pollOnce()
    {
        RobotSnapshot s{};
        // 저주기 항목이 매 cycle마다 초기화되지 않도록 이전 snapshot을 기반으로 시작
        {
            std::lock_guard<std::mutex> lock(state_mutex);
            s = last;
        }
        s.connected = true;
        s.last_error.clear();
        s.last_error_code = 0;

        const auto now = std::chrono::steady_clock::now();

        {
            std::unique_lock<std::mutex> sdk_lock(sdk_mutex, std::try_to_lock);
            if (!sdk_lock.owns_lock()) {
                return;
            }
            // Fast state는 매 polling마다 읽음
            const bool fast_ok = readFastState(s);

            if (!fast_ok) {
                // readFastState() 내부에서 connected=false, last_error 설정한다고 가정
                // SDK lock은 이 블록이 끝나면 자동 해제
            } else {
                // Driver torque: 200ms 주기 예시
                if (now - last_torque_poll >= std::chrono::milliseconds(200)) {
                    readDriverTorque(s);
                    last_torque_poll = now;
                }

                // Driver temperature: 1초 주기 예시
                if (now - last_temp_poll >= std::chrono::seconds(1)) {
                    readDriverTemperature(s);
                    last_temp_poll = now;
                }

                // Extra status: 500ms 주기 예시
                if (now - last_extra_poll >= std::chrono::milliseconds(500)) {
                    readExtraStatus(s);
                    last_extra_poll = now;
                }
            }
        }
        // 반드시 sdk_mutex 해제 후 호출
        publishSnapshot(s);
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

                publishSnapshot(s);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(opt.poll_period_ms));
        }
    }


    bool startJog(int ref, int axis, int dir, float vel, float acc, float max_dis)
    {
        std::lock_guard<std::mutex> lock(sdk_mutex);

        const int rtn = robot.StartJOG(
            static_cast<uint8_t>(ref),
            static_cast<uint8_t>(axis),
            static_cast<uint8_t>(dir),
            vel,
            acc,
            max_dis
            );

        if (rtn != 0) {
            std::lock_guard<std::mutex> state_lock(state_mutex);
            last.last_error_code = rtn;
            last.last_error = "StartJOG failed: code=" + std::to_string(rtn);
        }

        return rtn == 0;
    }

    bool stopJog(int ref)
    {
        std::lock_guard<std::mutex> lock(sdk_mutex);

        const int rtn = robot.StopJOG(static_cast<uint8_t>(ref));

        if (rtn != 0) {
            std::lock_guard<std::mutex> state_lock(state_mutex);
            last.last_error_code = rtn;
            last.last_error = "StopJOG failed: code=" + std::to_string(rtn);
        }

        return rtn == 0;
    }

    bool immStopJog()
    {
        std::lock_guard<std::mutex> lock(sdk_mutex);

        const int rtn = robot.ImmStopJOG();

        if (rtn != 0) {
            std::lock_guard<std::mutex> state_lock(state_mutex);
            last.last_error_code = rtn;
            last.last_error = "ImmStopJOG failed: code=" + std::to_string(rtn);
        }

        return rtn == 0;
    }

    bool robotEnable(bool enable)
    {
        std::lock_guard<std::mutex> lock(sdk_mutex);

        const int rtn = robot.RobotEnable(enable ? 1 : 0);

        if (rtn != 0) {
            std::lock_guard<std::mutex> state_lock(state_mutex);
            last.last_error_code = rtn;
            last.last_error = std::string("RobotEnable failed: code=") + std::to_string(rtn);
        }

        return rtn == 0;
    }

    bool setManualMode()
    {
        std::lock_guard<std::mutex> lock(sdk_mutex);

        // SDK 기준: 0 = Auto, 1 = Manual
        const int rtn = robot.Mode(1);

        if (rtn != 0) {
            std::lock_guard<std::mutex> state_lock(state_mutex);
            last.last_error_code = rtn;
            last.last_error = "Mode(Manual) failed: code=" + std::to_string(rtn);
        }

        return rtn == 0;
    }

    bool setAutoMode()
    {
        std::lock_guard<std::mutex> lock(sdk_mutex);

        // SDK 기준: 0 = Auto, 1 = Manual
        const int rtn = robot.Mode(0);

        if (rtn != 0) {
            std::lock_guard<std::mutex> state_lock(state_mutex);
            last.last_error_code = rtn;
            last.last_error = "Mode(Auto) failed: code=" + std::to_string(rtn);
        }

        return rtn == 0;
    }

    bool clearError()
    {
        std::lock_guard<std::mutex> lock(sdk_mutex);

        const int rtn = robot.ResetAllError();

        if (rtn != 0) {
            std::lock_guard<std::mutex> state_lock(state_mutex);
            last.last_error_code = rtn;
            last.last_error = "ResetAllError failed: code=" + std::to_string(rtn);
        }

        return rtn == 0;
    }

    bool queryStaticInfo()
    {
        std::lock_guard<std::mutex> lock(sdk_mutex);
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
        //std::lock_guard<std::mutex> lk(d->mtx);
        std::lock_guard<std::mutex> lock(d->state_mutex);
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

    //std::lock_guard<std::mutex> lk(d->mtx);
    std::lock_guard<std::mutex> lock(d->state_mutex);
    d->last.connected = false;
}

bool FairinoMonitorService::isRunning() const
{
    return d->running.load();
}

RobotSnapshot FairinoMonitorService::latest() const
{
    //std::lock_guard<std::mutex> lk(d->mtx);
    std::lock_guard<std::mutex> lock(d->state_mutex);
    return d->last;
}

void FairinoMonitorService::setCallback(SnapshotCallback cb)
{
    //std::lock_guard<std::mutex> lk(d->mtx);
    std::lock_guard<std::mutex> lock(d->state_mutex);
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
