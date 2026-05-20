# IoT Remote Command JSON Schema

## 1. 목적

Robot PC와 GUI PC가 분리된 구성에서 GUI PC가 Robot PC로 로봇 제어 명령을 전달하기 위한 표준 Command 메시지 구조를 정의한다.

본 문서는 아래 명령 채널을 대상으로 한다.

```txt
GUI PC → Robot PC
```

Robot PC는 수신한 명령을 검증한 뒤 Fairino SDK 또는 현장 제어 로직을 통해 실행하고, 실행 결과를 GUI PC로 응답한다.

---

## 2. 시스템 역할

### GUI PC

```txt
- 작업자 UI 제공
- 버튼 입력 감지
- Command Request 생성
- Robot PC로 명령 전송
- Command Response 수신
- commandFinished(robotId, command, ok, code, message)로 GUI 반영
```

### Robot PC

```txt
- Command Request 수신
- 명령 유효성 검증
- Manual / Auto / Interlock / Emergency 상태 확인
- Fairino SDK 또는 로봇 제어 로직 실행
- Command Response 반환
- Jog heartbeat timeout 감시
- 통신 이상 시 안전 정지 처리
```

---

## 3. 통신 채널 구분

### Monitoring Channel

```txt
방향:
Robot PC → GUI PC

용도:
- Snapshot publish
- 상태 모니터링
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
ZeroMQ REQ/REP
또는
ZeroMQ DEALER/ROUTER
```

초기 구현은 단순성을 위해 `REQ/REP`를 권장한다.  
복수 GUI, 비동기 명령 큐, 다중 Robot PC 연결까지 고려할 경우 `DEALER/ROUTER`로 확장한다.

---

## 4. Command Request 기본 구조

```json
{
  "messageType": "commandRequest",
  "schemaVersion": 1,

  "commandId": "cmd-20260520-000001",
  "robotId": 1,
  "command": "startJointJog",

  "params": {
    "joint": 3,
    "direction": "+",
    "speed": 10.0
  },

  "operator": "김진욱",
  "timestamp": "2026-05-20T19:45:00.123+09:00",
  "timeoutMs": 1000
}
```

---

## 5. Command Response 기본 구조

```json
{
  "messageType": "commandResponse",
  "schemaVersion": 1,

  "commandId": "cmd-20260520-000001",
  "robotId": 1,
  "command": "startJointJog",

  "ok": true,
  "code": 0,
  "message": "jog started",

  "timestamp": "2026-05-20T19:45:00.180+09:00"
}
```

---

## 6. Command Request 필드 정의

| Field | Type | Required | 설명 |
|---|---|---:|---|
| `messageType` | string | O | `"commandRequest"` |
| `schemaVersion` | int | O | 명령 메시지 스키마 버전 |
| `commandId` | string | O | 명령 추적용 고유 ID |
| `robotId` | int | O | 대상 로봇 ID. 1-based |
| `command` | string | O | 명령 이름 |
| `params` | object | X | 명령별 파라미터 |
| `operator` | string | X | 작업자 이름 |
| `timestamp` | string | O | GUI PC에서 명령을 생성한 시각 |
| `timeoutMs` | int | X | 명령 응답 대기 timeout |

---

## 7. Command Response 필드 정의

| Field | Type | Required | 설명 |
|---|---|---:|---|
| `messageType` | string | O | `"commandResponse"` |
| `schemaVersion` | int | O | 응답 메시지 스키마 버전 |
| `commandId` | string | O | 요청과 매칭되는 commandId |
| `robotId` | int | O | 대상 로봇 ID |
| `command` | string | O | 실행한 명령 이름 |
| `ok` | bool | O | 명령 성공 여부 |
| `code` | int | O | 결과 코드 |
| `message` | string | O | 결과 메시지 |
| `timestamp` | string | O | Robot PC에서 응답을 생성한 시각 |

---

## 8. Command ID 규칙

`commandId`는 GUI PC에서 생성한다.

권장 형식:

```txt
cmd-YYYYMMDD-HHMMSS-serial
```

예:

```txt
cmd-20260520-194500-000001
```

또는 UUID 사용 가능:

```txt
cmd-550e8400-e29b-41d4-a716-446655440000
```

`commandId`는 아래 용도로 사용한다.

```txt
- request / response 매칭
- 중복 명령 감지
- 로그 추적
- 작업자 조치 이력 연결
```

---

## 9. 지원 명령 목록

### 9.1 모드 전환

| Command | 설명 |
|---|---|
| `setManualMode` | 수동 모드 전환 |
| `setAutoMode` | 자동 모드 전환 |

Request 예:

