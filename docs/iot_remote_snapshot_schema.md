# IoT Remote Snapshot JSON Schema

## 1. 목적

Robot PC와 GUI PC가 분리된 구성에서 Robot PC가 수집한 로봇 모니터링 데이터를 GUI PC로 전달하기 위한 표준 Snapshot 메시지 구조를 정의한다.

GUI PC는 수신된 Snapshot을 `QVariantMap`으로 변환한 뒤 기존 `IRobotGateway::snapshotUpdated(int robotId, QVariantMap snapshot)` 신호로 전달한다.

---

## 2. 시스템 역할

### Robot PC

```txt
- Fairino SDK polling
- 로봇 상태 수집
- 온도 / 부하 / 위치 / 상태 데이터 수집
- UnifiedRobotSnapshot 호환 JSON 생성
- GUI PC로 Snapshot publish
- GUI PC에서 전달된 명령 처리
```

### GUI PC

```txt
- Snapshot 수신
- JSON parse
- QVariantMap 변환
- emit snapshotUpdated(robotId, snapshot)
- IoTViewModel 갱신
- IoTPanel 표시
- 알람 / 조치 이력 SQLite 저장
```

---

## 3. 통신 채널 구분

### Monitoring Channel

```txt
방향:
Robot PC → GUI PC

용도:
- 실시간 상태 표시
- 온도 / 부하 / 위치 / 상태 모니터링
- ONLINE / STALE / OFFLINE 판정
```

추천 방식:

```txt
ZeroMQ PUB/SUB
```

### Command Channel

```txt
방향:
GUI PC → Robot PC

용도:
- Manual / Auto 전환
- Clear Error
- Home
- Start / Stop
- Joint Jog
- IO 제어
```

추천 방식:

```txt
ZeroMQ REQ/REP 또는 DEALER/ROUTER
```

---

## 4. Topic 구조

ZeroMQ PUB/SUB 기준 추천 topic:

```txt
robot/1/snapshot
robot/2/snapshot
robot/all/snapshot
```

로봇별 구독:

```txt
robot/1/snapshot
```

전체 구독:

```txt
robot/
```

---

## 5. Snapshot JSON 예시

```json
{
  "messageType": "snapshot",
  "schemaVersion": 1,

  "robotId": 1,
  "robotName": "Robot 1",
  "model": "FR10",

  "connected": true,
  "running": true,

  "jointPositions": [0.0, 10.0, 20.0, 30.0, 40.0, 50.0],
  "tcpPose": [100.0, 200.0, 300.0, 180.0, 0.0, 90.0],

  "torques": [10.1, 9.8, 14.5, 11.2, 8.4, 10.9],
  "driverTemperatures": [40.1, 41.2, 42.3, 43.4, 44.5, 45.6],

  "robotState": "RUN",
  "errorText": "0",
  "sequenceState": "IDLE",
  "interlockState": "OK",

  "timestamp": "2026-05-20T19:30:00.123+09:00",
  "sequenceNumber": 12345
}
```

---

## 6. 필드 정의

| Field | Type | Required | 설명 |
|---|---|---:|---|
| `messageType` | string | O | 메시지 타입. Snapshot은 `"snapshot"` |
| `schemaVersion` | int | O | 메시지 스키마 버전 |
| `robotId` | int | O | GUI 내부 로봇 식별자. 1-based |
| `robotName` | string | X | 표시용 로봇 이름 |
| `model` | string | X | 예: `FR10`, `FR5` |
| `connected` | bool | O | Robot PC 기준 로봇 연결 상태 |
| `running` | bool | O | 로봇 동작 상태 |
| `jointPositions` | number[6] | O | J1~J6 현재 각도 |
| `tcpPose` | number[6] | O | X, Y, Z, RX, RY, RZ 또는 X, Y, Z, A, B, C |
| `torques` | number[6] | O | J1~J6 부하/토크 값 |
| `driverTemperatures` | number[6] | O | J1~J6 드라이버 또는 축 온도 |
| `robotState` | string | O | 예: `RUN`, `STOP`, `ERROR` |
| `errorText` | string | O | 에러 코드 또는 메시지 |
| `sequenceState` | string | X | 시퀀스 상태 |
| `interlockState` | string | X | 인터락 상태 |
| `timestamp` | string | O | Robot PC에서 Snapshot을 생성한 시각 |
| `sequenceNumber` | uint64 | O | Robot PC 기준 증가 번호 |

---

## 7. GUI 변환 규칙

Robot PC에서 수신한 JSON은 GUI PC에서 `QVariantMap`으로 변환한다.

GUI PC는 변환된 데이터를 아래 신호로 전달한다.

