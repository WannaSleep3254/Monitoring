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
    uint64_t command_sequence = 0;

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
#if false
        int rtn = 0;

        JointPos j_deg{};
        rtn = robot.GetActualJointPosDegree(opt.robot_id, &j_deg);
        if (rtn != 0) {
            const std::string msg =
                "GetActualJointPosDegree failed: code=" + std::to_string(rtn);

            s.connected = false;

            // polling error
            s.last_poll_error_code = rtn;
            s.last_poll_error = msg;

            // 기존 호환용 summary
            s.last_error_code = rtn;
            s.last_error = msg;
            return false;
        }

        for (int i = 0; i < 6; ++i) {
            s.joint_pos_deg[i] = j_deg.jPos[i];
        }

        return true;
#else
        DescPose tcpPose{};
        int rtn = robot.GetActualTCPPose(0, &tcpPose);   // SDK 시그니처 확인 필요

        if (rtn == 0) {
            s.tcp_pose[0] = tcpPose.tran.x;
            s.tcp_pose[1] = tcpPose.tran.y;
            s.tcp_pose[2] = tcpPose.tran.z;
            s.tcp_pose[3] = tcpPose.rpy.rx;
            s.tcp_pose[4] = tcpPose.rpy.ry;
            s.tcp_pose[5] = tcpPose.rpy.rz;
        } else {
            s.last_poll_error_code = rtn;
            s.last_poll_error = "GetActualTCPPose failed: code=" + std::to_string(rtn);
        }

        DescPose flangePose{};
        rtn = robot.GetActualToolFlangePose(0, &flangePose);   // SDK 시그니처 확인 필요

        if (rtn == 0) {
            s.flange_pose[0] = flangePose.tran.x;
            s.flange_pose[1] = flangePose.tran.y;
            s.flange_pose[2] = flangePose.tran.z;
            s.flange_pose[3] = flangePose.rpy.rx;
            s.flange_pose[4] = flangePose.rpy.ry;
            s.flange_pose[5] = flangePose.rpy.rz;
        }
        else {
            s.last_poll_error_code = rtn;
            s.last_poll_error = "GetActualToolFlangePose failed: code=" + std::to_string(rtn);
        }

        return rtn == 0;
