import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// =============================================================
// IoTPanel.qml — 로봇 2대 실시간 IoT 모니터링 패널
// 스타일: RobotPanel / VisionPanel / MainPanel 디자인 언어 통일
//   - 배경: #f0f2f5
//   - 카드:  white / #e0e4ea border / radius 8
//   - 폰트: "Asta Sans"
//   - 값 색상: #212121 (강조), 라벨: #9e9e9e
//   - 액센트: #1976d2
// =============================================================
Rectangle {
    id: root
    color: "#f0f2f5"

    // ── 레이아웃 튜닝 ──────────────────────────────────────────
    property int summaryCardHeight:     76
    property int bottomPanelHeight:    180 //158
    property int chartMinHeight:       110
    property int chartPreferredHeight: 180
    property int sectionSpacing:         8
    property int toolBarHeight:         34

    // ── 시그널 ────────────────────────────────────────────────
    signal thresholdSettingRequested()
    signal historyQueryRequested()

    // ── 축 색상 ───────────────────────────────────────────────
    property var axisColors: [
        "#2563eb",  // J1
        "#059669",  // J2
        "#7c3aed",  // J3
        "#0891b2",  // J4
        "#e85d04",  // J5
        "#64748b"   // J6
    ]
    property var timeLabels: ["-50s", "-40s", "-30s", "-20s", "-10s", "Now"]

    property bool hasIotViewModel: typeof iotViewModel !== "undefined" && iotViewModel !== null
    // ── 샘플 데이터 ───────────────────────────────────────────
    property var sampleRobotModels: [
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
            torqueSeries: [
                { axis: "J1", values: [10.2, 10.8, 11.4, 11.7, 12.0, 12.3] },
                { axis: "J2", values: [8.4,  8.7,  9.1,  9.4,  9.6,  9.8]  },
                { axis: "J3", values: [13.1, 13.7, 14.4, 15.0, 15.3, 15.6] },
                { axis: "J4", values: [9.8,  10.2, 10.7, 10.9, 11.0, 11.2] },
                { axis: "J5", values: [7.2,  7.5,  7.9,  8.1,  8.3,  8.4]  },
                { axis: "J6", values: [9.1,  9.5,  10.0, 10.4, 10.7, 10.9] }
            ],
            alarms: [
                { time: "17:04:58", level: "WARN", message: "J3 부하 임계치 접근" },
                { time: "17:04:41", level: "INFO", message: "실시간 갱신 정상" }
            ],
            prediction: {
                riskLevel: "주의",
                score: 82,
                causeText: "J3 부하 상승",
                recommendedAction: "J3 축 부하 추세 점검 권장"
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
            torqueSeries: [
                { axis: "J1", values: [11.8, 12.4, 13.0, 13.5, 13.8, 14.1] },
                { axis: "J2", values: [9.0,  9.4,  9.7,  10.0, 10.3, 10.6] },
                { axis: "J3", values: [12.0, 12.5, 13.0, 13.3, 13.6, 13.8] },
                { axis: "J4", values: [13.7, 14.2, 14.9, 15.4, 15.8, 16.2] },
                { axis: "J5", values: [8.1,  8.5,  8.8,  9.0,  9.3,  9.5]  },
                { axis: "J6", values: [10.2, 10.6, 10.9, 11.2, 11.5, 11.7] }
            ],
            alarms: [
                { time: "17:04:57", level: "WARN", message: "J4 부하 초과 감지" },
                { time: "17:04:32", level: "INFO", message: "알람 표시 정상" }
            ],
            prediction: {
                riskLevel: "경고",
                score: 91,
                causeText: "J4 부하 초과",
                recommendedAction: "J4 축 이상 징후 감지, 점검 요청"
            }
        }
    ]
    // 실제 모델 바인딩 (iotViewModel이 있을 때만)
    property var robotModels: root.hasIotViewModel
                              ? iotViewModel.robotModels
                              : root.sampleRobotModels

    // ── 임계값 상태 ───────────────────────────────────────────
    property var sampleRobotThresholds: [
        { temperature: { normalMax: 55, warningMax: 60, alarmMax: 70 },
          torque:     { normalMax: 12, warningMax: 14, alarmMax: 16 } },
        { temperature: { normalMax: 55, warningMax: 60, alarmMax: 70 },
          torque:     { normalMax: 12, warningMax: 14, alarmMax: 16 } }
    ]
    // 실제 모델 바인딩 (iotViewModel이 있을 때만)
    property var robotThresholds: root.hasIotViewModel
                                  ? iotViewModel.robotThresholds
                                  : root.sampleRobotThresholds
    // 임계값 기본값 반환
    function defaultThreshold() {
        return {
            temperature: { normalMax: 55, warningMax: 60, alarmMax: 70 },
            torque:     { normalMax: 12, warningMax: 14, alarmMax: 16 }
        }
    }
    // 임계값 데이터 정규화: 누락된 값은 기본값으로 채우고, 숫자 타입으로 변환
    function normalizeThreshold(thresholdData) {
        var fallback = root.defaultThreshold()
        if (!thresholdData) return fallback
        var temp    = thresholdData.temperature || fallback.temperature
        var torque = thresholdData.torque || fallback.torque
        return {
            temperature: { normalMax: Number(temp.normalMax),    warningMax: Number(temp.warningMax),    alarmMax: Number(temp.alarmMax)    },
            torque:     { normalMax: Number(torque.normalMax), warningMax: Number(torque.warningMax), alarmMax: Number(torque.alarmMax) }
        }
    }
    // robotIndex는 1-based, 내부 저장은 0-based 배열로 관리
    function thresholdFor(robotIndex) {
        if (!root.robotThresholds || robotIndex < 0 || robotIndex >= root.robotThresholds.length)
            return root.defaultThreshold()
        return root.normalizeThreshold(root.robotThresholds[robotIndex])
    }
    // 임계값 적용: robotIndex는 1-based, 내부 저장은 0-based 배열로 관리
    function applyThreshold(robotIndex, thresholdData) {
        if (!thresholdData) return
        var targetIndex = robotIndex - 1
        if (targetIndex < 0) return
        var copied = root.robotThresholds ? root.robotThresholds.slice() : []
        while (copied.length <= targetIndex) copied.push(root.defaultThreshold())
        copied[targetIndex] = root.normalizeThreshold(thresholdData)
        root.robotThresholds = copied
    }
    // 시리즈 아이템에서 최신값 반환, 값이 없으면 0 반환
    function latestValue(seriesItem) {
        if (!seriesItem || !seriesItem.values || seriesItem.values.length === 0) return 0
        return seriesItem.values[seriesItem.values.length - 1]
    }
    // 시리즈 배열에서 최신값이 가장 큰 축 정보 반환 (예: { axis: "J4", value: 46.3 })
    function maxLatestInfo(series) {
        var maxValue = -999999
        var maxAxis  = "-"
        for (var i = 0; i < series.length; ++i) {
            var v = latestValue(series[i])
            if (v > maxValue) { maxValue = v; maxAxis = series[i].axis }
        }
        return { axis: maxAxis, value: maxValue }
    }
    // 값과 임계값의 비율에 따른 위험도 레벨 반환: ratio >= 1.15 -> "경고", ratio >= 1.0 -> "주의", else "정상"
    function riskLevelFromRatio(ratio) {
        if (ratio >= 1.15)
            return "경고"

        if (ratio >= 1.0)
            return "주의"

        return "정상"
    }
    // 로봇 데이터와 임계값을 기반으로 워스트 축 정보 반환 (예: { axis: "J4", metric: "온도", value: 46.3, ratio: 1.15, level: "경고" })
    function worstAxisInfo(robotData, threshold) {
        var worst = {
            axis: "-",
            metric: "-",
            value: 0,
            ratio: 0,
            level: "정상"
        }

        if (!robotData || !threshold)
            return worst

        function updateWorst(series, metric, warningMax) {
            if (!series || warningMax <= 0)
                return

            for (var i = 0; i < series.length; ++i) {
                var v = root.latestValue(series[i])
                var ratio = v / warningMax

                if (ratio > worst.ratio) {
                    worst.axis = series[i].axis
                    worst.metric = metric
                    worst.value = v
                    worst.ratio = ratio
                    worst.level = root.riskLevelFromRatio(ratio)
                }
            }
        }

        updateWorst(robotData.tempSeries, "온도", threshold.temperature.warningMax)
        updateWorst(robotData.torqueSeries, "부하", threshold.torque.warningMax)

        return worst
    }
    // 알람 시간 표시: time > occurredAt > occurred_at > "-"
    function alarmDisplayTime(alarm) {
        if (!alarm)
            return "-"

        if (alarm.time !== undefined && alarm.time !== "")
            return alarm.time

        if (alarm.occurredAt !== undefined && alarm.occurredAt.length >= 19)
            return alarm.occurredAt.substr(11, 8)

        if (alarm.occurred_at !== undefined && alarm.occurred_at.length >= 19)
            return alarm.occurred_at.substr(11, 8)

        return "-"
    }
    // 알람 레벨 표시: level > severity > "-"
    function alarmLevelLabel(level) {
        if (level === "ALARM")
            return "ALARM"

        if (level === "WARNING" || level === "WARN")
            return "WARN"

        if (level === "INFO")
            return "INFO"

        return level !== undefined && level !== "" ? level : "-"
    }
    // 알람 레벨별 색상: ALARM - 빨강, WARN - 노랑, INFO/기타 - 회색
    function alarmBackground(level) {
        if (level === "ALARM")
            return "#ffebee"

        if (level === "WARNING" || level === "WARN")
            return "#fff8e1"

        return "#f5f5f5"
    }
    // 알람 레벨별 테두리 색상: ALARM - 진한 빨강, WARN - 진한 노랑, INFO/기타 - 회색
    function alarmBorder(level) {
        if (level === "ALARM")
            return "#ef9a9a"

        if (level === "WARNING" || level === "WARN")
            return "#ffe082"

        return "#e0e4ea"
    }
    // 알람 레벨별 배지 색상: ALARM - 빨강, WARN - 노랑, INFO/기타 - 회색
    function alarmBadgeColor(level) {
        if (level === "ALARM")
            return "#c62828"

        if (level === "WARNING" || level === "WARN")
            return "#f9a825"

        return "#9e9e9e"
    }
    // ── 다이얼로그 ────────────────────────────────────────────
    ThresholdSettingDialog {
        id: thresholdDialog
        hostWidth:  root.width
        hostHeight: root.height
        thresholds: root.robotThresholds
        onSaveRequested: function(robotIndex, thresholdData) {
            if (root.hasIotViewModel) {
                if (!iotViewModel.saveThreshold(robotIndex, thresholdData)) {
                    console.warn("[QML] Failed to save threshold:", robotIndex)
                }
                return
            }

            root.applyThreshold(robotIndex, thresholdData)
        }
    }

    HistoryQueryDialog {
        id: historyDialog
        hostWidth:  root.width
        hostHeight: root.height
        viewModel: root.hasIotViewModel ? iotViewModel : null

        onExportCsvRequested: function(rows, robotFilter, periodFilter, typeFilter, searchText) {
            console.log("[QML] CSV export requested, rows = " + rows.length)

            if (root.hasIotViewModel) {
                if (!iotViewModel.exportHistoryCsv(rows)) {
                    console.warn("[QML] CSV export failed:", iotViewModel.lastError)
                } else {
                    console.log("[QML] CSV exported:", iotViewModel.lastExportPath)
                }
            }
        }

        onAlarmConfirmRequested: function(alarmRow) {
            console.log("[QML] alarm confirm requested:",
                        alarmRow.id,
                        alarmRow.robot,
                        alarmRow.axis)

            if (!root.hasIotViewModel) {
                console.warn("[QML] IoTViewModel is not available")
                return
            }

            if (!iotViewModel.confirmAlarmAction(alarmRow)) {
                console.warn("[QML] alarm confirm failed:",
                             iotViewModel.lastError)
                return
            }

            historyDialog.requestHistoryQuery()
        }

        onAlarmActionRequested: function(alarmRow) {
            console.log("[QML] alarm action requested:",
                        alarmRow.id,
                        alarmRow.robot,
                        alarmRow.axis)
        }
    }

    // ═══════════════════════════════════════════════════════════
    // 메인 레이아웃
    // ═══════════════════════════════════════════════════════════
    ColumnLayout {
        anchors.fill:    parent
        anchors.margins: 12
        spacing:         root.sectionSpacing

        // ── 상단 툴바 ─────────────────────────────────────────
        RowLayout {
            Layout.fillWidth:       true
            Layout.preferredHeight: root.toolBarHeight
            Layout.minimumHeight:   root.toolBarHeight
            Layout.maximumHeight:   root.toolBarHeight
            Layout.fillHeight:      false
            spacing: 8

            Text {
                text:           "IoT 모니터링"
                font.family:    "Asta Sans"
                font.pixelSize: 20
                font.bold:      true
                color:          "#212121"
            }

            Item { Layout.fillWidth: true }

            IoTToolButton {
                text:                 "임계값 설정"
                Layout.preferredWidth: 112
                onClicked: {
                    root.thresholdSettingRequested()
                    thresholdDialog.open()
                }
            }

            IoTToolButton {
                text:                 "이력 조회"
                Layout.preferredWidth: 96
                onClicked: {
                    root.historyQueryRequested()
                    historyDialog.open()
                }
            }
        }

        // ── 로봇 2대 카드 영역 ────────────────────────────────
        RowLayout {
            Layout.fillWidth:  true
            Layout.fillHeight: true
            spacing: 12

            Repeater {
                model: root.robotModels

                // ── 개별 Robot 카드 ───────────────────────────
                Rectangle {
                    id: robotCard
                    property var robotData:  modelData
                    property int robotIndex: index

                    Layout.fillWidth:  true
                    Layout.fillHeight: true

                    radius:       8
                    color:        "#ffffff"
                    border.color: "#e0e4ea"
                    border.width: 1

                    ColumnLayout {
                        anchors.fill:    parent
                        anchors.margins: 14
                        spacing:         root.sectionSpacing

                        // 카드 제목 행
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Rectangle {
                                width:  4
                                height: 22
                                radius: 2
                                color:  "#1976d2"
                            }

                            Text {
                                text:           robotCard.robotData.name
                                font.family:    "Asta Sans"
                                font.pixelSize: 18
                                font.bold:      true
                                color:          "#212121"
                            }

                            Item { Layout.fillWidth: true }

                            Rectangle {
                                width:        58
                                height:       22
                                radius:       11
                                color:        robotCard.robotData.running ? "#e8f5e9" : "#ffebee"
                                border.color: robotCard.robotData.running ? "#66bb6a" : "#ef9a9a"
                                border.width: 1

                                Text {
                                    anchors.centerIn: parent
                                    text:           robotCard.robotData.running ? "RUN" : "STOP"
                                    font.family:    "Asta Sans"
                                    font.pixelSize: 11
                                    font.bold:      true
                                    color:          robotCard.robotData.running ? "#2e7d32" : "#c62828"
                                }
                            }
                        }

                        // 요약 카드 행
                        RowLayout {
                            Layout.fillWidth:       true
                            Layout.preferredHeight: root.summaryCardHeight
                            Layout.minimumHeight:   root.summaryCardHeight
                            Layout.maximumHeight:   root.summaryCardHeight
                            Layout.fillHeight:      false
                            spacing: 8

                            MetricCard {
                                Layout.fillWidth: true
                                cardLabel:    "Max 온도"
                                cardValue:    root.maxLatestInfo(robotCard.robotData.tempSeries).value.toFixed(1)
                                cardUnit:     "°C"
                                cardSub:      root.maxLatestInfo(robotCard.robotData.tempSeries).axis
                                accentColor:  root.maxLatestInfo(robotCard.robotData.tempSeries).value
                                              >= root.thresholdFor(robotCard.robotIndex).temperature.warningMax
                                              ? "#c62828" : "#1976d2"
                            }

                            MetricCard {
                                Layout.fillWidth: true
                                cardLabel:    "Max 부하"
                                cardValue:    root.maxLatestInfo(robotCard.robotData.torqueSeries).value.toFixed(1)
                                cardUnit:     "raw"
                                cardSub:      root.maxLatestInfo(robotCard.robotData.torqueSeries).axis
                                accentColor:  root.maxLatestInfo(robotCard.robotData.torqueSeries).value
                                              >= root.thresholdFor(robotCard.robotIndex).torque.warningMax
                                              ? "#c62828" : "#2e7d32"
                            }

                            MetricCard {
                                Layout.fillWidth: true

                                property var threshold: root.thresholdFor(robotCard.robotIndex)
                                property var worst: root.worstAxisInfo(robotCard.robotData, threshold)

                                cardLabel: "종합 위험축"
                                cardValue: worst.axis
                                cardUnit: ""
                                cardSub: worst.metric + " 기준 / " + worst.level
                                accentColor: worst.level === "경고"
                                             ? "#c62828"
                                             : worst.level === "주의"
                                               ? "#f9a825"
                                               : "#2e7d32"
                            }
                        }

                        // 온도 그래프
                        AxisLineChart {
                            Layout.fillWidth:       true
                            Layout.fillHeight:      true
                            Layout.minimumHeight:   root.chartMinHeight
                            Layout.preferredHeight: root.chartPreferredHeight
                            chartTitle:   "축별 온도 추세"
                            unit:         "°C"
                            minValue:     30
                            maxValue:     80
                            normalValue:  root.thresholdFor(robotCard.robotIndex).temperature.normalMax
                            warningValue: root.thresholdFor(robotCard.robotIndex).temperature.warningMax
                            alarmValue:   root.thresholdFor(robotCard.robotIndex).temperature.alarmMax
                            timeLabels:   root.timeLabels
                            series:       robotCard.robotData.tempSeries
                            lineColors:   root.axisColors
                        }

                        // 부하 그래프
                        AxisLineChart {
                            Layout.fillWidth:       true
                            Layout.fillHeight:      true
                            Layout.minimumHeight:   root.chartMinHeight
                            Layout.preferredHeight: root.chartPreferredHeight
                            chartTitle:   "축별 부하 추세"
                            unit:         "raw"
                            minValue:     0
                            maxValue:     20
                            normalValue:  root.thresholdFor(robotCard.robotIndex).torque.normalMax
                            warningValue: root.thresholdFor(robotCard.robotIndex).torque.warningMax
                            alarmValue:   root.thresholdFor(robotCard.robotIndex).torque.alarmMax
                            timeLabels:   root.timeLabels
                            series:       robotCard.robotData.torqueSeries
                            lineColors:   root.axisColors
                        }

                        // 하단: 알람 이력 + 위험도 패널
                        RowLayout {
                            Layout.fillWidth:       true
                            Layout.preferredHeight: root.bottomPanelHeight
                            Layout.minimumHeight:   root.bottomPanelHeight
                            Layout.maximumHeight:   root.bottomPanelHeight
                            Layout.fillHeight:      false
                            spacing: 8

                            // 알람 이력
                            Rectangle {
                                Layout.fillWidth:  true
                                Layout.fillHeight: true
                                radius:       6
                                color:        "#fafafa"
                                border.color: "#e0e4ea"
                                border.width: 1

                                ColumnLayout {
                                    anchors.fill:    parent
                                    anchors.margins: 10
                                    spacing:          6

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 6
                                        Rectangle { width: 3; height: 14; radius: 1; color: "#f9a825" }
                                        Text {
                                            text:           "최근 알람"
                                            font.family:    "Asta Sans"
                                            font.pixelSize: 13
                                            font.bold:      true
                                            color:          "#212121"
                                        }
                                    }

                                    Repeater {
                                        model: robotCard.robotData.alarms.slice(0, 3)

                                        Rectangle {
                                            Layout.fillWidth:       true
                                            Layout.preferredHeight: 32
                                            Layout.minimumHeight:   32
                                            Layout.maximumHeight:   32
                                            Layout.fillHeight:      false
                                            radius:       4
                                            color:        root.alarmBackground(modelData.level)
                                            border.color: root.alarmBorder(modelData.level)
                                            border.width: 1

                                            RowLayout {
                                                anchors.fill:    parent
                                                anchors.margins: 6
                                                spacing:         6

                                                Rectangle {
                                                    width:  36
                                                    height: 18
                                                    radius:  9
                                                    color: root.alarmBadgeColor(modelData.level)

                                                    Text {
                                                        anchors.centerIn: parent
                                                        text:           root.alarmLevelLabel(modelData.level)
                                                        color:          "white"
                                                        font.family:    "Asta Sans"
                                                        font.pixelSize: 9
                                                        font.bold:      true
                                                    }
                                                }

                                                Text {
                                                    text:                  root.alarmDisplayTime(modelData)
                                                    color:                 "#757575"
                                                    font.family:           "Asta Sans"
                                                    font.pixelSize:        11
                                                    Layout.preferredWidth: 56
                                                }

                                                Text {
                                                    text:           modelData.message
                                                    color:          "#374151"
                                                    font.family:    "Asta Sans"
                                                    font.pixelSize: 12
                                                    Layout.fillWidth: true
                                                    elide:          Text.ElideRight
                                                }
                                            }
                                        }
                                    }

                                    Item { Layout.fillHeight: true }
                                }
                            }

                            // 위험도 / 조치
                            Rectangle {
                                Layout.fillWidth:  true
                                Layout.fillHeight: true
                                radius:       6
                                color:        "#fafafa"
                                border.color: "#e0e4ea"
                                border.width: 1
                                clip:         true

                                ColumnLayout {
                                    anchors.fill:    parent
                                    anchors.margins: 10
                                    spacing:         6

                                    // 헤더: 항상 고정
                                    RowLayout {
                                        Layout.fillWidth:       true
                                        Layout.preferredHeight: 20
                                        Layout.minimumHeight:   20
                                        Layout.maximumHeight:   20
                                        Layout.fillHeight:      false
                                        spacing: 6
                                        Rectangle { width: 3; height: 14; radius: 1; color: "#1976d2" }
                                        Text {
                                            text:           "위험도 / 권장 조치"
                                            font.family:    "Asta Sans"
                                            font.pixelSize: 13
                                            font.bold:      true
                                            color:          "#212121"
                                        }
                                    }

                                    // 콘텐츠만 스크롤
                                    Flickable {
                                        id: riskFlick
                                        Layout.fillWidth:  true
                                        Layout.fillHeight: true
                                        contentWidth:  width
                                        contentHeight: riskCol.implicitHeight
                                        flickableDirection: Flickable.VerticalFlick
                                        boundsBehavior: Flickable.DragAndOvershootBounds
                                        clip: true

                                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                                        ColumnLayout {
                                            id: riskCol
                                            width: riskFlick.width
                                            spacing: 6

                                            RiskBlock {
                                                Layout.fillWidth: true
                                                riskLabel:    "위험도"
                                                riskValue:    robotCard.robotData.prediction.riskLevel
                                                              + " (" + robotCard.robotData.prediction.score + ")"
                                                riskDetail:   robotCard.robotData.prediction.score >= 90
                                                              ? "즉시 점검 필요" : "추세 관찰 필요"
                                                valueColor:   robotCard.robotData.prediction.score >= 90
                                                              ? "#c62828" : "#f9a825"
                                            }

                                            RiskBlock {
                                                Layout.fillWidth: true
                                                riskLabel:    "원인"
                                                riskValue:    robotCard.robotData.prediction.causeText
                                                riskDetail:   "이상 판단 원인"
                                                valueColor:   "#374151"
                                            }

                                            RiskBlock {
                                                Layout.fillWidth: true
                                                riskLabel:    "조치"
                                                riskValue:    robotCard.robotData.prediction.recommendedAction
                                                riskDetail:   "작업자 확인 항목"
                                                valueColor:   "#374151"
                                                wrapValue:    true
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
    }

    // ═══════════════════════════════════════════════════════════
    // [컴포넌트] MetricCard
    // ═══════════════════════════════════════════════════════════
    component MetricCard: Rectangle {
        id: mc

        Layout.fillWidth:  true
        Layout.fillHeight: true

        property string cardLabel:   ""
        property string cardValue:   "-"
        property string cardUnit:    ""
        property string cardSub:     ""
        property color  accentColor: "#1976d2"

        radius:       6
        color:        "#f5f7fa"
        border.color: "#e0e4ea"
        border.width: 1
        clip:         true

        ColumnLayout {
            anchors.fill:    parent
            anchors.margins: 6
            spacing:         2

            Text {
                Layout.fillWidth: true
                text:           mc.cardLabel
                font.family:    "Asta Sans"
                font.pixelSize: 11
                color:          "#9e9e9e"
                elide:          Text.ElideRight
            }

            RowLayout {
                Layout.fillWidth:       true
                Layout.preferredHeight: 30
                spacing: 3

                Text {
                    Layout.fillWidth: true
                    text:           mc.cardValue
                    font.family:    "Asta Sans"
                    font.pixelSize: 22
                    font.bold:      true
                    color:          mc.accentColor
                    elide:          Text.ElideRight
                }

                Text {
                    text:            mc.cardUnit
                    font.family:     "Asta Sans"
                    font.pixelSize:  13
                    color:           mc.accentColor
                    visible:         mc.cardUnit !== ""
                    Layout.alignment: Qt.AlignVCenter | Qt.AlignBottom
                    Layout.bottomMargin: 3
                }
            }

            Text {
                Layout.fillWidth: true
                text:           mc.cardSub
                font.family:    "Asta Sans"
                font.pixelSize: 11
                color:          "#9e9e9e"
                elide:          Text.ElideRight
            }
        }
    }

    // ═══════════════════════════════════════════════════════════
    // [컴포넌트] RiskBlock
    // ═══════════════════════════════════════════════════════════
    component RiskBlock: Rectangle {
        id: rb

        property string riskLabel:  ""
        property string riskValue:  "-"
        property string riskDetail: ""
        property color  valueColor: "#374151"
        property bool   wrapValue:  false

        Layout.fillWidth:     true
        Layout.fillHeight:    true
        Layout.minimumHeight: 40

        radius:       4
        color:        "#ffffff"
        border.color: "#e0e4ea"
        border.width: 1

        RowLayout {
            anchors.fill:    parent
            anchors.margins: 6
            spacing:         8

            Text {
                text:                  rb.riskLabel
                font.family:           "Asta Sans"
                font.pixelSize:        11
                font.bold:             true
                color:                 "#9e9e9e"
                Layout.preferredWidth: 38
                verticalAlignment:     Text.AlignVCenter
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 1

                Text {
                    Layout.fillWidth: true
                    text:           rb.riskValue
                    font.family:    "Asta Sans"
                    font.pixelSize: 13
                    font.bold:      rb.riskLabel === "위험도"
                    color:          rb.valueColor
                    wrapMode:       rb.wrapValue ? Text.WordWrap : Text.NoWrap
                    elide:          rb.wrapValue ? Text.ElideNone : Text.ElideRight
                    maximumLineCount: rb.wrapValue ? 2 : 1
                }

                Text {
                    Layout.fillWidth: true
                    text:           rb.riskDetail
                    font.family:    "Asta Sans"
                    font.pixelSize: 10
                    color:          "#bdbdbd"
                    elide:          Text.ElideRight
                }
            }
        }
    }

    // ═══════════════════════════════════════════════════════════
    // [컴포넌트] IoTToolButton
    // ═══════════════════════════════════════════════════════════
    component IoTToolButton: Button {
        id: tb

        Layout.preferredHeight: 30
        Layout.minimumHeight:   30
        Layout.maximumHeight:   30
        Layout.fillHeight:      false

        contentItem: Text {
            text:                tb.text
            font.family:         "Asta Sans"
            font.pixelSize:      13
            color:               "#374151"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment:   Text.AlignVCenter
            elide:               Text.ElideRight
        }

        background: Rectangle {
            radius:       4
            color:        tb.down    ? "#cbd5e1"
                        : tb.hovered ? "#e5e7eb"
                        :              "#f1f5f9"
            border.color: "#d1d5db"
            border.width: 1
        }
    }

    // ═══════════════════════════════════════════════════════════
    // [컴포넌트] AxisLineChart
    // Canvas 기반 라인 그래프
    // Y축 레이블 클리핑 방지: left 마진 48px, clip: true
    // ═══════════════════════════════════════════════════════════
    component AxisLineChart: Rectangle {
        id: chart

        property string chartTitle:   ""
        property string unit:         ""
        property real   minValue:      0
        property real   maxValue:    100
        property real   normalValue:   0
        property real   warningValue:  0
        property real   alarmValue:    0
        property var    timeLabels:   []
        property var    series:       []
        property var    lineColors:   []

        radius:       6
        color:        "#ffffff"
        border.color: "#e0e4ea"
        border.width: 1
        clip:         true

        ColumnLayout {
            anchors.fill:    parent
            anchors.margins: 6
            spacing:         2

            // 제목 행 (범례 제거 — Row positioner는 RowLayout 안에서 implicitWidth가 0이 되어 겹침 발생)
            RowLayout {
                Layout.fillWidth:       true
                Layout.preferredHeight: 16
                Layout.fillHeight:      false

                Text {
                    text:           chart.chartTitle
                    font.family:    "Asta Sans"
                    font.pixelSize: 12
                    font.bold:      true
                    color:          "#212121"
                }

                Item { Layout.fillWidth: true }

                Text {
                    text:           chart.unit + "  정상 " + chart.normalValue + "  /  주의 " + chart.warningValue + "  /  경고 " + chart.alarmValue
                    font.family:    "Asta Sans"
                    font.pixelSize: 10
                    color:          "#9e9e9e"
                }
            }

            // 그래프 영역
            Rectangle {
                Layout.fillWidth:  true
                Layout.fillHeight: true
                radius:       4
                color:        "#f5f7fa"
                border.color: "#e0e4ea"
                border.width: 1
                clip:         true

                Canvas {
                    id: cv
                    anchors.fill: parent

                    property var _series:       chart.series
                    property real _normalValue: chart.normalValue
                    property real _warningValue: chart.warningValue
                    property real _alarmValue:  chart.alarmValue

                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.clearRect(0, 0, width, height)

                        var w = width, h = height
                        // Y축 레이블이 잘리지 않도록 left 충분히 확보
                        var left = 48, right = 14, top = 14, bottom = 22
                        var plotW = w - left - right
                        var plotH = h - top - bottom
                        if (plotW <= 0 || plotH <= 0) return

                        function valueToY(v) {
                            var r = (v - chart.minValue) / (chart.maxValue - chart.minValue)
                            r = Math.max(0, Math.min(1, r))
                            return top + plotH * (1 - r)
                        }
                        function indexToX(i, cnt) {
                            if (cnt <= 1) return left
                            return left + plotW * i / (cnt - 1)
                        }

                        ctx.font = "11px 'Segoe UI', sans-serif"

                        // 수평 그리드 + Y 레이블
                        var gridCount = 4
                        ctx.textBaseline = "middle"
                        for (var gy = 0; gy <= gridCount; ++gy) {
                            var gy_y = top + plotH * gy / gridCount
                            ctx.strokeStyle = "#e0e4ea"
                            ctx.lineWidth   = 1
                            ctx.setLineDash([])
                            ctx.beginPath(); ctx.moveTo(left, gy_y); ctx.lineTo(left + plotW, gy_y); ctx.stroke()
                            var lv = chart.maxValue - (chart.maxValue - chart.minValue) * gy / gridCount
                            ctx.fillStyle = "#9e9e9e"
                            ctx.textAlign = "right"
                            ctx.fillText(lv.toFixed(0), left - 4, gy_y)
                        }

                        // 수직 그리드 + X 레이블
                        ctx.textBaseline = "alphabetic"
                        for (var tx = 0; tx < chart.timeLabels.length; ++tx) {
                            var tx_x = indexToX(tx, chart.timeLabels.length)
                            ctx.strokeStyle = "#eeeeee"
                            ctx.lineWidth   = 1
                            ctx.setLineDash([3, 3])
                            ctx.beginPath(); ctx.moveTo(tx_x, top); ctx.lineTo(tx_x, top + plotH); ctx.stroke()
                            ctx.setLineDash([])
                            ctx.fillStyle  = "#9e9e9e"
                            var isFirst = (tx === 0)
                            var isLast  = (tx === chart.timeLabels.length - 1)
                            ctx.textAlign  = isLast ? "right" : (isFirst ? "left" : "center")
                            ctx.fillText(chart.timeLabels[tx], tx_x, top + plotH + 14)
                        }

                        // 임계값 라인
                        function drawLine(val, col) {
                            if (val <= 0) return
                            var ty = valueToY(val)
                            ctx.strokeStyle = col; ctx.lineWidth = 1
                            ctx.setLineDash([5, 4])
                            ctx.beginPath(); ctx.moveTo(left, ty); ctx.lineTo(left + plotW, ty); ctx.stroke()
                            ctx.setLineDash([])
                        }
                        drawLine(chart.normalValue,  "#9e9e9e")
                        drawLine(chart.warningValue, "#f9a825")
                        drawLine(chart.alarmValue,   "#c62828")

                        // 외곽선
                        ctx.strokeStyle = "#d0d0d0"; ctx.lineWidth = 1; ctx.setLineDash([])
                        ctx.strokeRect(left, top, plotW, plotH)

                        // 데이터 시리즈
                        for (var si = 0; si < chart.series.length; ++si) {
                            var item   = chart.series[si]
                            var vals   = item.values
                            var col    = chart.lineColors[si % chart.lineColors.length]
                            if (!vals || vals.length === 0) continue

                            ctx.strokeStyle = col; ctx.fillStyle = col
                            ctx.lineWidth   = 1.8; ctx.setLineDash([])
                            ctx.beginPath()
                            for (var vi = 0; vi < vals.length; ++vi) {
                                var vx = indexToX(vi, vals.length)
                                var vy = valueToY(vals[vi])
                                if (vi === 0) ctx.moveTo(vx, vy); else ctx.lineTo(vx, vy)
                            }
                            ctx.stroke()

                            for (var pi = 0; pi < vals.length; ++pi) {
                                var px = indexToX(pi, vals.length)
                                var py = valueToY(vals[pi])
                                ctx.beginPath(); ctx.arc(px, py, 2.5, 0, Math.PI * 2); ctx.fill()
                            }
                        }
                    }

                    on_SeriesChanged:        requestPaint()
                    on_NormalValueChanged:   requestPaint()
                    on_WarningValueChanged:  requestPaint()
                    on_AlarmValueChanged:    requestPaint()
                    onWidthChanged:          requestPaint()
                    onHeightChanged:         requestPaint()
                }
            }
        }
    }
}
