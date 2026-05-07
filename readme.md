Monitoring/
├── CMakeLists.txt
├── CMakePresets.json                 # (권장) Debug/Release, Qt 경로 표준화
├── cmake/
│   ├── modules/
│   │   └── FindFairinoSDK.cmake      # (선택) SDK 탐색/검증 로직
│   └── toolchains/                   # (선택) 크로스/특수 툴체인
│
├── 3rdparty/
│   └── fairino_sdk/
│       ├── include/                  # robot.h 등
│       └── lib/                      # Win: .lib/.dll, Linux: .so
│       └── VERSION.txt               # (권장) 3.8.3 / 3.9.3 기록
│
├── core/                             # ✅ “로봇 모니터링 로직” (GUI/ROS 독립)
│   ├── CMakeLists.txt
│   ├── include/
│   │   └── monitoring/
│   │       ├── MonitorConfig.h       # IP, pollHz, 옵션
│   │       ├── RobotSnapshot.h       # UI/ROS로 전달할 DTO (SDK 타입 노출 금지)
│   │       ├── IMonitorService.h     # start/stop, latest(), signals/callback
│   │       └── AlarmRules.h          # (선택) 예지보전 룰/임계치
│   └── src/
│       ├── FairinoMonitorService.cpp # SDK 포함(robot.h) “여기서만”
│       ├── FairinoMonitorService_p.h # PImpl(권장)
│       ├── RingBuffer.cpp            # (선택) 트렌드 버퍼
│       └── AlarmRules.cpp            # (선택)
│
├── app/                              # ✅ Qt GUI (core만 링크)
│   ├── CMakeLists.txt
│   ├── main.cpp
│   ├── gui/
│   │   ├── MainWindow.h
│   │   ├── MainWindow.cpp
│   │   └── MainWindow.ui             # (선택)
│   └── resources/
│       └── app.qrc                   # (선택) 아이콘/폰트
│
├── tests/                            # (선택) core 단위테스트/스모크 테스트
│   ├── CMakeLists.txt
│   ├── test_snapshot_math.cpp
│   └── test_alarm_rules.cpp
│
├── scripts/                          # (선택) 배포/유틸
│   ├── package_win.ps1               # DLL 포함 패키징
│   └── package_linux.sh              # rpath/so 포함 패키징
│
├── deploy/                           # (선택) 패키징 산출물
└── build/                            # (gitignore) 빌드 출력