```json
{
  "messageType": "commandRequest",
  "schemaVersion": 1,
  "commandId": "cmd-20260520-000010",
  "robotId": 1,
  "command": "setManualMode",
  "params": {},
  "operator": "김진욱",
  "timestamp": "2026-05-20T19:45:00.123+09:00",
  "timeoutMs": 1000
}
```

---

### 9.2 에러 클리어

| Command | 설명 |
|---|---|
| `clearError` | 로봇 또는 시스템 에러 클리어 |

Request 예:

```json
{
  "messageType": "commandRequest",
  "schemaVersion": 1,
  "commandId": "cmd-20260520-000020",
  "robotId": 1,
  "command": "clearError",
  "params": {},
  "operator": "김진욱",
  "timestamp": "2026-05-20T19:45:00.123+09:00",
  "timeoutMs": 1000
}
```

---

### 9.3 홈 / 시작 / 정지

| Command | 설명 |
|---|---|
| `home` | 홈 위치 이동 |
| `start` | 자동 운전 또는 시퀀스 시작 |
| `stop` | 자동 운전 또는 시퀀스 정지 |

Request 예:

```json
{
  "messageType": "commandRequest",
  "schemaVersion": 1,
  "commandId": "cmd-20260520-000030",
  "robotId": 1,
  "command": "home",
  "params": {},
  "operator": "김진욱",
  "timestamp": "2026-05-20T19:45:00.123+09:00",
  "timeoutMs": 3000
}
```

---

### 9.4 Joint Jog Start

| Command | 설명 |
|---|---|
| `startJointJog` | 지정 축 방향으로 조그 시작 |

Request 예:

```json
{
  "messageType": "commandRequest",
  "schemaVersion": 1,
  "commandId": "cmd-20260520-000040",
  "robotId": 1,
  "command": "startJointJog",
  "params": {
    "joint": 3,
    "direction": "+",
    "speed": 10.0
  },
  "operator": "김진욱",
  "timestamp": "2026-05-20T19:45:00.123+09:00",
  "timeoutMs": 500
}
```

`params` 필드:

| Field | Type | Required | 설명 |
|---|---|---:|---|
| `joint` | int | O | 조그 대상 축. 1~6 |
| `direction` | string | O | `"+"` 또는 `"-"` |
| `speed` | number | O | 조그 속도. 단위는 Robot PC 정책 기준 |

---

### 9.5 Joint Jog Stop

| Command | 설명 |
|---|---|
| `stopJointJog` | 지정 로봇의 조그 정지 |

Request 예:

```json
{
  "messageType": "commandRequest",
  "schemaVersion": 1,
  "commandId": "cmd-20260520-000041",
  "robotId": 1,
  "command": "stopJointJog",
  "params": {},
  "operator": "김진욱",
  "timestamp": "2026-05-20T19:45:01.123+09:00",
  "timeoutMs": 500
}
```

---

### 9.6 Jog Heartbeat

| Command | 설명 |
|---|---|
| `jogHeartbeat` | 조그 유지용 heartbeat |

Request 예:

```json
{
  "messageType": "commandRequest",
  "schemaVersion": 1,
  "commandId": "cmd-20260520-000042",
  "robotId": 1,
  "command": "jogHeartbeat",
  "params": {
    "jogSessionId": "jog-robot1-j3-plus-0001"
  },
  "operator": "김진욱",
  "timestamp": "2026-05-20T19:45:01.250+09:00",
  "timeoutMs": 300
}
```

---

## 10. Jog 안전 정책

조그는 반드시 `start`, `heartbeat`, `stop` 구조로 처리한다.

```txt
GUI 버튼 Press
→ startJointJog

GUI 버튼 Hold
→ jogHeartbeat 주기 전송

GUI 버튼 Release
→ stopJointJog
```

Robot PC는 heartbeat timeout을 감시해야 한다.

권장 기준:

```txt
GUI heartbeat 주기:
100~200ms

Robot PC jog timeout:
300~500ms

timeout 발생 시:
Robot PC가 즉시 stopJointJog 수행
```

조그 허용 조건:

```txt
- 로봇 연결 상태 정상
- 수동 모드
- 인터락 OK
- Emergency Stop 아님
- 동일 로봇에 기존 jog session이 없거나 정상 종료됨
```

Robot PC에서 조그 거부해야 하는 경우:

```txt
- Auto mode 상태
- Interlock NG
- Emergency Stop 상태
- Robot SDK disconnected
- 잘못된 joint 번호
- 잘못된 direction
- speed 범위 초과
```

---

## 11. Jog Session ID

`startJointJog` 성공 시 Robot PC는 `jogSessionId`를 응답에 포함할 수 있다.

Response 예:

