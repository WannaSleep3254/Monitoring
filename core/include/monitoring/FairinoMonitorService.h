#pragma once
#include <functional>
#include <memory>
#include <string>
#include <chrono>
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

        Options()
            : enable_reconnect(true)
            , reconnect_timeout_ms(30000)
            , reconnect_interval_ms(500)
            , poll_period_ms(100)
            , robot_id(0)
            , logger_level(1)
        {}
    };
#if false
    // 콜백발생 시간을 모름
    using SnapshotCallback = std::function<void(const RobotSnapshot&)>;
#else
    // 개선안
    /* 미사용
    struct CallbackData {
        const RobotSnapshot& snapshot;
        std::chrono::system_clock::time_point timestamp;
        uint64_t sequence_number;  // 폴링 횟수
    };
    */
    struct SnapshotWithMeta {
        RobotSnapshot snapshot;                          // ✅ 값 복사
        std::chrono::system_clock::time_point timestamp; // 콜백 발생 시간, 로그, DB 저장, GUI 표시용
        std::chrono::steady_clock::time_point steady_timestamp; // 콜백 발생 시간 (steady_clock), polling 주기 계산, stale 판단, 지연 측정용
        uint64_t sequence_number;                        // 폴링 순서
    };

    using SnapshotCallback = std::function<void(const SnapshotWithMeta&)>;


#endif
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

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace monitoring
