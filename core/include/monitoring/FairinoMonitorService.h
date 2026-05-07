#pragma once
#include <functional>
#include <memory>
#include <string>
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

    using SnapshotCallback = std::function<void(const RobotSnapshot&)>;

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