```json
{
  "messageType": "commandResponse",
  "schemaVersion": 1,
  "commandId": "cmd-20260520-000040",
  "robotId": 1,
  "command": "startJointJog",
  "ok": true,
  "code": 0,
  "message": "jog started",
  "params": {
    "jogSessionId": "jog-robot1-j3-plus-0001"
  },
  "timestamp": "2026-05-20T19:45:00.180+09:00"
}
```

GUI PC는 이후 `jogHeartbeat`, `stopJointJog`에 동일한 `jogSessionId`를 포함할 수 있다.

---

## 12. 결과 코드 체계

| Code | 의미 |
|---:|---|
| `0` | 성공 |
| `100` | 알 수 없는 명령 |
| `101` | 잘못된 파라미터 |
| `102` | 대상 로봇 없음 |
| `200` | 로봇 미연결 |
| `201` | 수동 모드 아님 |
| `202` | 인터락 상태 불량 |
| `203` | 비상정지 상태 |
| `300` | SDK 명령 실패 |
| `301` | SDK timeout |
| `400` | 명령 실행 중 |
| `500` | 내부 오류 |

---

## 13. GUI commandFinished 매핑

Robot PC에서 받은 Command Response는 GUI PC에서 기존 신호로 변환한다.

```cpp
emit commandFinished(robotId, command, ok, code, message);
```

매핑 기준:

```txt
robotId  ← response.robotId
command  ← response.command
ok       ← response.ok
code     ← response.code
message  ← response.message
```

---

## 14. Timeout 처리 기준

### GUI PC timeout

GUI PC는 명령 요청 후 `timeoutMs` 안에 응답이 없으면 명령 실패로 처리한다.

```txt
ok = false
code = 301
message = "command response timeout"
```

단, 조그 명령의 경우 GUI timeout만으로 충분하지 않다.  
Robot PC가 heartbeat timeout을 기준으로 반드시 강제 정지해야 한다.

### Robot PC timeout

Robot PC는 SDK 호출 timeout 또는 jog heartbeat timeout을 감시한다.

```txt
SDK timeout:
- commandResponse ok=false
- code=301

Jog heartbeat timeout:
- Robot PC에서 즉시 stopJointJog
- commandResponse 또는 event로 GUI PC에 알림
```

---

## 15. Command Event

명령 결과 응답 외에도 Robot PC는 이벤트를 Monitoring Channel 또는 별도 Event Channel로 발행할 수 있다.

예:

```json
{
  "messageType": "commandEvent",
  "schemaVersion": 1,
  "robotId": 1,
  "event": "jogTimeoutStop",
  "level": "WARNING",
  "message": "Jog heartbeat timeout. Robot jog stopped.",
  "timestamp": "2026-05-20T19:45:02.000+09:00"
}
```

이벤트 예:

```txt
- jogStarted
- jogStopped
- jogTimeoutStop
- commandRejected
- robotDisconnected
- interlockChanged
```

---

## 16. 명령 로깅 정책

GUI PC와 Robot PC 모두 명령 로그를 남기는 것을 권장한다.

### GUI PC 로그

```txt
- 작업자 입력
- commandId
- command
- request timestamp
- response timestamp
- ok / code / message
```

### Robot PC 로그

```txt
- command 수신 시각
- 검증 결과
- SDK 호출 결과
- timeout 여부
- jog heartbeat 수신 상태
- 강제 stop 여부
```

운영 이력 DB는 기본적으로 GUI PC SQLite에 저장한다.  
Robot PC는 장애 분석용 로컬 로그 또는 최근 N분 이벤트 버퍼만 유지하는 것을 권장한다.

---

## 17. 보안 / 안전 고려사항

초기 현장 LAN 내부 통신에서는 단순 TCP 기반 ZeroMQ로 시작할 수 있다.

운영 확장 시 고려 항목:

```txt
- 허용 IP 제한
- 명령 채널 인증 토큰
- 작업자 권한 구분
- 명령 로그 보존
- Jog 명령 rate limit
- Emergency Stop / Interlock은 Robot PC 또는 PLC 우선
```

안전 관련 최종 판단은 GUI가 아니라 Robot PC 또는 PLC에서 해야 한다.

```txt
GUI PC:
- 명령 요청

Robot PC / PLC:
- 명령 허용 여부 최종 판단
- jog timeout safety stop
- interlock / emergency 우선 처리
```

---

## 18. 향후 확장

추후 추가 가능한 명령:

```txt
- Cartesian Jog
- Tool IO 제어
- Program Select
- Program Start / Pause / Resume
- Speed Override
- Servo Enable / Disable
- Reset
```

확장 시에도 request / response 기본 구조는 유지한다.
