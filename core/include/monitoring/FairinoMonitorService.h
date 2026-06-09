#pragma once
#include <functional>
#include <memory>
#include <string>
#include <chrono>
#include <cstdint>

#include "monitoring/RobotSnapshot.h"

namespace monitoring {

// fr_test()를 "서비스화"한 클래스
class FairinoMonitorService
{
public:
    struct Options {
        bool enable_reconnect;
        int reconnect_timeout_ms;
        int reconnect_interval_ms;
        int poll_period_ms;
        uint8_t robot_id;
        int logger_level;
        int manual_mode_initial_speed_percent;
        int auto_mode_initial_speed_percent;

        Options()
            : enable_reconnect(true)
            , reconnect_timeout_ms(30000)
            , reconnect_interval_ms(500)
            , poll_period_ms(100)
            , robot_id(0)
            , logger_level(1)
            , manual_mode_initial_speed_percent(30)
            , auto_mode_initial_speed_percent(100)
        {}
    };

    // 개선안
    // SnapshotWithMeta 구조체: RobotSnapshot과 메타데이터(타임스탬프, 시퀀스 번호 등)를 함께 전달하는 구조체
    struct SnapshotWithMeta {
        RobotSnapshot snapshot;                          // ✅ 값 복사
        std::chrono::system_clock::time_point timestamp; // 콜백 발생 시간, 로그, DB 저장, GUI 표시용
        std::chrono::steady_clock::time_point steady_timestamp; // 콜백 발생 시간 (steady_clock), polling 주기 계산, stale 판단, 지연 측정용
        uint64_t sequence_number;                        // 폴링 순서
    };
    // 콜백 타입 정의 (SnapshotWithMeta 전달)
    using SnapshotCallback = std::function<void(const SnapshotWithMeta&)>;

    // 추가: 명령 실행 결과를 상태에 기록하는 구조체, updateCommandResult에서 사용
    struct CommandResult {
        bool ok = false;
        int code = 0;
        std::string name;
        std::string message;
        uint64_t sequence_number = 0;
        std::chrono::system_clock::time_point timestamp{};
    };

    FairinoMonitorService();
    ~FairinoMonitorService();

    FairinoMonitorService(const FairinoMonitorService&) = delete;
    FairinoMonitorService& operator=(const FairinoMonitorService&) = delete;

    // 연결 + 모니터링 루프 시작
    bool start(const std::string& ip, const Options& opt = Options{});

    // 루프 종료 + 연결 해제
    void stop();

    bool isRunning() const;

    // 최신 스냅샷 복사 반환
    RobotSnapshot latest() const;

    // (선택) 업데이트마다 콜백
    void setCallback(SnapshotCallback cb);

    // (선택) fr_test에서 출력하던 버전/하드웨어/설치각도 정보를 한 번에 조회
    bool queryAndPrintStaticInfo();

    // ---- Manual control API ----
    bool startJog(int ref, int axis, int dir, float vel, float acc, float max_dis);
    bool stopJog(int ref);
    bool immStopJog();

    bool robotEnable(bool enable);
    bool setManualMode();
    bool setAutoMode();
    bool setSpeedOverride(int speed_percent);
    bool clearError();
    bool recoverCommunication();
    // ---- Safe convenience wrapper ----
    bool startJointJog(int joint, bool positive, float vel, float acc, float max_deg);
    bool stopJointJog();
    // ---- CommandResult API ----
    CommandResult startJogEx(int ref, int axis, int dir, float vel, float acc, float max_dis);
    CommandResult stopJogEx(int ref);
    CommandResult immStopJogEx();

    CommandResult robotEnableEx(bool enable);
    CommandResult setManualModeEx();
    CommandResult setAutoModeEx();
    CommandResult setSpeedOverrideEx(int speed_percent);
    CommandResult clearErrorEx();
    CommandResult recoverCommunicationEx();
    CommandResult setRobotDoEx(int do_index, bool state);

    CommandResult startJointJogEx(int joint, bool positive, float vel, float acc, float max_deg);
    CommandResult stopJointJogEx();
    CommandResult startWorkspaceJogEx(const std::string& axis, bool positive, float vel, float acc, float max_dis);
    CommandResult stopWorkspaceJogEx();
    CommandResult startBaseJogEx(const std::string& axis, bool positive, float vel, float acc, float max_dis);
    CommandResult stopBaseJogEx();

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace monitoring
