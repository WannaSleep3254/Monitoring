#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <chrono>

namespace monitoring {

struct RobotSnapshot
{
    bool connected = false;

    // ---- Joint ----
    std::array<double, 6> joint_pos_deg{};     // GetActualJointPosDegree
    std::array<float,  6> joint_speed_deg{};   // GetActualJointSpeedsDegree
    std::array<float,  6> joint_acc_deg{};     // GetActualJointAccDegree

    // ---- TCP speed ----
    float target_tcp_speed_comp = 0.0f; // GetTargetTCPCompositeSpeed(tcp)
    float target_ori_speed_comp = 0.0f; // GetTargetTCPCompositeSpeed(ori)
    float actual_tcp_speed_comp = 0.0f; // GetActualTCPCompositeSpeed(tcp)
    float actual_ori_speed_comp = 0.0f; // GetActualTCPCompositeSpeed(ori)

    std::array<float, 6> target_tcp_speed_6{}; // GetTargetTCPSpeed
    std::array<float, 6> actual_tcp_speed_6{}; // GetActualTCPSpeed

    // ---- Pose ----
    std::array<double, 6> tcp_pose{};     // x,y,z,rx,ry,rz
    std::array<double, 6> flange_pose{};  // x,y,z,rx,ry,rz

    // ---- Frames ----
    int tcp_num = 0;   // GetActualTCPNum
    int wobj_num = 0;  // GetActualWObjNum

    // ---- Torque / temp ----
    std::array<float, 6> joint_torque{};        // GetJointTorques (float)
    std::array<double,6> driver_temperature{};  // GetJointDriverTemperature (double)
    std::array<double,6> driver_torque{};       // GetJointDriverTorque (double)

    bool temperature_valid = false;
    bool driver_torque_valid = false;

    int last_temperature_error = 0;
    int last_driver_torque_error = 0;

    // ---- System / state ----
    float system_clock_ms = 0.0f;  // GetSystemClock
    int joints_config = 0;         // GetRobotCurJointsConfig
    uint8_t motion_done = 0;       // GetRobotMotionDone
    int motion_queue_len = 0;      // GetMotionQueueLength
    uint8_t emergency_stop = 0;    // GetRobotEmergencyStopState
    int sdk_com_state = 0;         // GetSDKComState
    uint8_t safety_si0 = 0;        // GetSafetyStopState
    uint8_t safety_si1 = 0;

    // ---- Meta ----
    std::string last_error;        // 에러 텍스트(필요 시)
    int last_error_code = 0;       // 에러 코드(필요 시)

    // ---- Polling error ----
    std::string last_poll_error;
    int last_poll_error_code = 0;

    // ---- Recovery state ----
    bool reconnecting = false;
    int reconnect_count = 0;
    std::string last_recovery_message;

    // ---- Command result ----
    std::string last_command_name;
    std::string last_command_error;
    int last_command_error_code = 0;
    bool last_command_ok = true;
    uint64_t command_sequence_number = 0;
    std::chrono::system_clock::time_point last_command_timestamp{};


    uint64_t sequence_number = 0;
    std::chrono::system_clock::time_point system_timestamp{};
    std::chrono::steady_clock::time_point steady_timestamp{};
};

} // namespace monitoring
