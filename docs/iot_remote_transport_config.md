# IoT Remote Transport Configuration

## 1. 목적

GUI PC와 Robot PC가 분리된 구성에서 원격 모니터링/명령 통신에 필요한 Transport 설정 기준을 정의한다.

본 문서는 실제 ZeroMQ 또는 TCP 구현 전, endpoint / topic / timeout / source mode 설정 기준을 정리하기 위한 문서이다.

---

## 2. 시스템 구성

```txt
Robot PC
- Fairino SDK polling
- 로봇 상태 Snapshot 생성
- Snapshot publish
- Command request 수신
- Command 실행
- Command response 반환

GUI PC
- Snapshot 수신
- IoT 대시보드 표시
- 작업자 명령 생성
- Command request 전송
- Command response 수신
```

---

## 3. 통신 방향

### Snapshot Channel

```txt
방향:
Robot PC → GUI PC

용도:
- 로봇 상태 모니터링
- 온도 / 부하 / 조인트 / TCP / 상태값 전달
- ONLINE / STALE / OFFLINE 판정
```

### Command Channel

```txt
방향:
GUI PC → Robot PC

용도:
- Manual / Auto
- Clear Error
- Start / Stop
- Home
- Joint Jog
- IO 명령
```

---

## 4. 권장 Transport 방식

초기 구현 기준:

```txt
Snapshot Channel:
ZeroMQ PUB/SUB

Command Channel:
ZeroMQ REQ/REP
```

확장 가능 구조:

```txt
IRemoteTransportClient
├─ DryRunRemoteTransportClient
├─ ZeroMqRemoteTransportClient
└─ TcpRemoteTransportClient
```

---

## 5. Endpoint 기본값

GUI PC 기준 설정값:

```txt
snapshotEndpoint = tcp://<ROBOT_PC_IP>:5556
commandEndpoint  = tcp://<ROBOT_PC_IP>:5557
snapshotTopic    = robot/
commandTimeoutMs = 1000
```

예시:

```txt
snapshotEndpoint = tcp://192.168.0.20:5556
commandEndpoint  = tcp://192.168.0.20:5557
snapshotTopic    = robot/
commandTimeoutMs = 1000
```

---

## 6. Port 기준

| Port | Direction | 용도 |
|---:|---|---|
| 5556 | Robot PC → GUI PC | Snapshot PUB/SUB |
| 5557 | GUI PC → Robot PC | Command REQ/REP |

초기 개발에서는 위 포트를 기본값으로 사용한다.

현장 적용 시에는 다음을 확인해야 한다.

```txt
- 방화벽 허용 여부
- 동일 포트 사용 프로세스 존재 여부
- Robot PC / GUI PC 네트워크 대역
- 고정 IP 사용 여부
```

---

## 7. Topic 기준

Snapshot topic:

```txt
robot/1/snapshot
robot/2/snapshot
```

전체 구독 topic:

```txt
robot/
```

GUI PC는 기본적으로 `robot/`을 구독하여 전체 로봇 snapshot을 수신한다.

---

## 8. Source Mode 기준

현재 QSettings 기준:

```txt
runtime/gatewaySourceMode
```

현재 지원값:

```txt
dummy
remote
```

권장 확장:

```txt
runtime/gatewaySourceMode = dummy | remote
runtime/remoteTransportType = dryrun | zeromq | tcp
```

현재 구조에서는 `remote` mode일 때 `DryRunRemoteTransportClient`를 사용한다.

향후 구조:

```txt
gatewaySourceMode = remote
remoteTransportType = dryrun
→ DryRunRemoteTransportClient

gatewaySourceMode = remote
remoteTransportType = zeromq
→ ZeroMqRemoteTransportClient
```

---

## 9. QSettings 예시

Windows Registry 기준:

```bat
reg add "HKCU\Software\Gachisoft\MonitoringApp\runtime" /v gatewaySourceMode /t REG_SZ /d remote /f
reg add "HKCU\Software\Gachisoft\MonitoringApp\runtime" /v remoteTransportType /t REG_SZ /d dryrun /f
reg add "HKCU\Software\Gachisoft\MonitoringApp\runtime" /v snapshotEndpoint /t REG_SZ /d tcp://192.168.0.20:5556 /f
reg add "HKCU\Software\Gachisoft\MonitoringApp\runtime" /v commandEndpoint /t REG_SZ /d tcp://192.168.0.20:5557 /f
reg add "HKCU\Software\Gachisoft\MonitoringApp\runtime" /v snapshotTopic /t REG_SZ /d robot/ /f
```

---

## 10. Timeout 기준

| 항목 | 권장값 |
|---|---:|
| Command response timeout | 1000 ms |
| Jog command timeout | 300~500 ms |
| Snapshot freshness ONLINE | 0~3 sec |
| Snapshot freshness STALE | 3~10 sec |
| Snapshot freshness OFFLINE | 10 sec 이상 |

---

## 11. Jog 안전 기준

Jog 명령은 반드시 아래 구조로 처리한다.

```txt
Button Press
→ startJointJog

Button Hold
→ jogHeartbeat

Button Release
→ stopJointJog
```

Robot PC는 heartbeat timeout을 기준으로 강제 정지를 수행해야 한다.

```txt
GUI heartbeat 주기:
100~200 ms

Robot PC jog timeout:
300~500 ms
```

---

## 12. 현재 구현 상태

현재 완료된 기준점:

```txt
remote-transport-dryrun-v1
```

현재 완료 내용:

```txt
- IRemoteTransportClient 정의
- DryRunRemoteTransportClient 구현
- MultiChannelRobotGateway가 DryRunRemoteTransportClient 경유
- Snapshot payload 수신 흐름 검증
- Command request/response 흐름 검증
```

---

## 13. 다음 구현 예정

다음 구현 대상:

```txt
ZeroMqRemoteTransportClient
```

예상 파일:

```txt
app/src/runtime/ZeroMqRemoteTransportClient.h
app/src/runtime/ZeroMqRemoteTransportClient.cpp
```

예상 역할:

```txt
- Snapshot SUB socket 연결
- Command REQ socket 연결
- command request 전송
- command response 수신
- snapshot payload signal emit
- command response payload signal emit
```

---

## 14. 한계 및 주의사항

현재 `DryRunRemoteTransportClient`는 실제 네트워크 통신을 수행하지 않는다.

```txt
- Robot PC 실제 연결 없음
- ZeroMQ socket 없음
- 명령 실행 없음
- response는 내부에서 즉시 생성
```

따라서 실제 Robot PC 연동 전에는 반드시 ZeroMQ 또는 TCP 구현체를 추가해야 한다.