```cpp
emit snapshotUpdated(robotId, snapshot);
```

현재 GUI 내부 표준 Snapshot 필드는 `UnifiedRobotSnapshot` 구조와 호환되어야 한다.

```txt
robotId
connected
running
jointPositions
tcpPose
torques
driverTemperatures
robotState
errorText
sequenceState
interlockState
timestamp
sequenceNumber
```

---

## 8. 누락 필드 처리 기준

### Required 필드 누락 시

```txt
- robotId 없음: 메시지 폐기
- timestamp 없음: GUI PC 수신 시각으로 대체 가능
- sequenceNumber 없음: 0 또는 내부 증가값 사용 가능
```

### 배열 길이 부족 시

```txt
- jointPositions 길이 < 6: 부족한 축은 0으로 보정
- torques 길이 < 6: 부족한 축은 0으로 보정
- driverTemperatures 길이 < 6: 부족한 축은 0으로 보정
```

### connected false인 경우

```txt
- connected = false
- running = false 권장
- 온도/부하 값은 마지막값 유지 또는 0 처리 중 정책 결정 필요
```

현재 GUI 대시보드는 `lastUpdateEpochMs` 기준으로 ONLINE / STALE / OFFLINE을 판정하므로, 메시지가 끊기면 GUI에서 자동으로 STALE / OFFLINE으로 전환된다.

---

## 9. Timestamp 기준

`timestamp`는 Robot PC에서 Snapshot을 생성한 시각을 기준으로 한다.

권장 형식:

```txt
ISO 8601
예: 2026-05-20T19:30:00.123+09:00
```

GUI PC는 별도로 수신 시각을 기록할 수 있다.

```txt
Robot PC timestamp:
- 데이터 생성 시각

GUI PC receive time:
- 데이터 수신 시각
- ONLINE / STALE / OFFLINE 판정에 사용 가능
```

현재 GUI 구현에서는 snapshot 수신 시 `lastUpdateEpochMs`를 GUI PC 현재 시각으로 기록한다.

---

## 10. Sequence Number 기준

`sequenceNumber`는 Robot PC에서 로봇별 또는 전체 발행 기준으로 증가시킨다.

권장:

```txt
로봇별 sequenceNumber 증가
Robot 1: 1, 2, 3, ...
Robot 2: 1, 2, 3, ...
```

GUI PC는 sequenceNumber를 이용해 다음을 확인할 수 있다.

```txt
- 메시지 누락 여부
- 순서 역전 여부
- Robot PC 발행 주기 이상 여부
```

---

## 11. 전송 주기 권장

| 데이터 | 권장 주기 |
|---|---:|
| 온도 | 1 Hz |
| 부하/토크 | 1~5 Hz |
| Joint/TCP 상태 | 1~5 Hz |
| 알람 이벤트 | 이벤트 발생 시 |
| Heartbeat | 1 Hz |

현재 대시보드 기준 1 Hz부터 시작하는 것을 권장한다.

---

## 12. GUI 상태 판정 기준

GUI PC는 마지막 Snapshot 수신 시간을 기준으로 데이터 신선도를 판단한다.

```txt
0~3초 이내 수신      ONLINE
3~10초 미수신        STALE
10초 이상 미수신     OFFLINE
```

이 기준은 로봇 동작 상태인 RUN/STOP과 별도로 관리한다.

```txt
RUN/STOP:
- 로봇 동작 상태

ONLINE/STALE/OFFLINE:
- GUI PC 기준 데이터 수신 상태
```

---

## 13. 알람 저장 정책

GUI PC는 수신된 Snapshot을 기준으로 임계값을 평가하고 알람을 생성한다.

운영 기준:

```txt
알람 자동 DB 저장:
- 운영 시 ON 권장
- 개발/시연 중 OFF 가능

동일 알람 쿨다운:
- 기본 300초
- 기준: robotId + axis + metric + level
```

현재 ViewModel에는 아래 설정이 준비되어 있다.

```txt
alarmHistoryInsertEnabled
alarmCooldownSec
```

단, 일반 대시보드 UI에는 직접 노출하지 않고, 추후 운영 설정 화면에서 관리한다.

---

## 14. Command Channel 별도 정의 예정

본 문서는 Monitoring Snapshot 전송 포맷만 정의한다.

조그/홈/시작/정지 등 명령 메시지는 별도 문서에서 정의한다.

예정 문서:

```txt
docs/iot_remote_command_schema.md
```

명령 채널에서 반드시 고려할 항목:

```txt
- commandId
- robotId
- command
- params
- operator
- timestamp
- timeout
- command response
- jog heartbeat
- jog timeout safety stop
```
