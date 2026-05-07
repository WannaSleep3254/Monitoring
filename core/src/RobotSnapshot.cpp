#include "monitoring/RobotSnapshot.h"

#include "robot.h"
#include <iostream>

int fr_test()
{
    std::cout << "Fairino SDK Test Start" << std::endl;

    ROBOT_STATE_PKG pkg = {};
    FRRobot robot;

    robot.LoggerInit();
    robot.SetLoggerLevel(1);
    int rtn;
    // SDK 버전 체크
    char sdkVersion[64] = {0,};
    rtn = robot.GetSDKVersion(sdkVersion);
    if(rtn==0)
        std::cout << "SDK-Version : "<< sdkVersion << std::endl<< std::endl;

    // 연결
    rtn = robot.RPC("192.168.57.121");
    std::cout << "Connect : "<< (rtn==0?"true":"false") << std::endl;
    if (rtn != 0)
    {
        return -1;
    }
    robot.SetReConnectParam(true, 30000, 500);

#if false
    // 로봇 버전
    char robotModel[64] = { 0 };
    char webversion[64] = { 0 };
    char controllerVersion[64] = { 0 };

    char ctrlBoxBoardversion[128] = { 0 };
    char driver1version[128] = { 0 };
    char driver2version[128] = { 0 };
    char driver3version[128] = { 0 };
    char driver4version[128] = { 0 };
    char driver5version[128] = { 0 };
    char driver6version[128] = { 0 };
    char endBoardversion[128] = { 0 };

    rtn = robot.GetSoftwareVersion(robotModel, webversion, controllerVersion);
    printf("robotmodel is: %s, webversion is: %s, controllerVersion is: %s \n\n", robotModel, webversion, controllerVersion);

    rtn = robot.GetHardwareVersion(ctrlBoxBoardversion, driver1version, driver2version, driver3version, driver4version, driver5version, driver6version, endBoardversion);
    printf("GetHardwareversion get hardware versoin is: %s, %s, %s, %s, %s, %s, %s, %s\n\n", ctrlBoxBoardversion, driver1version, driver2version, driver3version, driver4version, driver5version, driver6version, endBoardversion);

    rtn = robot.GetFirmwareVersion(ctrlBoxBoardversion, driver1version, driver2version, driver3version, driver4version, driver5version, driver6version, endBoardversion);
    printf("GetHardwareversion get hardware versoin is: %s, %s, %s, %s, %s, %s, %s, %s\n\n", ctrlBoxBoardversion, driver1version, driver2version, driver3version, driver4version, driver5version, driver6version, endBoardversion);
#else

    char robotModel[64] = { 0 };
    char webversion[64] = { 0 };
    char controllerVersion[64] = { 0 };

    char ctrlBoxBoardversion[128] = { 0 };
    char driver1version[128] = { 0 };
    char driver2version[128] = { 0 };
    char driver3version[128] = { 0 };
    char driver4version[128] = { 0 };
    char driver5version[128] = { 0 };
    char driver6version[128] = { 0 };
    char endBoardversion[128] = { 0 };

    rtn = robot.GetSoftwareVersion(robotModel, webversion, controllerVersion);
    printf("robotmodel is: %s, webversion is: %s, controllerVersion is: %s \n\n", robotModel, webversion, controllerVersion);

    rtn = robot.GetHardwareVersion(ctrlBoxBoardversion, driver1version, driver2version, driver3version, driver4version, driver5version, driver6version, endBoardversion);
    printf("GetHardwareversion get hardware versoin is: %s, %s, %s, %s, %s, %s, %s, %s\n\n", ctrlBoxBoardversion, driver1version, driver2version, driver3version, driver4version, driver5version, driver6version, endBoardversion);

    rtn = robot.GetFirmwareVersion(ctrlBoxBoardversion, driver1version, driver2version, driver3version, driver4version, driver5version, driver6version, endBoardversion);
    printf("GetHardwareversion get hardware versoin is: %s, %s, %s, %s, %s, %s, %s, %s\n\n", ctrlBoxBoardversion, driver1version, driver2version, driver3version, driver4version, driver5version, driver6version, endBoardversion);

    float yangle, zangle;
    robot.GetRobotInstallAngle(&yangle, &zangle);
    printf("yangle:%f,zangle:%f\n", yangle, zangle);

    while(true)
    {
        JointPos j_deg = {};
        robot.GetActualJointPosDegree(0, &j_deg);
        printf("joint pos deg:%f,%f,%f,%f,%f,%f\n", j_deg.jPos[0], j_deg.jPos[1], j_deg.jPos[2], j_deg.jPos[3], j_deg.jPos[4], j_deg.jPos[5]);

        float jointSpeed[6] = { 0.0 };
        robot.GetActualJointSpeedsDegree(0, jointSpeed);
        printf("joint speeds deg:%f,%f,%f,%f,%f,%f\n", jointSpeed[0], jointSpeed[1], jointSpeed[2], jointSpeed[3], jointSpeed[4], jointSpeed[5]);

        float jointAcc[6] = { 0.0 };
        robot.GetActualJointAccDegree(0, jointAcc);
        printf("joint acc deg:%f,%f,%f,%f,%f,%f\n", jointAcc[0], jointAcc[1], jointAcc[2], jointAcc[3], jointAcc[4], jointAcc[5]);

        float tcp_speed = 0.0;
        float ori_speed = 0.0;
        robot.GetTargetTCPCompositeSpeed(0, &tcp_speed, &ori_speed);
        printf("GetTargetTCPCompositeSpeed tcp %f;  ori  %f\n", tcp_speed, ori_speed);

        robot.GetActualTCPCompositeSpeed(0, &tcp_speed, &ori_speed);
        printf("GetActualTCPCompositeSpeed tcp %f;  ori  %f\n", tcp_speed, ori_speed);

        float targetSpeed[6] = { 0.0 };
        robot.GetTargetTCPSpeed(0, targetSpeed);
        printf("GetTargetTCPSpeed  %f,%f,%f,%f,%f,%f\n", targetSpeed[0], targetSpeed[1], targetSpeed[2], targetSpeed[3], targetSpeed[4], targetSpeed[5]);

        float actualSpeed[6] = { 0.0 };
        robot.GetActualTCPSpeed(0, actualSpeed);
        printf("GetActualTCPSpeed  %f,%f,%f,%f,%f,%f\n", actualSpeed[0], actualSpeed[1], actualSpeed[2], actualSpeed[3], actualSpeed[4], actualSpeed[5]);

        DescPose tcp = {};
        robot.GetActualTCPPose(0, &tcp);
        printf("tcp pose:%f,%f,%f,%f,%f,%f\n", tcp.tran.x, tcp.tran.y, tcp.tran.z, tcp.rpy.rx, tcp.rpy.ry, tcp.rpy.rz);

        DescPose flange = {};
        robot.GetActualToolFlangePose(0, &flange);
        printf("flange pose:%f,%f,%f,%f,%f,%f\n", flange.tran.x, flange.tran.y, flange.tran.z, flange.rpy.rx, flange.rpy.ry, flange.rpy.rz);

        int id = 0;
        robot.GetActualTCPNum(0, &id);
        printf("tcp num:%d\n", id);

        robot.GetActualWObjNum(0, &id);
        printf("wobj num:%d\n", id);

        float jtorque[6] = { 0.0 };
        robot.GetJointTorques(0, jtorque);
        printf("torques:%f,%f,%f,%f,%f,%f\n", jtorque[0], jtorque[1], jtorque[2], jtorque[3], jtorque[4], jtorque[5]);

        float t_ms = 0.0;
        robot.GetSystemClock(&t_ms);
        printf("system clock:%f\n", t_ms);

        int config = 0;
        robot.GetRobotCurJointsConfig(&config);
        printf("joint config:%d\n", config);

        uint8_t motionDone = 0;
        robot.GetRobotMotionDone(&motionDone);
        printf("GetRobotMotionDone :%d\n", motionDone);

        int len = 0;
        robot.GetMotionQueueLength(&len);
        printf("GetMotionQueueLength :%d\n", len);

        uint8_t emergState = 0;
        robot.GetRobotEmergencyStopState(&emergState);
        printf("GetRobotEmergencyStopState :%d\n", emergState);

        int comstate = 0;
        robot.GetSDKComState(&comstate);
        printf("GetSDKComState :%d\n", comstate);

        uint8_t si0_state, si1_state;
        robot.GetSafetyStopState(&si0_state, &si1_state);
        printf("GetSafetyStopState :%d  %d\n", si0_state, si1_state);

        double temp[6] = { 0.0 };
        robot.GetJointDriverTemperature(temp);
        printf("Temperature:%f,%f,%f,%f,%f,%f\n", temp[0], temp[1], temp[2], temp[3], temp[4], temp[5]);

        double torque[6] = { 0.0 };
        robot.GetJointDriverTorque(torque);
        printf("torque:%f,%f,%f,%f,%f,%f\n\n", torque[0], torque[1], torque[2], torque[3], torque[4], torque[5]);

        robot.GetRobotRealTimeState(&pkg);
    }
#endif
    robot.CloseRPC();
    return 0;
}