#endif
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
            // command 상태는 polling snapshot이 덮어쓰지 않도록 보존
            s.last_command_name = last.last_command_name;
            s.last_command_error = last.last_command_error;
            s.last_command_error_code = last.last_command_error_code;
            s.last_command_ok = last.last_command_ok;
            s.command_sequence_number = last.command_sequence_number;
            s.last_command_timestamp = last.last_command_timestamp;

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
        // polling error만 초기화
        s.last_poll_error.clear();
        s.last_poll_error_code = 0;

        // 기존 호환 필드도 polling 에러 기준으로 초기화
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
                {
                    std::lock_guard<std::mutex> lock(state_mutex);
                    s = last;
                }
                s.connected = false;

                s.last_poll_error_code = -1;
                s.last_poll_error = "Exception in pollOnce()";

                s.last_error_code = -1;
                s.last_error = s.last_poll_error;

                publishSnapshot(s);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(opt.poll_period_ms));
        }
    }
    // SDK 명령 실행 결과를 상태에 기록하는 공통 함수, 명령 함수에서 호출

    CommandResult updateCommandResult(int rtn, const std::string& name)
    {
        CommandResult result;
        result.ok = (rtn == 0);
        result.code = rtn;
        result.name = name;
        result.timestamp = std::chrono::system_clock::now();

        if (rtn != 0) {
            result.message = name + " failed: code=" + std::to_string(rtn);
        }

        {
            std::lock_guard<std::mutex> state_lock(state_mutex);

            result.sequence_number = ++command_sequence;

            last.last_command_name = result.name;
            last.last_command_timestamp = result.timestamp;
            last.command_sequence_number = result.sequence_number;
            last.last_command_ok = result.ok;
            last.last_command_error_code = result.code;
            last.last_command_error = result.message;
        }

        return result;
    }
    // SDK 명령 함수 예시: StartJointJog, StopJointJog 등, 내부적으로 startJog/stopJog 호출
    CommandResult startJointJogEx(
        int joint,
        bool positive,
        float vel,
        float acc,
        float max_deg)
    {
        if (joint < 1 || joint > 6) {
            return makeValidationError(
                -10001,
                "startJointJog",
                "Invalid joint index: " + std::to_string(joint)
                );
        }

        if (vel < 0.0f || vel > 100.0f) {
            return makeValidationError(
                -10002,
                "startJointJog",
                "Invalid velocity percent: " + std::to_string(vel)
                );
        }

        if (acc < 0.0f || acc > 100.0f) {
            return makeValidationError(
                -10003,
                "startJointJog",
                "Invalid acceleration percent: " + std::to_string(acc)
                );
        }

        if (max_deg <= 0.0f || max_deg > 5.0f) {
            return makeValidationError(
                -10004,
                "startJointJog",
                "Invalid max degree: " + std::to_string(max_deg)
                );
        }

        return startJogEx(
            0,                  // StartJOG ref: Joint
            joint,              // J1~J6
            positive ? 1 : 0,   // + / -
            vel,
            acc,
            max_deg
            );
    }

    bool startJointJog(
        int joint,
        bool positive,
        float vel,
        float acc,
        float max_deg)
    {
        return startJointJogEx(joint, positive, vel, acc, max_deg).ok;
    }

    CommandResult stopJointJogEx()
    {
        return stopJogEx(1);    // StopJOG ref: Joint stop
    }

    bool stopJointJog()
    {
        return stopJointJogEx().ok;
    }

    // SDK 명령 함수 예시: StartJOG, StopJOG, ImmStopJOG, RobotEnable, Mode, ResetAllError 등
    CommandResult startJogEx(int ref, int axis, int dir, float vel, float acc, float max_dis)
    {
        int rtn = 0;

        {
            std::lock_guard<std::mutex> lock(sdk_mutex);

            rtn = robot.StartJOG(
                static_cast<uint8_t>(ref),
                static_cast<uint8_t>(axis),
                static_cast<uint8_t>(dir),
                vel,
                acc,
                max_dis
                );
        }

        return updateCommandResult(rtn, "StartJOG");
    }

    bool startJog(int ref, int axis, int dir, float vel, float acc, float max_dis)
    {
        return startJogEx(ref, axis, dir, vel, acc, max_dis).ok;
    }

    CommandResult stopJogEx(int ref)
    {
        int rtn = 0;

        {
            std::lock_guard<std::mutex> lock(sdk_mutex);
            rtn = robot.StopJOG(static_cast<uint8_t>(ref));
        }

        return updateCommandResult(rtn, "StopJOG");
    }

    bool stopJog(int ref)
    {
        return stopJogEx(ref).ok;
    }

    CommandResult immStopJogEx()
    {
        int rtn = 0;

        {
            std::lock_guard<std::mutex> lock(sdk_mutex);
            rtn = robot.ImmStopJOG();
        }

        return updateCommandResult(rtn, "ImmStopJOG");
    }

    bool immStopJog()
    {
        return immStopJogEx().ok;
    }

    CommandResult robotEnableEx(bool enable)
    {
        int rtn = 0;

        {
            std::lock_guard<std::mutex> lock(sdk_mutex);
            rtn = robot.RobotEnable(enable ? 1 : 0);
        }

        return updateCommandResult(rtn, "RobotEnable");
    }

    bool robotEnable(bool enable)
    {
        return robotEnableEx(enable).ok;
    }

    CommandResult makeValidationError(
        int code,
        const std::string& name,
        const std::string& message)
    {
        CommandResult result;
        result.ok = false;
        result.code = code;
        result.name = name;
        result.message = message;
        result.timestamp = std::chrono::system_clock::now();

        {
            std::lock_guard<std::mutex> state_lock(state_mutex);

            result.sequence_number = ++command_sequence;

            last.last_command_name = result.name;
            last.last_command_timestamp = result.timestamp;
            last.command_sequence_number = result.sequence_number;
            last.last_command_ok = false;
            last.last_command_error_code = result.code;
            last.last_command_error = result.message;
        }

        return result;
    }

    CommandResult setManualModeEx()
    {
        int rtn = 0;

        {
            std::lock_guard<std::mutex> lock(sdk_mutex);
            rtn = robot.Mode(1);
        }

        return updateCommandResult(rtn, "Mode(Manual)");
    }

    bool setManualMode()
    {
        return setManualModeEx().ok;
    }

    CommandResult setAutoModeEx()
    {
        int rtn = 0;

        {
            std::lock_guard<std::mutex> lock(sdk_mutex);
            rtn = robot.Mode(0);
        }

        return updateCommandResult(rtn, "Mode(Auto)");
    }

    bool setAutoMode()
    {
        return setAutoModeEx().ok;
    }

    CommandResult clearErrorEx()
    {
        int rtn = 0;

        {
            std::lock_guard<std::mutex> lock(sdk_mutex);
            rtn = robot.ResetAllError();
        }

        return updateCommandResult(rtn, "ResetAllError");
    }

    bool clearError()
    {
        return clearErrorEx().ok;
    }
    // fr_test의 queryStaticInfo() 내용을 그대로 옮김, 필요 시 start()에서 호출하거나 외부에 공개하는 함수로 분리 가능
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

