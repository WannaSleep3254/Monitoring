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
    int consecutive_fast_poll_failures = 0;
    int consecutive_command_failures = 0;
    bool sdk_session_disconnected = false;
    bool reconnecting = false;
    int reconnect_count = 0;
    std::string last_recovery_message;
    int64_t last_recovery_epoch_ms = 0;
    int recovery_event_id = 0;

    std::chrono::steady_clock::time_point last_fast_poll{};
    std::chrono::steady_clock::time_point last_torque_poll{};
    std::chrono::steady_clock::time_point last_temp_poll{};
    std::chrono::steady_clock::time_point last_extra_poll{};
    std::chrono::steady_clock::time_point last_reconnect_attempt{};

    static constexpr int kDisconnectFailureThreshold = 3;

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

    static bool isCommunicationError(int code)
    {
        return code == -2; // ERR_SOCKET_COM_FAILED
    }

    static int64_t currentEpochMs()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::system_clock::now().time_since_epoch())
            .count();
    }

    void markRecoveryEvent(const std::string& message)
    {
        last_recovery_message = message;
        last_recovery_epoch_ms = currentEpochMs();
        ++recovery_event_id;
    }

    bool connectLocked()
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

        auto now = std::chrono::steady_clock::now();
        last_fast_poll = now;
        last_torque_poll = now;
        last_temp_poll = now;
        last_extra_poll = now;
        consecutive_fast_poll_failures = 0;
        sdk_session_disconnected = false;
        reconnecting = false;
        last_recovery_message.clear();
        last_recovery_epoch_ms = 0;
        recovery_event_id = 0;

        return true;
    }

    bool connect()
    {
        std::lock_guard<std::mutex> lock(sdk_mutex);
        return connectLocked();
    }

    bool reconnectLocked(const std::string& reason,
                         bool force = false,
                         const std::string& successMessage = "reconnect success")
    {
        const auto now = std::chrono::steady_clock::now();

        if (!force &&
            last_reconnect_attempt.time_since_epoch().count() != 0 &&
            now - last_reconnect_attempt < std::chrono::seconds(3)) {
            return false;
        }

        last_reconnect_attempt = now;
        reconnecting = true;
        ++reconnect_count;
        last_recovery_message = "reconnect requested";

        std::cerr << "[FairinoMonitorService] reconnect requested: "
                  << reason << "\n";

        robot.CloseRPC();
        const bool ok = connectLocked();

        if (ok) {
            consecutive_fast_poll_failures = 0;
            reconnecting = false;
            markRecoveryEvent(successMessage);
            std::cerr << "[FairinoMonitorService] reconnect succeeded\n";
        } else {
            sdk_session_disconnected = true;
            reconnecting = true;
            last_recovery_message = "reconnect failed";
            std::cerr << "[FairinoMonitorService] reconnect failed\n";
        }

        return ok;
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

        auto setPollError = [&](int code, const std::string& msg)
        {
            ++consecutive_fast_poll_failures;

            s.last_poll_error_code = code;
            s.last_poll_error = msg;

            // 기존 호환용 summary
            s.last_error_code = code;
            s.last_error = msg;

            if (consecutive_fast_poll_failures >= kDisconnectFailureThreshold) {
                s.connected = false;
                sdk_session_disconnected = true;
                s.reconnecting = reconnecting;
                s.reconnect_count = reconnect_count;
                s.last_recovery_message = "fast poll failed";
                s.last_recovery_epoch_ms = last_recovery_epoch_ms;
                s.recovery_event_id = recovery_event_id;
                last_recovery_message = s.last_recovery_message;

                std::cerr << "[FairinoMonitorService] fast poll disconnected after "
                          << consecutive_fast_poll_failures
                          << " failures: " << msg << "\n";
            }
        };

        auto clearPollError = [&]()
        {
            if (consecutive_fast_poll_failures > 0 || sdk_session_disconnected) {
                std::cerr << "[FairinoMonitorService] fast poll recovered after "
                          << consecutive_fast_poll_failures
                          << " failures\n";
                markRecoveryEvent("fast poll recovered");
            }

            consecutive_fast_poll_failures = 0;
            sdk_session_disconnected = false;
            reconnecting = false;
            s.connected = true;
            s.reconnecting = false;
            s.reconnect_count = reconnect_count;
            s.last_recovery_message = last_recovery_message;
            s.last_recovery_epoch_ms = last_recovery_epoch_ms;
            s.recovery_event_id = recovery_event_id;

            s.last_poll_error_code = 0;
            s.last_poll_error.clear();

            s.last_error_code = 0;
            s.last_error.clear();
        };

        auto copyDescPoseToArray = [](const DescPose& pose, auto& out)
        {
            out[0] = pose.tran.x;
            out[1] = pose.tran.y;
            out[2] = pose.tran.z;
            out[3] = pose.rpy.rx;
            out[4] = pose.rpy.ry;
            out[5] = pose.rpy.rz;
        };

        // ------------------------------------------------------------
        // 1. Joint position [deg] - 필수
        // ------------------------------------------------------------
        JointPos j_deg{};
        rtn = robot.GetActualJointPosDegree(opt.robot_id, &j_deg);

        if (rtn != 0) {
            const std::string msg =
                "GetActualJointPosDegree failed: code=" + std::to_string(rtn);

            setPollError(rtn, msg);
            if (isCommunicationError(rtn) &&
                consecutive_fast_poll_failures >= kDisconnectFailureThreshold) {
                reconnectLocked(msg);
                s.reconnecting = reconnecting;
                s.reconnect_count = reconnect_count;
                s.last_recovery_message = last_recovery_message;
                s.last_recovery_epoch_ms = last_recovery_epoch_ms;
                s.recovery_event_id = recovery_event_id;
            }
            return false;
        }

        for (int i = 0; i < 6; ++i) {
            s.joint_pos_deg[i] = j_deg.jPos[i];
        }

        // ------------------------------------------------------------
        // 2. TCP pose [x, y, z, rx, ry, rz] - 필수
        // ------------------------------------------------------------
        DescPose tcpPose{};
        rtn = robot.GetActualTCPPose(0, &tcpPose);

        if (rtn != 0) {
            const std::string msg =
                "GetActualTCPPose failed: code=" + std::to_string(rtn);

            setPollError(rtn, msg);
            if (isCommunicationError(rtn) &&
                consecutive_fast_poll_failures >= kDisconnectFailureThreshold) {
                reconnectLocked(msg);
                s.reconnecting = reconnecting;
                s.reconnect_count = reconnect_count;
                s.last_recovery_message = last_recovery_message;
                s.last_recovery_epoch_ms = last_recovery_epoch_ms;
                s.recovery_event_id = recovery_event_id;
            }
            return false;
        }

        copyDescPoseToArray(tcpPose, s.tcp_pose);

        // ------------------------------------------------------------
        // 3. 여기까지 성공이면 joint/tcp 전송 가능 상태
        // ------------------------------------------------------------
        clearPollError();

        // ------------------------------------------------------------
        // 4. Flange pose - 선택 데이터
        // ------------------------------------------------------------
        DescPose flangePose{};
        rtn = robot.GetActualToolFlangePose(0, &flangePose);

        if (rtn == 0) {
            copyDescPoseToArray(flangePose, s.flange_pose);
        }
        else {
            // Flange는 선택 데이터로 처리.
            // joint/tcp 전송 성공 상태는 유지한다.
            //
            // 필요 시 별도 warning 필드가 있으면 여기에 저장하는 것이 좋음:
            // s.last_warning_code = rtn;
            // s.last_warning = "GetActualToolFlangePose failed: code=" + std::to_string(rtn);
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

        const int diCount = opt.robot_id == 1 ? 2 : 4;
        bool diOk = true;

        for (int i = 0; i < diCount; ++i) {
            uint8_t di = 0;
            const int rtn = robot.GetDI(i, 1, &di);
            if (rtn != 0) {
                s.robot_di_valid = false;
                s.last_robot_di_error = rtn;
                diOk = false;
                break;
            }

            s.robot_di[static_cast<size_t>(i)] = di;
        }

        if (diOk) {
            s.robot_di_valid = true;
            s.last_robot_di_error = 0;
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
            s.reconnecting = reconnecting;
            s.reconnect_count = reconnect_count;
            s.last_recovery_epoch_ms = last_recovery_epoch_ms;
            s.recovery_event_id = recovery_event_id;
            if (s.last_recovery_message.empty())
                s.last_recovery_message = last_recovery_message;
            if (s.last_recovery_message.empty())
                s.last_recovery_message = last.last_recovery_message;
            if (s.last_recovery_epoch_ms == 0)
                s.last_recovery_epoch_ms = last.last_recovery_epoch_ms;
            if (s.recovery_event_id == 0)
                s.recovery_event_id = last.recovery_event_id;

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
            ++consecutive_command_failures;

            std::cerr << "[FairinoMonitorService] command failed: "
                      << result.message
                      << ", consecutive=" << consecutive_command_failures
                      << "\n";
        } else {
            if (consecutive_command_failures > 0) {
                std::cerr << "[FairinoMonitorService] command recovered: "
                          << name
                          << ", previousFailures=" << consecutive_command_failures
                          << "\n";
            }

            consecutive_command_failures = 0;
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
            last.reconnecting = reconnecting;
            last.reconnect_count = reconnect_count;
            last.last_recovery_message = last_recovery_message;
            last.last_recovery_epoch_ms = last_recovery_epoch_ms;
            last.recovery_event_id = recovery_event_id;
        }

        return result;
    }

    template <typename Fn>
    CommandResult executeSdkCommandWithReconnect(const std::string& name, Fn fn)
    {
        int rtn = 0;

        {
            std::lock_guard<std::mutex> lock(sdk_mutex);

            rtn = fn();

            if (isCommunicationError(rtn) &&
                reconnectLocked(name + " failed: code=" + std::to_string(rtn),
                                true,
                                "command retry after reconnect")) {
                rtn = fn();
            }
        }

        return updateCommandResult(rtn, name);
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

        if (max_deg <= 0.0f || max_deg > 300.0f) {
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

    CommandResult startWorkspaceJogEx(
        const std::string& axis,
        bool positive,
        float vel,
        float acc,
        float max_dis)
    {
        int axisIndex = 0;

        if (axis == "X") {
            axisIndex = 1;
        } else if (axis == "Y") {
            axisIndex = 2;
        } else if (axis == "Z") {
            axisIndex = 3;
        } else if (axis == "RX") {
            axisIndex = 4;
        } else if (axis == "RY") {
            axisIndex = 5;
        } else if (axis == "RZ") {
            axisIndex = 6;
        } else {
            return makeValidationError(
                -10011,
                "startWorkspaceJog",
                "Invalid workspace jog axis: " + axis
                );
        }

        if (vel < 0.0f || vel > 100.0f) {
            return makeValidationError(
                -10012,
                "startWorkspaceJog",
                "Invalid velocity percent: " + std::to_string(vel)
                );
        }

        if (acc < 0.0f || acc > 100.0f) {
            return makeValidationError(
                -10013,
                "startWorkspaceJog",
                "Invalid acceleration percent: " + std::to_string(acc)
                );
        }

        if (max_dis <= 0.0f || max_dis > 300.0f) {
            return makeValidationError(
                -10014,
                "startWorkspaceJog",
                "Invalid max distance/degree: " + std::to_string(max_dis)
                );
        }

        return startJogEx(
            8,                       // StartJOG ref: workpiece/workspace coordinate system
            axisIndex,               // X/Y/Z/RX/RY/RZ
            positive ? 1 : 0,        // + / -
            vel,
            acc,
            max_dis
            );
    }

    CommandResult stopWorkspaceJogEx()
    {
        return stopJogEx(9);    // StopJOG ref: workpiece/workspace coordinate system
    }

    CommandResult startBaseJogEx(
        const std::string& axis,
        bool positive,
        float vel,
        float acc,
        float max_dis)
    {
        return startWorkspaceJogEx(axis, positive, vel, acc, max_dis);
    }

    CommandResult stopBaseJogEx()
    {
        return stopWorkspaceJogEx();
    }

    // SDK 명령 함수 예시: StartJOG, StopJOG, ImmStopJOG, RobotEnable, Mode, ResetAllError 등
    CommandResult startJogEx(int ref, int axis, int dir, float vel, float acc, float max_dis)
    {
        return executeSdkCommandWithReconnect("StartJOG", [&]() {
            return robot.StartJOG(
                static_cast<uint8_t>(ref),
                static_cast<uint8_t>(axis),
                static_cast<uint8_t>(dir),
                vel,
                acc,
                max_dis
                );
        });
    }

    bool startJog(int ref, int axis, int dir, float vel, float acc, float max_dis)
    {
        return startJogEx(ref, axis, dir, vel, acc, max_dis).ok;
    }

    CommandResult stopJogEx(int ref)
    {
        return executeSdkCommandWithReconnect("StopJOG", [&]() {
            return robot.StopJOG(static_cast<uint8_t>(ref));
        });
    }

    bool stopJog(int ref)
    {
        return stopJogEx(ref).ok;
    }

    CommandResult immStopJogEx()
    {
        return executeSdkCommandWithReconnect("ImmStopJOG", [&]() {
            return robot.ImmStopJOG();
        });
    }

    bool immStopJog()
    {
        return immStopJogEx().ok;
    }

    CommandResult robotEnableEx(bool enable)
    {
        return executeSdkCommandWithReconnect("RobotEnable", [&]() {
            return robot.RobotEnable(enable ? 1 : 0);
        });
    }

    bool robotEnable(bool enable)
    {
        return robotEnableEx(enable).ok;
    }

    CommandResult setRobotDoEx(int do_index, bool state)
    {
        if (do_index < 0 || do_index > 15) {
            return makeValidationError(
                -11005,
                "SetDO",
                "Invalid robot DO index: " + std::to_string(do_index)
                );
        }

        return executeSdkCommandWithReconnect("SetDO", [&]() {
            return robot.SetDO(
                do_index,
                state ? 1 : 0,
                0,  // smooth: off
                1   // SDK block flag: non-blocking
                );
        });
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
        return executeSdkCommandWithReconnect("Mode(Manual)", [&]() {
            return robot.Mode(1);
        });
    }

    bool setManualMode()
    {
        return setManualModeEx().ok;
    }

    CommandResult setAutoModeEx()
    {
        return executeSdkCommandWithReconnect("Mode(Auto)", [&]() {
            return robot.Mode(0);
        });
    }

    bool setAutoMode()
    {
        return setAutoModeEx().ok;
    }

    CommandResult clearErrorEx()
    {
        return executeSdkCommandWithReconnect("ResetAllError", [&]() {
            return robot.ResetAllError();
        });
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
FairinoMonitorService::startWorkspaceJogEx(
    const std::string& axis,
    bool positive,
    float vel,
    float acc,
    float max_dis)
{
    return d->startWorkspaceJogEx(axis, positive, vel, acc, max_dis);
}

FairinoMonitorService::CommandResult
FairinoMonitorService::stopWorkspaceJogEx()
{
    return d->stopWorkspaceJogEx();
}

FairinoMonitorService::CommandResult
FairinoMonitorService::startBaseJogEx(
    const std::string& axis,
    bool positive,
    float vel,
    float acc,
    float max_dis)
{
    return d->startBaseJogEx(axis, positive, vel, acc, max_dis);
}

FairinoMonitorService::CommandResult
FairinoMonitorService::stopBaseJogEx()
{
    return d->stopBaseJogEx();
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

FairinoMonitorService::CommandResult
FairinoMonitorService::setRobotDoEx(int do_index, bool state)
{
    return d->setRobotDoEx(do_index, state);
}

bool FairinoMonitorService::queryAndPrintStaticInfo()
{
    // start() 호출 전에 쓰면 연결이 안 되어있으니,
    // start 이후 또는 별도 연결 흐름이 필요합니다.
    // 여기서는 "이미 start로 연결된 상태"를 가정합니다.
    return d->queryStaticInfo();
}

} // namespace monitoring
