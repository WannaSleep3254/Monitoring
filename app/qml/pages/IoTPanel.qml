import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root
    anchors.fill: parent

    // ============================================================
    // [레이아웃 높이 튜닝]
    // summaryCardHeight     : 각 Robot 카드 상단의 요약 카드 높이
    // bottomPanelHeight     : 각 Robot 카드 하단의 알람/예지보전 영역 높이
    // chartMinHeight        : 온도/전류 그래프의 최소 높이
    // chartPreferredHeight  : 온도/전류 그래프의 권장 높이
    //
    // 목표:
    // - 요약 카드와 하단 알람/예지보전은 고정 높이 유지
    // - 온도/전류 그래프만 남는 세로 공간을 가져가도록 구성
    // ============================================================
    property int summaryCardHeight: 68
    property int bottomPanelHeight: 200
    property int chartMinHeight: 160
    property int chartPreferredHeight: 215
    property int sectionSpacing: 8
    property int sectionRadius: 6
    property int toolBarHeight: 34

    // ============================================================
    // [IoT 패널 상단 기능 버튼 시그널]
    // 실제 로직 연동 시 C++ ViewModel 또는 상위 QML에서 연결
    // ============================================================
    signal thresholdSettingRequested()
    signal historyQueryRequested()

    // ===== 샘플 데이터 =====
    property int refreshIntervalMs: 1000
    property string lastUpdateText: "17:05:01"
    property real alarmLatencySec: 0.4
    property real predictionAccuracy: 92.4
    // =====================

    // ===== 임계치 튜닝용 =====
    property real tempWarningThreshold: 65
    property real currentWarningThreshold: 14
    // =====================

    property var axisColors: [
        "#2563eb",   // J1
        "#059669",   // J2
        "#7c3aed",   // J3
        "#0891b2",   // J4
        "#4f46e5",   // J5
        "#475569"    // J6
    ]

    // 시간 레이블은 고정으로 6개, 10초 간격으로 -50s ~ Now
    property var timeLabels: ["-50s", "-40s", "-30s", "-20s", "-10s", "Now"]

    property var robotModels: [
        {
            name: "Robot 1 (FR10)",
            running: true,
            tempSeries: [
                { axis: "J1", values: [40.1, 40.8, 41.5, 42.0, 42.3, 42.5] },
                { axis: "J2", values: [41.2, 41.6, 42.1, 43.0, 43.6, 44.1] },
                { axis: "J3", values: [39.8, 40.3, 40.7, 41.1, 41.5, 41.8] },
                { axis: "J4", values: [42.0, 42.8, 43.5, 44.2, 45.4, 46.3] },
                { axis: "J5", values: [40.5, 41.0, 41.8, 42.3, 42.8, 43.2] },
                { axis: "J6", values: [38.9, 39.4, 39.8, 40.2, 40.5, 40.7] }
            ],
            currentSeries: [
                { axis: "J1", values: [10.2, 10.8, 11.4, 11.7, 12.0, 12.3] },
                { axis: "J2", values: [8.4, 8.7, 9.1, 9.4, 9.6, 9.8] },
                { axis: "J3", values: [13.1, 13.7, 14.4, 15.0, 15.3, 15.6] },
                { axis: "J4", values: [9.8, 10.2, 10.7, 10.9, 11.0, 11.2] },
                { axis: "J5", values: [7.2, 7.5, 7.9, 8.1, 8.3, 8.4] },
                { axis: "J6", values: [9.1, 9.5, 10.0, 10.4, 10.7, 10.9] }
            ],
            alarms: [
                { time: "17:04:58", level: "WARN", message: "J3 전류 임계치 접근" },
                { time: "17:04:41", level: "INFO", message: "실시간 갱신 정상" },
                { time: "17:04:20", level: "INFO", message: "로봇 가동 상태 RUN" }
            ],
            prediction: {
                modelStatus: "구현 완료",
                riskLevel: "주의",
                score: 82,
                causeText: "J3 전류 상승",
                recommendedAction: "J3 축 전류 추세 점검 권장"
            }
        },
        {
            name: "Robot 2 (FR5)",
            running: true,
            tempSeries: [
                { axis: "J1", values: [41.5, 42.0, 42.8, 43.4, 43.8, 44.2] },
                { axis: "J2", values: [42.0, 42.7, 43.3, 44.0, 44.5, 45.0] },
                { axis: "J3", values: [40.2, 40.7, 41.3, 41.9, 42.3, 42.6] },
                { axis: "J4", values: [43.1, 43.8, 44.6, 45.4, 46.2, 47.1] },
                { axis: "J5", values: [41.0, 41.7, 42.2, 42.8, 43.4, 43.9] },
                { axis: "J6", values: [39.7, 40.2, 40.7, 41.0, 41.3, 41.5] }
            ],
            currentSeries: [
                { axis: "J1", values: [11.8, 12.4, 13.0, 13.5, 13.8, 14.1] },
                { axis: "J2", values: [9.0, 9.4, 9.7, 10.0, 10.3, 10.6] },
                { axis: "J3", values: [12.0, 12.5, 13.0, 13.3, 13.6, 13.8] },
                { axis: "J4", values: [13.7, 14.2, 14.9, 15.4, 15.8, 16.2] },
                { axis: "J5", values: [8.1, 8.5, 8.8, 9.0, 9.3, 9.5] },
                { axis: "J6", values: [10.2, 10.6, 10.9, 11.2, 11.5, 11.7] }
            ],
            alarms: [
                { time: "17:04:57", level: "WARN", message: "J4 전류 초과 감지" },
                { time: "17:04:32", level: "INFO", message: "알람 표시 정상" },
                { time: "17:04:10", level: "INFO", message: "로봇 가동 상태 RUN" }
            ],
            prediction: {
                modelStatus: "구현 완료",
                riskLevel: "경고",
                score: 91,
                causeText: "J4 전류 초과",
                recommendedAction: "J4 축 이상 징후 감지, 점검 요청"
            }
        }
    ]

    // ============================================================
    // [로봇별 임계값 상태]
    // Dialog에서 저장한 값이 여기에 반영되고,
    // 각 AxisLineChart는 이 값을 binding으로 참조한다.
    // ============================================================
    property var robotThresholds: [
        {
            temperature: {
                normalMax: 55,
                warningMax: 60,
                alarmMax: 70
            },
            current: {
                normalMax: 12,
                warningMax: 14,
                alarmMax: 16
            }
        },
        {
            temperature: {
                normalMax: 55,
                warningMax: 60,
                alarmMax: 70
            },
            current: {
                normalMax: 12,
                warningMax: 14,
                alarmMax: 16
            }
        }
    ]

    function defaultThreshold() {
        return {
            temperature: {
                normalMax: 55,
                warningMax: 60,
                alarmMax: 70
            },
            current: {
                normalMax: 12,
                warningMax: 14,
                alarmMax: 16
            }
        }
    }

    function normalizeThreshold(thresholdData) {
        var fallback = root.defaultThreshold()

        if (!thresholdData)
            return fallback

        var temp = thresholdData.temperature || fallback.temperature
        var current = thresholdData.current || thresholdData.torque || fallback.current

        return {
            temperature: {
                normalMax: Number(temp.normalMax),
                warningMax: Number(temp.warningMax),
                alarmMax: Number(temp.alarmMax)
            },
            current: {
                normalMax: Number(current.normalMax),
                warningMax: Number(current.warningMax),
                alarmMax: Number(current.alarmMax)
            }
        }
    }

    function thresholdFor(robotIndex) {
        // robotIndex: 0-based
        if (!root.robotThresholds || robotIndex < 0 || robotIndex >= root.robotThresholds.length)
            return root.defaultThreshold()

        return root.normalizeThreshold(root.robotThresholds[robotIndex])
    }

    function applyThreshold(robotIndex, thresholdData) {
        // robotIndex: 1-based from ThresholdSettingDialog
        if (!thresholdData) {
            console.warn("[QML] thresholdData is undefined. Check ThresholdSettingDialog.saveRequested signature.")
            return
        }

        var targetIndex = robotIndex - 1
        if (targetIndex < 0)
            return

        var copied = root.robotThresholds ? root.robotThresholds.slice() : []

        while (copied.length <= targetIndex)
            copied.push(root.defaultThreshold())

        copied[targetIndex] = root.normalizeThreshold(thresholdData)

        // QML binding 갱신을 위해 배열 자체를 다시 대입
        root.robotThresholds = copied

        console.log("[QML] threshold applied, robot =", robotIndex,
                    "temp =", copied[targetIndex].temperature.normalMax,
                    copied[targetIndex].temperature.warningMax,
                    copied[targetIndex].temperature.alarmMax,
                    "current =", copied[targetIndex].current.normalMax,
                    copied[targetIndex].current.warningMax,
                    copied[targetIndex].current.alarmMax)
    }

    function latestValue(seriesItem) {
        if (!seriesItem || !seriesItem.values || seriesItem.values.length === 0)
            return 0
        return seriesItem.values[seriesItem.values.length - 1]
    }

    function maxLatestInfo(series) {
        var maxValue = -999999
        var maxAxis = "-"
        for (var i = 0; i < series.length; ++i) {
            var v = latestValue(series[i])
            if (v > maxValue) {
                maxValue = v
                maxAxis = series[i].axis
            }
        }
        return { axis: maxAxis, value: maxValue }
    }

    function monitoringCompletion() {
        return "100%"
    }

    Rectangle {
        anchors.fill: parent
        radius: 8
        color: "white"
        border.color: "#d7dde6"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: root.sectionSpacing
/*
            // 상단 요구사항 요약
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                MetricCard {
                    Layout.fillWidth: true
                    title: "모니터링 항목 완성도"
                    valueText: root.monitoringCompletion()
                    subText: "전류 / 온도 / 가동상태"
                    valueColor: "#2563eb"
                }

                MetricCard {
                    Layout.fillWidth: true
                    title: "실시간 갱신 주기"
                    valueText: (root.refreshIntervalMs / 1000).toFixed(1) + " s"
                    subText: "마지막 갱신 " + root.lastUpdateText
                    valueColor: "#16a34a"
                }

                MetricCard {
                    Layout.fillWidth: true
                    title: "알람 발생 지연"
                    valueText: root.alarmLatencySec.toFixed(1) + " s"
                    subText: root.alarmLatencySec < 1.0 ? "Spec 만족" : "점검 필요"
                    valueColor: root.alarmLatencySec < 1.0 ? "#16a34a" : "#dc2626"
                }

                MetricCard {
                    Layout.fillWidth: true
                    title: "예측 정확도"
                    valueText: root.predictionAccuracy.toFixed(1) + " %"
                    subText: root.predictionAccuracy > 90 ? "Spec 만족" : "점검 필요"
                    valueColor: root.predictionAccuracy > 90 ? "#16a34a" : "#f97316"
                }
            }
*/
            // ============================================================
            // [IoT 상단 툴바]
            // 위치: Robot 1 / Robot 2 카드 영역 상단
            // 용도:
            // - 임계값 설정
            // - 이력 조회
            //
            // 기존 CSV 저장 버튼은 직접 저장 기능이 아니라
            // "이력 조회" 화면에서 기간 조회 후 CSV 저장까지 확장하는 구조로 변경
            // ============================================================
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: root.toolBarHeight
                Layout.minimumHeight: root.toolBarHeight
                Layout.maximumHeight: root.toolBarHeight
                Layout.fillHeight: false
                spacing: 8

                Item {
                    Layout.fillWidth: true
                }

                IoTToolbarButton {
                    text: "임계값 설정"
                    Layout.preferredWidth: 112
                    onClicked: {
                        console.log("[QML] IoT threshold setting requested")
                        root.thresholdSettingRequested()
                        thresholdDialog.open()
                        // TODO: iotViewModel.requestThresholdSetting()
                    }
                }

                IoTToolbarButton {
                    text: "이력 조회"
                    Layout.preferredWidth: 96
                    onClicked: {
                        console.log("[QML] IoT history query requested")
                        root.historyQueryRequested()
                        historyDialog.open()
                        // TODO: iotViewModel.requestHistoryQuery()
                    }
                }
            }

            // ============================================================
            // [로봇 2대 동시 모니터링 영역]
            // 위치: IoTPanel 전체 본문
            // 구성: 좌측 Robot 1 카드 / 우측 Robot 2 카드
            // ============================================================
            GridLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                columns: 2
                rowSpacing: 12
                columnSpacing: 12

                Repeater {
                    model: root.robotModels

                    Rectangle {
                        id: robotCard
                        property var robotData: modelData
                        property int robotIndex: index

                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 8
                        color: "#f8fafc"
                        border.color: "#e2e8f0"

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: root.sectionSpacing

                            // ====================================================
                            // [Robot 카드 제목 영역]
                            // 위치: 각 Robot 카드 최상단
                            // 예: Robot 1 (FR10), Robot 2 (FR5)
                            // ====================================================
                            Text {
                                text: robotCard.robotData.name
                                color: "#2563eb"
                                font.pixelSize: 18
                                font.bold: true
                            }

                            // ====================================================
                            // [요약 카드 영역]
                            // 위치: Robot 카드 상단, 제목 바로 아래
                            // 구성:
                            // - 가동 상태
                            // - Max 온도
                            // - Max 전류
                            // - 워스트 축
                            //
                            // 높이 제어:
                            // - root.summaryCardHeight로 고정
                            // - Layout.fillHeight는 false로 유지
                            // ====================================================
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.preferredHeight: root.summaryCardHeight
                                Layout.minimumHeight: root.summaryCardHeight
                                Layout.maximumHeight: root.summaryCardHeight
                                Layout.fillHeight: false
                                spacing: 8

                                MetricCard {
                                    Layout.fillWidth: true
                                    title: "가동 상태"
                                    valueText: robotCard.robotData.running ? "RUN" : "STOP"
                                    subText: robotCard.robotData.running ? "정상 운전" : "정지"
                                    valueColor: robotCard.robotData.running ? "#16a34a" : "#dc2626"
                                }

                                MetricCard {
                                    Layout.fillWidth: true
                                    title: "Max 온도"
                                    valueText: root.maxLatestInfo(robotCard.robotData.tempSeries).value.toFixed(1) + " °C"
                                    subText: root.maxLatestInfo(robotCard.robotData.tempSeries).axis
                                    valueColor: root.maxLatestInfo(robotCard.robotData.tempSeries).value >= root.thresholdFor(robotCard.robotIndex).temperature.warningMax ? "#dc2626" : "#334155"
                                }

                                MetricCard {
                                    Layout.fillWidth: true
                                    title: "Max 전류"
                                    valueText: root.maxLatestInfo(robotCard.robotData.currentSeries).value.toFixed(1) + " A"
                                    subText: root.maxLatestInfo(robotCard.robotData.currentSeries).axis
                                    valueColor: root.maxLatestInfo(robotCard.robotData.currentSeries).value >= root.thresholdFor(robotCard.robotIndex).current.warningMax ? "#dc2626" : "#334155"
                                }

                                MetricCard {
                                    Layout.fillWidth: true
                                    title: "워스트 축"
                                    valueText: root.maxLatestInfo(robotCard.robotData.tempSeries).axis
                                    subText: "온도 기준"
                                    valueColor: "#f97316"
                                }
                            }

                            // ====================================================
                            // [온도 그래프 영역]
                            // 위치: 요약 카드 아래, 첫 번째 그래프
                            // 목적:
                            // - J1~J6 축별 온도 시계열 표시
                            // - 임계값 초과 여부를 시각적으로 확인
                            //
                            // 높이 제어:
                            // - Layout.fillHeight: true
                            // - 남는 세로 공간을 전류 그래프와 함께 나눠 사용
                            // ====================================================
                            AxisLineChart {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.minimumHeight: root.chartMinHeight
                                Layout.preferredHeight: root.chartPreferredHeight
                                title: "축별 온도 추세"
                                unit: "°C"
                                minValue: 30
                                maxValue: 80
                                normalValue: root.thresholdFor(robotCard.robotIndex).temperature.normalMax
                                warningValue: root.thresholdFor(robotCard.robotIndex).temperature.warningMax
                                alarmValue: root.thresholdFor(robotCard.robotIndex).temperature.alarmMax
                                timeLabels: root.timeLabels
                                series: robotCard.robotData.tempSeries
                                lineColors: root.axisColors
                            }

                            // ====================================================
                            // [전류/토크 그래프 영역]
                            // 위치: 온도 그래프 아래, 두 번째 그래프
                            // 목적:
                            // - J1~J6 축별 전류 또는 토크 시계열 표시
                            // - 이상 부하/마찰 증가 추세 확인
                            //
                            // 현재 표기:
                            // - 축별 전류 추세
                            // - 토크 값을 사용할 경우 title/unit만 변경 가능
                            // ====================================================
                            AxisLineChart {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.minimumHeight: root.chartMinHeight
                                Layout.preferredHeight: root.chartPreferredHeight
                                title: "축별 전류 추세"
                                unit: "A"
                                minValue: 0
                                maxValue: 20
                                normalValue: root.thresholdFor(robotCard.robotIndex).current.normalMax
                                warningValue: root.thresholdFor(robotCard.robotIndex).current.warningMax
                                alarmValue: root.thresholdFor(robotCard.robotIndex).current.alarmMax
                                timeLabels: root.timeLabels
                                series: robotCard.robotData.currentSeries
                                lineColors: root.axisColors
                            }

                            // ====================================================
                            // [하단 알람 / 예지보전 영역]
                            // 위치: Robot 카드 최하단
                            // 좌측: 알람 이력 / 이상 징후
                            // 우측: 예지 보전 / 조치 요청
                            //
                            // 높이 제어:
                            // - root.bottomPanelHeight로 고정
                            // - 그래프 시인성을 위해 낮은 높이로 유지
                            // ====================================================
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.preferredHeight: root.bottomPanelHeight
                                Layout.minimumHeight: root.bottomPanelHeight
                                Layout.maximumHeight: root.bottomPanelHeight
                                Layout.fillHeight: false
                                spacing: 8

                                // ------------------------------------------------
                                // [하단 좌측 패널] 알람 이력 / 이상 징후
                                // - 임계값 초과, 이상 징후, 상태 이벤트를 표시
                                // - 현재는 최근 2개만 표시하도록 제한
                                // ------------------------------------------------
                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    radius: 6
                                    color: "white"
                                    border.color: "#e2e8f0"

                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: 8
                                        spacing: 5

                                        Text {
                                            Layout.fillWidth: true
                                            Layout.preferredHeight: 20
                                            Layout.fillHeight: false

                                            text: "알람 이력 / 이상 징후"
                                            color: "#334155"
                                            font.pixelSize: 15
                                            font.bold: true
                                            verticalAlignment: Text.AlignVCenter
                                        }

                                        Repeater {
                                            model: robotCard.robotData.alarms.slice(0, 2)

                                            Rectangle {
                                                // 알람 1행은 고정 높이로 유지
                                                // Layout.fillHeight를 false로 둬서 Row가 세로로 늘어나지 않게 함
                                                Layout.fillWidth: true
                                                Layout.preferredHeight: 28
                                                Layout.minimumHeight: 28
                                                Layout.maximumHeight: 28
                                                Layout.fillHeight: false

                                                radius: 4
                                                color: modelData.level === "WARN" ? "#fff7ed" : "#f8fafc"
                                                border.color: modelData.level === "WARN" ? "#fdba74" : "#e2e8f0"

                                                RowLayout {
                                                    anchors.fill: parent
                                                    anchors.margins: 5
                                                    spacing: 6

                                                    Rectangle {
                                                        width: 38
                                                        height: 18
                                                        radius: 9
                                                        color: modelData.level === "WARN" ? "#f97316" : "#94a3b8"

                                                        Text {
                                                            anchors.centerIn: parent
                                                            text: modelData.level
                                                            color: "white"
                                                            font.pixelSize: 9
                                                            font.bold: true
                                                        }
                                                    }

                                                    Text {
                                                        text: modelData.time
                                                        color: "#64748b"
                                                        font.pixelSize: 11
                                                        Layout.preferredWidth: 54
                                                    }

                                                    Text {
                                                        text: modelData.message
                                                        color: "#334155"
                                                        font.pixelSize: 11
                                                        Layout.fillWidth: true
                                                        elide: Text.ElideRight
                                                    }
                                                }
                                            }
                                        }

                                        // 남는 세로 공간을 아래로 밀어
                                        // 알람 목록이 제목 바로 아래부터 상단 정렬되도록 함
                                        Item {
                                            Layout.fillHeight: true
                                        }
                                    }
                                }

                                // ------------------------------------------------
                                // [하단 우측 패널] 위험도 / 조치 요청
                                // - 위험도
                                // - 원인
                                // - 조치 권장 메시지 표시
                                // ------------------------------------------------
                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    radius: 6
                                    color: "white"
                                    border.color: "#e2e8f0"

                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: 8
                                        spacing: 4

                                        Text {
                                            text: "위험도 / 조치 요청"
                                            color: "#334155"
                                            font.pixelSize: 14
                                            font.bold: true
                                        }

                                        // ----------------------------------------
                                        // [3분할 표시]
                                        // 1) 위험도 : 등급 + 스코어
                                        // 2) 원인   : 이상 판단 원인 축/항목
                                        // 3) 조치   : 작업자 확인/점검 요청
                                        // ----------------------------------------
                                        RiskActionBlock {
                                            title: "위험도"
                                            valueText: robotCard.robotData.prediction.riskLevel
                                                       + " (" + robotCard.robotData.prediction.score + ")"
                                            detailText: robotCard.robotData.prediction.score >= 90
                                                        ? "즉시 점검 필요"
                                                        : "추세 관찰 필요"
                                            valueColor: robotCard.robotData.prediction.score >= 90
                                                        ? "#dc2626"
                                                        : "#f97316"
                                        }

                                        RiskActionBlock {
                                            title: "원인"
                                            valueText: robotCard.robotData.prediction.causeText
                                            detailText: "이상 판단 원인"
                                            valueColor: "#334155"
                                        }

                                        RiskActionBlock {
                                            title: "조치"
                                            valueText: robotCard.robotData.prediction.recommendedAction
                                            detailText: "작업자 확인 항목"
                                            valueColor: "#334155"
                                            wrapValue: true
                                        }
                                    }
                                }
                            }
}
                    }
                }
            }
        }
    }

    // ============================================================
    // [다이얼로그] 임계값 설정
    // 호출 위치:
    // - 상단 툴바 [임계값 설정] 버튼 클릭
    //
    // 구성:
    // - Robot 1 임계값 설정
    // - Robot 2 임계값 설정
    // - 온도 / 전류 임계값 입력
    // ============================================================


    // ============================================================
    // [다이얼로그] 이력 조회
    // 호출 위치:
    // - 상단 툴바 [이력 조회] 버튼 클릭
    //
    // 구성:
    // - 로봇 선택
    // - 조회 기간 선택
    // - CSV 내보내기
    // - 시간 / 로봇 / 축 / 온도 / 토크 / 상태 / 설명 테이블
    // ============================================================
    // ============================================================
    // [다이얼로그] 이력 조회
    // 호출 위치:
    // - 상단 툴바 [이력 조회] 버튼 클릭
    //
    // 구성:
    // - 로봇 선택
    // - 기간 선택
    // - 이력 구분: 전체 / 알람 이력 / 조치 이력
    // - 설명/조치내용 검색
    // - CSV 내보내기
    // - 좌측 이력 테이블
    // - 우측 선택 이력 상세
    // ============================================================




    // ============================================================
    // [분리된 다이얼로그]
    // 임계값 설정 / 이력 조회 화면은 별도 QML로 분리
    //
    // 배치 권장:
    // - IoTPanel.qml
    // - ThresholdSettingDialog.qml
    // - HistoryQueryDialog.qml
    //
    // 주의:
    // - 같은 폴더에 두면 별도 import 없이 타입 인식 가능
    // - qrc/CMake 사용 시 새 QML 파일 등록 필요
    // ============================================================
    ThresholdSettingDialog {
        id: thresholdDialog
        hostWidth: root.width
        hostHeight: root.height
        thresholds: root.robotThresholds

        onSaveRequested: function(robotIndex, thresholdData) {
            root.applyThreshold(robotIndex, thresholdData)

            // 이후 실제 로직 붙일 때
            // TODO: iotViewModel.saveThreshold(robotIndex, thresholdData)
        }
    }

    HistoryQueryDialog {
        id: historyDialog
        hostWidth: root.width
        hostHeight: root.height

        onExportCsvRequested: function(rows, robotFilter, periodFilter, typeFilter, searchText) {
            console.log("[QML] CSV export requested, rows = " + rows.length)
            // TODO: iotViewModel.exportHistoryCsv(rows, robotFilter, periodFilter, typeFilter, searchText)
        }
    }

    // ============================================================
    // [컴포넌트] MetricCard
    // 사용 위치: 요약 카드 영역
    // 용도: 가동 상태, Max 온도, Max 전류, 워스트 축 표시
    // ============================================================
    component MetricCard: Rectangle {
        id: metricRoot

        property string title: ""
        property string valueText: "-"
        property string subText: "-"
        property color valueColor: "#334155"

        Layout.fillWidth: true
        Layout.preferredHeight: root.summaryCardHeight
        Layout.minimumHeight: root.summaryCardHeight
        Layout.maximumHeight: root.summaryCardHeight
        Layout.fillHeight: false

        radius: 6
        color: "#f8fafc"
        border.color: "#e2e8f0"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 6
            spacing: 0

            Text {
                Layout.fillWidth: true
                text: metricRoot.title
                color: "#64748b"
                font.pixelSize: 11
                elide: Text.ElideRight
            }

            Text {
                Layout.fillWidth: true
                text: metricRoot.valueText
                color: metricRoot.valueColor
                font.pixelSize: 15
                font.bold: true
                elide: Text.ElideRight
            }

            Text {
                Layout.fillWidth: true
                text: metricRoot.subText
                color: "#94a3b8"
                font.pixelSize: 10
                elide: Text.ElideRight
            }
        }
    }


    // ============================================================
    // [컴포넌트] RiskActionBlock
    // 사용 위치: 하단 우측 위험도 / 조치 요청 패널
    // 용도:
    // - 위험도
    // - 원인
    // - 조치
    // 3개 항목을 동일한 카드 형태로 표시
    // ============================================================
    component RiskActionBlock: Rectangle {
        id: blockRoot

        property string title: ""
        property string valueText: "-"
        property string detailText: ""
        property color valueColor: "#334155"
        property bool wrapValue: false

        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumHeight: 42

        radius: 6
        color: "#f8fafc"
        border.color: "#e2e8f0"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 6
            spacing: 2

            Text {
                Layout.fillWidth: true
                text: blockRoot.title
                color: "#64748b"
                font.pixelSize: 10
                font.bold: true
                elide: Text.ElideRight
            }

            Text {
                Layout.fillWidth: true
                Layout.fillHeight: blockRoot.wrapValue
                text: blockRoot.valueText
                color: blockRoot.valueColor
                font.pixelSize: 12
                font.bold: blockRoot.title === "위험도"
                wrapMode: blockRoot.wrapValue ? Text.WordWrap : Text.NoWrap
                elide: blockRoot.wrapValue ? Text.ElideNone : Text.ElideRight
                maximumLineCount: blockRoot.wrapValue ? 2 : 1
            }

            Text {
                Layout.fillWidth: true
                text: blockRoot.detailText
                color: "#94a3b8"
                font.pixelSize: 9
                elide: Text.ElideRight
            }
        }
    }

    // ============================================================
    // [컴포넌트] IoTToolbarButton
    // 사용 위치: IoT 상단 툴바
    // 용도:
    // - 임계값 설정
    // - 이력 조회
    // ============================================================
    component IoTToolbarButton: Button {
        id: toolbarButton

        Layout.preferredHeight: 28
        Layout.minimumHeight: 28
        Layout.maximumHeight: 28
        Layout.fillHeight: false

        font.pixelSize: 12

        contentItem: Text {
            text: toolbarButton.text
            color: "#334155"
            font.pixelSize: 12
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        background: Rectangle {
            radius: 5
            color: toolbarButton.pressed ? "#cbd5e1"
                 : toolbarButton.hovered ? "#e2e8f0"
                 : "#f1f5f9"
            border.color: "#cbd5e1"
        }
    }

    // ============================================================
    // [컴포넌트] AxisLineChart
    // 사용 위치:
    // - 온도 그래프 영역
    // - 전류/토크 그래프 영역
    //
    // 용도:
    // - Canvas 기반 라인 그래프
    // - J1~J6 다중 시계열 표시
    // - 임계값 라인 표시
    // ============================================================
    component AxisLineChart: Rectangle {
        id: chartRoot

        property string title: ""
        property string unit: ""
        property real minValue: 0
        property real maxValue: 100

        // 임계값 라인
        property real normalValue: 0
        property real warningValue: 0
        property real alarmValue: 0
        property var timeLabels: []
        property var series: []
        property var lineColors: []

        // ============================================================
        // [선 스타일 설정]
        // 축별 데이터 라인과 임계값 라인의 시각적 구분용
        // ============================================================
        property real dataLineWidth: 1.7
        property real thresholdLineWidth: 1.0
        property real thresholdAlpha: 0.72
        property real pointRadius: 2.6

        radius: 6
        color: "white"
        border.color: "#e2e8f0"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 4

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 20
                Layout.fillHeight: false

                Text {
                    text: chartRoot.title
                    color: "#334155"
                    font.pixelSize: 15
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: "정상 " + chartRoot.normalValue
                          + " / 주의 " + chartRoot.warningValue
                          + " / 경고 " + chartRoot.alarmValue
                          + " " + chartRoot.unit
                    color: "#f97316"
                    font.pixelSize: 11
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 4
                color: "#f8fafc"
                border.color: "#e5e7eb"

                Canvas {
                    id: lineCanvas
                    anchors.fill: parent
                    anchors.margins: 8

                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.clearRect(0, 0, width, height)

                        var w = width
                        var h = height

                        var left = 40
                        var right = 12
                        var top = 12
                        var bottom = 24

                        var plotW = w - left - right
                        var plotH = h - top - bottom

                        function valueToY(v) {
                            var ratio = (v - chartRoot.minValue) / (chartRoot.maxValue - chartRoot.minValue)
                            ratio = Math.max(0, Math.min(1, ratio))
                            return top + plotH * (1 - ratio)
                        }

                        function indexToX(index, count) {
                            if (count <= 1)
                                return left
                            return left + plotW * index / (count - 1)
                        }

                        // horizontal grid
                        var gridCount = 4
                        ctx.strokeStyle = "#e5e7eb"
                        ctx.lineWidth = 1
                        ctx.font = "11px sans-serif"
                        ctx.fillStyle = "#94a3b8"

                        for (var gy = 0; gy <= gridCount; ++gy) {
                            var y = top + plotH * gy / gridCount
                            ctx.beginPath()
                            ctx.moveTo(left, y)
                            ctx.lineTo(left + plotW, y)
                            ctx.stroke()

                            var labelValue = chartRoot.maxValue - (chartRoot.maxValue - chartRoot.minValue) * gy / gridCount
                            ctx.fillText(labelValue.toFixed(0), 4, y + 4)
                        }

                        // vertical grid + x labels
                        for (var tx = 0; tx < chartRoot.timeLabels.length; ++tx) {
                            var x = indexToX(tx, chartRoot.timeLabels.length)
                            ctx.strokeStyle = "#eef2f7"
                            ctx.beginPath()
                            ctx.moveTo(x, top)
                            ctx.lineTo(x, top + plotH)
                            ctx.stroke()

                            ctx.fillStyle = "#94a3b8"
                            ctx.textAlign = "center"
                            ctx.fillText(chartRoot.timeLabels[tx], x, h - 6)
                        }

                        function drawThresholdLine(value, color, label) {
                            if (value <= 0)
                                return

                            var y = valueToY(value)

                            ctx.strokeStyle = color
                            ctx.lineWidth = 1
                            ctx.setLineDash([5, 4])
                            ctx.beginPath()
                            ctx.moveTo(left, y)
                            ctx.lineTo(left + plotW, y)
                            ctx.stroke()
                            ctx.setLineDash([])

                            ctx.fillStyle = color
                            ctx.font = "11px sans-serif"
                            ctx.textAlign = "right"
                            ctx.fillText(label + " " + value + chartRoot.unit, left + plotW - 4, y - 4)
                        }

                        // threshold lines
                        drawThresholdLine(chartRoot.normalValue, "#94a3b8", "정상")
                        drawThresholdLine(chartRoot.warningValue, "#f97316", "주의")
                        drawThresholdLine(chartRoot.alarmValue, "#dc2626", "경고")

                        // border
                        ctx.strokeStyle = "#cbd5e1"
                        ctx.strokeRect(left, top, plotW, plotH)

                        // series
                        for (var si = 0; si < chartRoot.series.length; ++si) {
                            var item = chartRoot.series[si]
                            var values = item.values
                            var color = chartRoot.lineColors[si % chartRoot.lineColors.length]

                            if (!values || values.length === 0)
                                continue

                            ctx.strokeStyle = color
                            ctx.fillStyle = color
                            ctx.lineWidth = 2
                            ctx.beginPath()

                            for (var vi = 0; vi < values.length; ++vi) {
                                var px = indexToX(vi, values.length)
                                var py = valueToY(values[vi])

                                if (vi === 0)
                                    ctx.moveTo(px, py)
                                else
                                    ctx.lineTo(px, py)
                            }
                            ctx.stroke()

                            for (var pi = 0; pi < values.length; ++pi) {
                                var cx = indexToX(pi, values.length)
                                var cy = valueToY(values[pi])
                                ctx.beginPath()
                                ctx.arc(cx, cy, 3, 0, Math.PI * 2)
                                ctx.fill()
                            }
                        }
                    }

                    Component.onCompleted: requestPaint()
                    onWidthChanged: requestPaint()
                    onHeightChanged: requestPaint()

                    Connections {
                        target: chartRoot

                        function onNormalValueChanged() { lineCanvas.requestPaint() }
                        function onWarningValueChanged() { lineCanvas.requestPaint() }
                        function onAlarmValueChanged() { lineCanvas.requestPaint() }
                        function onMinValueChanged() { lineCanvas.requestPaint() }
                        function onMaxValueChanged() { lineCanvas.requestPaint() }
                        function onSeriesChanged() { lineCanvas.requestPaint() }
                        function onTimeLabelsChanged() { lineCanvas.requestPaint() }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 16
                Layout.fillHeight: false
                spacing: 7

                Repeater {
                    model: chartRoot.series

                    RowLayout {
                        spacing: 4

                        Rectangle {
                            width: 14
                            height: 3
                            radius: 2
                            color: chartRoot.lineColors[index % chartRoot.lineColors.length]
                        }

                        Text {
                            text: modelData.axis
                            color: "#475569"
                            font.pixelSize: 11
                        }
                    }
                }

                Item { Layout.fillWidth: true }
            }
        }
    }
}
