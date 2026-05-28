import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import "../components"

Rectangle {
    id: root
    color: "#f0f2f5"

    // Robot jog display/control mode
    // "joint" or "base"
    property string robot1JogMode: "joint"
    property string robot2JogMode: "joint"

    property bool robotManualMode: true
    function jointIndexFromName(name) {
        if (typeof name !== "string")
            return 0

        if (name.length < 2 || name.charAt(0) !== "J")
            return 0

        var index = parseInt(name.substring(1))
        return index >= 1 && index <= 6 ? index : 0
    }

    property var robot1JointModel: [
        { name: "J1", value: "90.264", unit: "deg" },
        { name: "J2", value: "-111.039", unit: "deg" },
        { name: "J3", value: "118.105", unit: "deg" },
        { name: "J4", value: "-96.669", unit: "deg" },
        { name: "J5", value: "-89.814", unit: "deg" },
        { name: "J6", value: "4.319", unit: "deg" }
    ]

    property var robot2JointModel: [
        { name: "J1", value: "0.000", unit: "deg" },
        { name: "J2", value: "0.000", unit: "deg" },
        { name: "J3", value: "0.000", unit: "deg" },
        { name: "J4", value: "0.000", unit: "deg" },
        { name: "J5", value: "0.000", unit: "deg" },
        { name: "J6", value: "0.000", unit: "deg" }
    ]

    property var robot1BaseModel: [
        { name: "X",  value: "0.000", unit: "mm" },
        { name: "Y",  value: "0.000", unit: "mm" },
        { name: "Z",  value: "0.000", unit: "mm" },
        { name: "RX", value: "0.000", unit: "deg" },
        { name: "RY", value: "0.000", unit: "deg" },
        { name: "RZ", value: "0.000", unit: "deg" }
    ]

    property var robot2BaseModel: [
        { name: "X",  value: "0.000", unit: "mm" },
        { name: "Y",  value: "0.000", unit: "mm" },
        { name: "Z",  value: "0.000", unit: "mm" },
        { name: "RX", value: "0.000", unit: "deg" },
        { name: "RY", value: "0.000", unit: "deg" },
        { name: "RZ", value: "0.000", unit: "deg" }
    ]

    property var robot1IoModel: [
        { text: "툴 체결",   log: "Robot1 ToolChanger Lock" },
        { text: "툴 해제",   log: "Robot1 ToolChanger Release" },
        { text: "벌크 흡착", log: "Robot1 Bulk Magnet Attach" },
        { text: "벌크 해제", log: "Robot1 Bulk Magnet Release" },
        { text: "소팅 흡착", log: "Robot1 Sorting Magnet Attach" },
        { text: "소팅 해제", log: "Robot1 Sorting Magnet Release" }
    ]

    property var robot2IoModel: [
        { text: "마그넷 흡착", log: "Robot2 Magnet Attach" },
        { text: "마그넷 해제", log: "Robot2 Magnet Release" },
        { text: "클램프1 체결", log: "Robot2 Clamp1 Lock" },
        { text: "클램프1 해제", log: "Robot2 Clamp1 Release" },
        { text: "클램프2 체결", log: "Robot2 Clamp2 Lock" },
        { text: "클램프2 해제", log: "Robot2 Clamp2 Release" }
    ]

    property var toolChangeModel: [
        { text: "벌크툴 장착",       log: "Mount Bulk Tool" },
        { text: "벌크툴 탈거",       log: "Unmount Bulk Tool" },
        { text: "소팅툴 장착",       log: "Mount Sorting Tool" },
        { text: "소팅툴 탈거",       log: "Unmount Sorting Tool" },
        { text: "벌크 → 소팅 교체",  log: "Change Bulk Tool to Sorting Tool" },
        { text: "소팅 → 벌크 교체",  log: "Change Sorting Tool to Bulk Tool" }
    ]

    property var processUnitActionModel: [
        { text: "벌크 픽",          log: "Process Bulk Pick" },
        { text: "벌크 플레이스",    log: "Process Bulk Place" },
        { text: "소팅 F 픽",        log: "Process Sorting Front Pick" },
        { text: "소팅 F 플레이스",  log: "Process Sorting Front Place" },
        { text: "소팅 R 픽",        log: "Process Sorting Rear Pick" },
        { text: "소팅 R 플레이스",  log: "Process Sorting Rear Place" },
        { text: "조립 픽",          log: "Process Assembly Pick" },
        { text: "조립 플레이스",    log: "Process Assembly Place" },
        { text: "컨베어 이동",      log: "Conveyor Move" }
    ]

    property var referencePointModel: [
        { text: "P1 확인", point: "P1" },
        { text: "P2 확인", point: "P2" },
        { text: "P3 확인", point: "P3" },
        { text: "P4 확인", point: "P4" }
    ]

    property var gantryAxisModel: [
        { axis: "X축", pos: "0.000", unit: "mm" },
        { axis: "Z축", pos: "0.000", unit: "mm" },
        { axis: "피커 회전 (R)", pos: "0.000", unit: "deg" }
    ]

    // ============================================================
    // Local reusable UI blocks
    // ============================================================
    component SectionLabel: Text {
        color: "#2563eb"
        font.pixelSize: 12
        font.bold: true
        font.family: "Asta Sans"
    }

    component PlainButton: Button {
        id: buttonRoot

        Layout.fillWidth: true
        Layout.minimumWidth: 0
        Layout.preferredWidth: 1
        Layout.preferredHeight: 28

        font.pixelSize: 13

        contentItem: Text {
            text: buttonRoot.text
            color: "#374151"
            font.pixelSize: buttonRoot.font.pixelSize
            font.family: "Asta Sans"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        background: Rectangle {
            radius: 4
            color: buttonRoot.down ? "#cbd5e1" : "#e5e7eb"
            border.color: "#d1d5db"
        }
    }

    component ProcessActionButton: Button {
        id: processButton

        property bool danger: false

        Layout.fillWidth: true
        Layout.minimumWidth: 0
        Layout.preferredWidth: 1
        Layout.preferredHeight: 28

        contentItem: Text {
            text: processButton.text
            color: processButton.danger ? "#dc2626" : "#374151"
            font.pixelSize: 13
            font.bold: processButton.danger
            font.family: "Asta Sans"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        background: Rectangle {
            radius: 4
            color: processButton.down
                   ? (processButton.danger ? "#fecaca" : "#cbd5e1")
                   : (processButton.danger ? "#fee2e2" : "#e5e7eb")
            border.color: processButton.danger ? "#fca5a5" : "#d1d5db"
        }
    }

    component ModeSelectButton: Button {
        id: modeSelectButton
        property bool selected: false

        Layout.fillWidth: true
        Layout.preferredHeight: 28

        contentItem: Text {
            text: modeSelectButton.text
            color: modeSelectButton.selected ? "white" : "#334155"
            font.pixelSize: 13
            font.bold: modeSelectButton.selected
            font.family: "Asta Sans"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            radius: 3
            color: modeSelectButton.selected ? "#1976d2" : "#e5e7eb"
            border.color: modeSelectButton.selected ? "#1565c0" : "#d1d5db"
        }
    }

    component ModeButton: Button {
        id: modeRoot
        property bool selected: false

        Layout.fillWidth: true
        Layout.preferredHeight: 40

        contentItem: Text {
            text: modeRoot.text
            color: modeRoot.selected ? "white" : "#111827"
            font.pixelSize: 14
            font.bold: true
            font.family: "Asta Sans"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            radius: 6
            color: modeRoot.selected ? "#2563eb" : "#f8fafc"
            border.color: modeRoot.selected ? "#1d4ed8" : "#d1d5db"
        }
    }

    component RobotJogPanel: Rectangle {
        id: jogPanel

        property int robotId: 1
        property string jogMode: "joint"
        property var jogModel: []

        property int activeJogJoint: 0
        property bool activeJogPositive: true
        property int jogHeartbeatIntervalMs: 150

        function startJointJogWithHeartbeat(joint, positive) {
            if (joint <= 0)
                return

            activeJogJoint = joint
            activeJogPositive = positive

            iotViewModel.startRobotJointJog(robotId, joint, positive)
            jogHeartbeatTimer.restart()
        }

        function stopJointJogWithHeartbeat() {
            jogHeartbeatTimer.stop()

            if (activeJogJoint > 0) {
                iotViewModel.stopRobotJointJog(robotId)
            }

            activeJogJoint = 0
        }

        Timer {
            id: jogHeartbeatTimer
            interval: jogPanel.jogHeartbeatIntervalMs
            repeat: true
            running: false

            onTriggered: {
                if (jogPanel.activeJogJoint > 0) {
                    iotViewModel.sendRobotJogHeartbeat(jogPanel.robotId)
                } else {
                    stop()
                }
            }
        }

        Layout.fillWidth: true
        Layout.preferredHeight: 44 + 28 + 4 + (jogModel.length * 28) + ((jogModel.length - 1) * 4)

        radius: 6
        color: "#f8fafc"
        border.color: "#e2e8f0"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 4

            RowLayout {
                Layout.fillWidth: true
                spacing: 4

                ModeSelectButton {
                    text: "조인트"
                    selected: jogPanel.jogMode === "joint"

                    onClicked: {
                        if (jogPanel.robotId === 1) {
                            root.robot1JogMode = "joint"
                            console.log("[QML] Robot1 jog mode: Joint")
                        } else {
                            root.robot2JogMode = "joint"
                            console.log("[QML] Robot2 jog mode: Joint")
                        }
                    }
                }

                ModeSelectButton {
                    text: "베이스"
                    selected: jogPanel.jogMode === "base"

                    onClicked: {
                        if (jogPanel.robotId === 1) {
                            root.robot1JogMode = "base"
                            console.log("[QML] Robot1 jog mode: Base")
                        } else {
                            root.robot2JogMode = "base"
                            console.log("[QML] Robot2 jog mode: Base")
                        }
                    }
                }
            }

            Repeater {
                model: jogPanel.jogModel

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Text {
                        text: modelData.name
                        Layout.preferredWidth: 28
                        font.pixelSize: 13
                        font.bold: true
                        font.family: "Asta Sans"
                    }

                    PlainButton {
                        text: "−"
                        Layout.preferredWidth: 46
                        Layout.fillWidth: false

                        onPressed: {
                            console.log("[QML] Robot" + jogPanel.robotId + " jog minus:", modelData.name, jogPanel.jogMode)

                            if (jogPanel.jogMode === "joint") {
                                var joint = root.jointIndexFromName(modelData.name)
                                if (joint > 0)
                                    jogPanel.startJointJogWithHeartbeat(joint, false)
                            }
                        }
                        onReleased: {
                            console.log("[QML] Robot" + jogPanel.robotId + " jog stop:", modelData.name)

                            if (jogPanel.jogMode === "joint") {
                                jogPanel.stopJointJogWithHeartbeat()
                            }
                        }
                        onCanceled: {
                            console.log("[QML] Robot" + jogPanel.robotId + " jog canceled:", modelData.name)

                            if (jogPanel.jogMode === "joint") {
                                jogPanel.stopJointJogWithHeartbeat()
                            }
                        }
                    }

                    TextField {
                        text: modelData.value
                        readOnly: true
                        color: "#374151"
                        horizontalAlignment: Text.AlignRight
                        font.family: "Asta Sans"
                        font.pixelSize: 13
                        Layout.fillWidth: true
                        Layout.preferredHeight: 28
                        background: Rectangle {
                            radius: 4
                            color: "#f8fafc"
                            border.color: "#d1d5db"
                            border.width: 1
                        }
                    }

                    Text {
                        text: modelData.unit
                        Layout.preferredWidth: 36
                        color: "#334155"
                        font.pixelSize: 13
                        font.family: "Asta Sans"
                    }

                    PlainButton {
                        text: "+"
                        Layout.preferredWidth: 46
                        Layout.fillWidth: false

                        onPressed: {
                            console.log("[QML] Robot" + jogPanel.robotId + " jog plus:", modelData.name, jogPanel.jogMode)

                            if (jogPanel.jogMode === "joint") {
                                var joint = root.jointIndexFromName(modelData.name)
                                if (joint > 0)
                                    jogPanel.startJointJogWithHeartbeat(joint, true)
                            }
                        }
                        onReleased: {
                            console.log("[QML] Robot" + jogPanel.robotId + " jog stop:", modelData.name)

                            if (jogPanel.jogMode === "joint") {
                                jogPanel.stopJointJogWithHeartbeat()
                            }
                        }
                        onCanceled: {
                            console.log("[QML] Robot" + jogPanel.robotId + " jog canceled:", modelData.name)

                            if (jogPanel.jogMode === "joint") {
                                jogPanel.stopJointJogWithHeartbeat()
                            }
                        }
                    }
                }
            }
        }
    }

    component JogAxisBox: Rectangle {
        id: axisBox

        property string axisName: "X축"
        property string axisValue: "0.000"
        property string unitText: "mm"

        Layout.fillWidth: true
        Layout.preferredHeight: 88

        radius: 6
        color: "#f8fafc"
        border.color: "#e2e8f0"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 6
            spacing: 4

            Text {
                text: axisBox.axisName
                Layout.fillWidth: true
                Layout.preferredHeight: 16
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                color: "#111827"
                font.pixelSize: 13
                font.bold: true
                font.family: "Asta Sans"
            }

            TextField {
                text: axisBox.axisValue + " " + axisBox.unitText
                readOnly: true
                color: "#374151"
                Layout.fillWidth: true
                Layout.preferredHeight: 26
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 13
                font.family: "Asta Sans"
                background: Rectangle {
                    radius: 4
                    color: "#f8fafc"
                    border.color: "#d1d5db"
                    border.width: 1
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                PlainButton {
                    text: "−"
                    onPressed: console.log("[QML] Gantry jog negative:", axisBox.axisName)
                    onReleased: console.log("[QML] Gantry jog stop:", axisBox.axisName)
                }

                PlainButton {
                    text: "+"
                    onPressed: console.log("[QML] Gantry jog positive:", axisBox.axisName)
                    onReleased: console.log("[QML] Gantry jog stop:", axisBox.axisName)
                }
            }
        }
    }

    component RobotColumn: Rectangle {
        id: robotColumn

        property int robotId: 1
        property string titleText: "로봇 1 (FR10)"
        property string jogMode: "joint"
        property var jogModel: []
        property var ioModel: []

        Layout.fillWidth: true
        Layout.preferredWidth: 1.0
        implicitHeight: _robotColLayout.implicitHeight + 20

        radius: 8
        color: "white"
        border.color: "#d7dde6"

        ColumnLayout {
            id: _robotColLayout
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 10
            spacing: 6

            Text {
                text: robotColumn.titleText
                color: "#2563eb"
                font.pixelSize: 16
                font.bold: true
                font.family: "Asta Sans"
            }

            DeviceControlStatusBox {
                connectionText: "연결됨"
                servoText: "ON"
                estopText: "0"
                errorText: "0"
                connectButtonText: "통신 해제"
                alarmResetButtonText: "알람 리셋"

                showConnectButton: true
                showServoButton: false
                showAlarmResetButton: true

                onConnectClicked: console.log("[QML] Robot" + robotColumn.robotId + " communication toggle")
                onAlarmResetClicked: console.log("[QML] Robot" + robotColumn.robotId + " alarm reset")
            }
            // 섹션1: 초기화 / 대기
            SectionLabel { text: "초기화 / 대기" }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                radius: 6
                color: "#f8fafc"
                border.color: "#e2e8f0"

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 6

                    PlainButton {
                        text: "초기화 위치 이동"
                        onClicked: console.log("[QML] Robot" + robotColumn.robotId + " Initial Pose")
                    }

                    PlainButton {
                        text: "대기 위치 이동"
                        onClicked: console.log("[QML] Robot" + robotColumn.robotId + " Standby Pose")
                    }
                }
            }
            // 섹션2: 기준점 확인
            SectionLabel { text: "기준점 확인" }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 80
                radius: 6
                color: "#f8fafc"
                border.color: "#e2e8f0"

                GridLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    columns: 2
                    rowSpacing: 4
                    columnSpacing: 6

                    Repeater {
                        model: root.referencePointModel

                        PlainButton {
                            text: modelData.text
                            onClicked: console.log("[QML] Robot" + robotColumn.robotId + " Reference Point Check:", modelData.point)
                        }
                    }
                }
            }
            // 섹션3: 조그
            SectionLabel { text: "조그" }

            RobotJogPanel {
                robotId: robotColumn.robotId
                jogMode: robotColumn.jogMode
                jogModel: robotColumn.jogModel
            }
            // 섹션4: 로봇 I/O
            SectionLabel { text: "로봇 I/O" }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 112
                radius: 6
                color: "#f8fafc"
                border.color: "#e2e8f0"

                GridLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    columns: 2
                    rowSpacing: 4
                    columnSpacing: 6

                    Repeater {
                        model: robotColumn.ioModel

                        PlainButton {
                            text: modelData.text
                            onClicked: console.log("[QML]", modelData.log)
                        }
                    }
                }
            } //
        }
    }

    RowLayout {
        id: _contentRow
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        // --------------------------------------------------------
        // Column 1: 로봇 1
        // --------------------------------------------------------
        ScrollView {
            id: _scroll1
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentHeight: _col1.implicitHeight
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            RobotColumn {
                id: _col1
                width: _scroll1.availableWidth
                robotId: 1
                titleText: "로봇 1 (FR10)"
                jogMode: root.robot1JogMode
                jogModel: root.robot1JogMode === "joint" ? root.robot1JointModel : root.robot1BaseModel
                ioModel: root.robot1IoModel
            }   // end of Column 1
        }   // end of ScrollView 1

        // --------------------------------------------------------
        // Column 2: 로봇 2
        // --------------------------------------------------------
        ScrollView {
            id: _scroll2
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentHeight: _col2.implicitHeight
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            RobotColumn {
                id: _col2
                width: _scroll2.availableWidth
                robotId: 2
                titleText: "로봇 2 (FR5)"
                jogMode: root.robot2JogMode
                jogModel: root.robot2JogMode === "joint" ? root.robot2JointModel : root.robot2BaseModel
                ioModel: root.robot2IoModel
            }   // end of Column 2
        }   // end of ScrollView 2

        // --------------------------------------------------------
        // Column 3: 전체 제어 / 정상확인
        // --------------------------------------------------------
        ScrollView {
            id: _scroll3
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentHeight: _col3Content.implicitHeight
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            Rectangle {
                id: _col3Content
                width: _scroll3.availableWidth
                implicitHeight: _col3Layout.implicitHeight + 20

                radius: 8
                color: "white"
                border.color: "#d7dde6"

                ColumnLayout {
                    id: _col3Layout
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 10
                    spacing: 6
                    // 섹션1: 전체 제어 / 정상확인
                    Text {
                        text: "전체 제어 / 정상확인"
                        color: "#2563eb"
                        font.pixelSize: 16
                        font.bold: true
                        font.family: "Asta Sans"
                    }   // end of section1
                    // 섹션2: 로봇 제어 모드
                    SectionLabel { text: "로봇 제어 모드" }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 56
                        radius: 6
                        color: "#f8fafc"
                        border.color: "#e2e8f0"

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 6
                            spacing: 6

                            ModeButton {
                                text: "수동 모드"
                                selected: root.robotManualMode
                                onClicked: {
                                    root.robotManualMode = true
                                    console.log("[QML] Robot manual mode")

                                    iotViewModel.setRobotManualMode(1)
                                    iotViewModel.setRobotManualMode(2)
                                }
                            }

                            ModeButton {
                                text: "자동 모드"
                                selected: !root.robotManualMode
                                onClicked: {
                                    root.robotManualMode = false
                                    console.log("[QML] Robot auto mode")

                                    iotViewModel.setRobotAutoMode(1)
                                    iotViewModel.setRobotAutoMode(2)
                                }
                            }
                        }
                    }   // end of section2
                    // 섹션3: 복구 / 초기화
                    SectionLabel { text: "복구 / 초기화" }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 80
                        radius: 6
                        color: "#f8fafc"
                        border.color: "#e2e8f0"

                        GridLayout {
                            anchors.fill: parent
                            anchors.margins: 6
                            columns: 2
                            rowSpacing: 4
                            columnSpacing: 6

                            PlainButton {
                                text: "전체 에러 해제"
                                Layout.columnSpan: 2
                                onClicked: {
                                    console.log("[QML] Clear all errors")
                                    iotViewModel.clearRobotError(1)
                                    iotViewModel.clearRobotError(2)
                                }

                            }

                            PlainButton {
                                text: "전체 초기화 위치 이동"
                                onClicked: console.log("[QML] Move all initial position")
                            }

                            PlainButton {
                                text: "전체 대기 위치 이동"
                                onClicked: console.log("[QML] Move all standby position")
                            }
                        }
                    }   // end of section3
                    // 섹션4: 로봇 동작 확인
                    SectionLabel { text: "로봇 동작 확인" }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 46
                        radius: 6
                        color: "#f8fafc"
                        border.color: "#e2e8f0"

                        GridLayout {
                            anchors.fill: parent
                            anchors.margins: 6
                            columns: 2
                            rowSpacing: 4
                            columnSpacing: 6

                            PlainButton { text: "로봇 1 확인"; onClicked: console.log("[QML] Check Robot1") }
                            PlainButton { text: "로봇 2 확인"; onClicked: console.log("[QML] Check Robot2") }
                        }
                    }   // end of section4
                    // 섹션5: 공정 단위 동작
                    SectionLabel { text: "공정 단위 동작" }

                    Rectangle {
                        property int buttonCount: root.processUnitActionModel.length
                        property int columnCount: 2
                        property int rowCount: Math.ceil(buttonCount / columnCount)

                        Layout.fillWidth: true
                        Layout.preferredHeight: 46 + Math.max(0, rowCount - 1) * 34
                        radius: 6
                        color: "#f8fafc"
                        border.color: "#e2e8f0"

                        GridLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            columns: 2
                            rowSpacing: 6
                            columnSpacing: 6

                            Repeater {
                                model: root.processUnitActionModel

                                ProcessActionButton {
                                    text: modelData.text
                                    danger: modelData.danger === true
                                    onClicked: console.log("[QML]", modelData.log)
                                }
                            }
                        }
                    }   // end of section5
                    // 섹션6: 툴 교체
                    SectionLabel { text: "툴 교체" }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 112
                        radius: 6
                        color: "#f8fafc"
                        border.color: "#e2e8f0"

                        GridLayout {
                            anchors.fill: parent
                            anchors.margins: 6
                            columns: 2
                            rowSpacing: 4
                            columnSpacing: 6

                            Repeater {
                                model: root.toolChangeModel

                                PlainButton {
                                    text: modelData.text
                                    onClicked: console.log("[QML]", modelData.log)
                                }
                            }
                        }
                    }   // end of section6
                    // 섹션7: 최근 명령 결과
                    SectionLabel { text: "최근 명령 결과" }
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 58
                        radius: 6
                        color: {
                            if (!iotViewModel.lastCommandResult ||
                                iotViewModel.lastCommandResult.command === undefined)
                                return "#f8fafc"

                            return iotViewModel.lastCommandResult.ok ? "#ecfdf5" : "#fef2f2"
                        }
                        border.color: {
                            if (!iotViewModel.lastCommandResult ||
                                iotViewModel.lastCommandResult.command === undefined)
                                return "#e2e8f0"

                            return iotViewModel.lastCommandResult.ok ? "#86efac" : "#fca5a5"
                        }

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 2

                            Text {
                                Layout.fillWidth: true
                                text: "최근 명령 결과"
                                color: "#334155"
                                font.pixelSize: 12
                                font.bold: true
                                font.family: "Asta Sans"
                            }

                            Text {
                                Layout.fillWidth: true
                                text: {
                                    var r = iotViewModel.lastCommandResult

                                    if (!r || r.command === undefined)
                                        return "명령 이력 없음"

                                    var state = r.ok ? "OK" : "FAIL"
                                    return "Robot " + r.robotId
                                           + " / " + r.command
                                           + " / " + state
                                           + " / code=" + r.code
                                           + " / " + r.message
                                }
                                color: {
                                    var r = iotViewModel.lastCommandResult

                                    if (!r || r.command === undefined)
                                        return "#64748b"

                                    return r.ok ? "#047857" : "#dc2626"
                                }
                                font.pixelSize: 12
                                font.family: "Asta Sans"
                                elide: Text.ElideRight
                            }
                        }
                    }   // end of section7
                }   // end of column layout
            }   // end of column 3
        }   // end of scroll 3

        // --------------------------------------------------------
        // Column 4: 갠트리/피커
        // --------------------------------------------------------
        ScrollView {
            id: _scroll4
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentHeight: _col4Content.implicitHeight
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            Rectangle {
                id: _col4Content
                width: _scroll4.availableWidth
                implicitHeight: _col4Layout.implicitHeight + 20

                radius: 8
                color: "white"
                border.color: "#d7dde6"

                ColumnLayout {
                    id: _col4Layout
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 10
                    spacing: 6

                    Text {
                        text: "갠트리/피커 (X/Z/R)"
                        color: "#2563eb"
                        font.pixelSize: 16
                        font.bold: true
                        font.family: "Asta Sans"
                    }

                    DeviceControlStatusBox {
                        connectionText: "연결됨"
                        servoText: "ON"
                        estopText: "0"
                        errorText: "0"
                        connectButtonText: "통신 해제"
                        servoButtonText: "서보 OFF"
                        alarmResetButtonText: "알람 리셋"

                        showConnectButton: true
                        showServoButton: true
                        showAlarmResetButton: true

                        onConnectClicked: console.log("[QML] Gantry communication toggle")
                        onServoClicked: console.log("[QML] Gantry servo toggle")
                        onAlarmResetClicked: console.log("[QML] Gantry alarm reset")
                    }

                    SectionLabel { text: "원점 / 초기화" }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 80
                        radius: 6
                        color: "#f8fafc"
                        border.color: "#e2e8f0"

                        GridLayout {
                            anchors.fill: parent
                            anchors.margins: 6
                            columns: 2
                            rowSpacing: 4
                            columnSpacing: 6

                            PlainButton { text: "X축 원점"; onClicked: console.log("[QML] Home X") }
                            PlainButton { text: "Z축 원점"; onClicked: console.log("[QML] Home Z") }
                            PlainButton { text: "R축 원점"; onClicked: console.log("[QML] Home R") }
                            PlainButton { text: "전체 원점"; onClicked: console.log("[QML] Home all") }
                        }
                    }

                    SectionLabel { text: "조그" }

                    Repeater {
                        model: root.gantryAxisModel

                        JogAxisBox {
                            axisName: modelData.axis
                            axisValue: modelData.pos
                            unitText: modelData.unit
                        }
                    }

                    SectionLabel { text: "갠트리/피커 I/O" }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 50
                        radius: 6
                        color: "#f8fafc"
                        border.color: "#e2e8f0"

                        GridLayout {
                            anchors.fill: parent
                            anchors.margins: 6
                            columns: 2
                            rowSpacing: 4
                            columnSpacing: 6

                            PlainButton {
                                text: "피커 흡착"
                                onClicked: console.log("[QML] Picker attach")
                            }

                            PlainButton {
                                text: "피커 해제"
                                onClicked: console.log("[QML] Picker release")
                            }
                        }
                    }
                }
            }
        }
    }
}