bool FairinoMonitorService::startJointJog(
    int joint,
    bool positive,
    float vel,
    float acc,
    float max_deg)
{
    return d->startJointJog(joint, positive, vel, acc, max_deg);
}

bool FairinoMonitorService::stopJointJog()
{
    return d->stopJointJog();
}

bool FairinoMonitorService::startJog(int ref, int axis, int dir, float vel, float acc, float max_dis)
{
    return d->startJog(ref, axis, dir, vel, acc, max_dis);
}

bool FairinoMonitorService::stopJog(int ref)
{
    return d->stopJog(ref);
}

bool FairinoMonitorService::immStopJog()
{
    return d->immStopJog();
}

bool FairinoMonitorService::robotEnable(bool enable)
{
    return d->robotEnable(enable);
}

bool FairinoMonitorService::setManualMode()
{
    return d->setManualMode();
}

bool FairinoMonitorService::setAutoMode()
{
    return d->setAutoMode();
}

bool FairinoMonitorService::clearError()
{
    return d->clearError();
}

FairinoMonitorService::CommandResult
FairinoMonitorService::startJointJogEx(
    int joint,
    bool positive,
    float vel,
    float acc,
    float max_deg)
{
    return d->startJointJogEx(joint, positive, vel, acc, max_deg);
}

FairinoMonitorService::CommandResult
FairinoMonitorService::stopJointJogEx()
{
    return d->stopJointJogEx();
}

FairinoMonitorService::CommandResult
FairinoMonitorService::startJogEx(
    int ref,
    int axis,
    int dir,
    float vel,
    float acc,
    float max_dis)
{
    return d->startJogEx(ref, axis, dir, vel, acc, max_dis);
}

FairinoMonitorService::CommandResult
FairinoMonitorService::stopJogEx(int ref)
{
    return d->stopJogEx(ref);
}

FairinoMonitorService::CommandResult
FairinoMonitorService::immStopJogEx()
{
    return d->immStopJogEx();
}

FairinoMonitorService::CommandResult
FairinoMonitorService::robotEnableEx(bool enable)
{
    return d->robotEnableEx(enable);
}

FairinoMonitorService::CommandResult
FairinoMonitorService::setManualModeEx()
{
    return d->setManualModeEx();
}

FairinoMonitorService::CommandResult
FairinoMonitorService::setAutoModeEx()
{
    return d->setAutoModeEx();
}

FairinoMonitorService::CommandResult
FairinoMonitorService::clearErrorEx()
{
    return d->clearErrorEx();
}

bool FairinoMonitorService::queryAndPrintStaticInfo()
{
    // start() 호출 전에 쓰면 연결이 안 되어있으니,
    // start 이후 또는 별도 연결 흐름이 필요합니다.
    // 여기서는 "이미 start로 연결된 상태"를 가정합니다.
    return d->queryStaticInfo();
}

} // namespace monitoring
