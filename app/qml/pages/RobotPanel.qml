import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "qrc:/qml/components"

Item {
    id: root
    anchors.fill: parent

    // Robot jog display/control mode
    // "joint" or "base"
    property string robot1JogMode: "joint"
    property string robot2JogMode: "joint"

    property bool robotManualMode: true

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
        font.pixelSize: 13
        font.bold: true
    }

    component PlainButton: Button {
        id: buttonRoot

        Layout.fillWidth: true
        Layout.minimumWidth: 0
        Layout.preferredWidth: 1
        Layout.preferredHeight: 34

        font.pixelSize: 14

        contentItem: Text {
            text: buttonRoot.text
            color: "#374151"
            font.pixelSize: buttonRoot.font.pixelSize
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

    component DangerButton: Button {
        id: dangerRoot
        Layout.fillWidth: true
        Layout.preferredHeight: 34

        contentItem: Text {
            text: dangerRoot.text
            color: "#dc2626"
            font.pixelSize: 14
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            radius: 4
            color: dangerRoot.down ? "#fecaca" : "#fee2e2"
            border.color: "#fca5a5"
        }
    }


    component ProcessActionButton: Button {
        id: processButton

        property bool danger: false

        Layout.fillWidth: true
        Layout.minimumWidth: 0
        Layout.preferredWidth: 1
        Layout.preferredHeight: 34

        contentItem: Text {
            text: processButton.text
            color: processButton.danger ? "#dc2626" : "#374151"
            font.pixelSize: 14
            font.bold: processButton.danger
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
        Layout.preferredHeight: 34

        contentItem: Text {
            text: modeSelectButton.text
            color: modeSelectButton.selected ? "white" : "#334155"
            font.pixelSize: 14
            font.bold: modeSelectButton.selected
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            radius: 3
            color: modeSelectButton.selected ? "#2f2f2f" : "#e5e7eb"
            border.color: modeSelectButton.selected ? "#2f2f2f" : "#d1d5db"
        }
    }

    component ModeButton: Button {
        id: modeRoot
        property bool selected: false

        Layout.fillWidth: true
        Layout.preferredHeight: 48

        contentItem: Text {
            text: modeRoot.text
            color: modeRoot.selected ? "white" : "#111827"
            font.pixelSize: 16
            font.bold: true
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

        Layout.fillWidth: true
        Layout.preferredHeight: 64 + 34 + 6 + (jogModel.length * 34) + ((jogModel.length - 1) * 6) //300

        radius: 6
        color: "#f8fafc"
        border.color: "#e2e8f0"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 6

            RowLayout {
                Layout.fillWidth: true
                spacing: 6

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
                        font.pixelSize: 14
                        font.bold: true
                    }

                    PlainButton {
                        text: "−"
                        Layout.preferredWidth: 46
                        Layout.fillWidth: false

                        onPressed: console.log("[QML] Robot" + jogPanel.robotId + " jog minus:", modelData.name, jogPanel.jogMode)
                        onReleased: console.log("[QML] Robot" + jogPanel.robotId + " jog stop:", modelData.name)
                    }

                    TextField {
                        text: modelData.value
                        readOnly: true
                        horizontalAlignment: Text.AlignRight
                        Layout.fillWidth: true
                        Layout.preferredHeight: 34
                    }

                    Text {
                        text: modelData.unit
                        Layout.preferredWidth: 36
                        color: "#334155"
                    }

                    PlainButton {
                        text: "+"
                        Layout.preferredWidth: 46
                        Layout.fillWidth: false

                        onPressed: console.log("[QML] Robot" + jogPanel.robotId + " jog plus:", modelData.name, jogPanel.jogMode)
                        onReleased: console.log("[QML] Robot" + jogPanel.robotId + " jog stop:", modelData.name)
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
        Layout.preferredHeight: 106

        radius: 6
        color: "#f8fafc"
        border.color: "#e2e8f0"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 5

            Text {
                text: axisBox.axisName
                Layout.fillWidth: true
                Layout.preferredHeight: 18
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                color: "#111827"
                font.pixelSize: 14
                font.bold: true
            }

            TextField {
                text: axisBox.axisValue + " " + axisBox.unitText
                readOnly: true
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 14
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 28
                spacing: 6

                PlainButton {
                    text: "−"
                    Layout.preferredHeight: 28
                    onPressed: console.log("[QML] Gantry jog negative:", axisBox.axisName)
                    onReleased: console.log("[QML] Gantry jog stop:", axisBox.axisName)
                }

                PlainButton {
                    text: "+"
                    Layout.preferredHeight: 28
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
        Layout.fillHeight: true
        Layout.preferredWidth: 1.0

        radius: 8
        color: "white"
        border.color: "#d7dde6"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8

            Text {
                text: robotColumn.titleText
                color: "#2563eb"
                font.pixelSize: 18
                font.bold: true
            }

            DeviceControlStatusBox {
                connectionText: "연결됨"
                servoText: "ON"
                estopText: "0"
                errorText: "0"
                connectButtonText: "통신 해제"
//                servoButtonText: "서보 OFF"
                alarmResetButtonText: "알람 리셋"

                showConnectButton: true
                showServoButton: false
                showAlarmResetButton: true

                onConnectClicked: console.log("[QML] Robot" + robotColumn.robotId + " communication toggle")
//                onServoClicked: console.log("[QML] Robot" + robotColumn.robotId + " servo toggle")
                onAlarmResetClicked: console.log("[QML] Robot" + robotColumn.robotId + " alarm reset")
            }

            SectionLabel { text: "초기화 / 대기" }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 62
                radius: 6
                color: "#f8fafc"
                border.color: "#e2e8f0"

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 8

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

            SectionLabel { text: "기준점 확인" }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 108
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
                        model: root.referencePointModel

                        PlainButton {
                            text: modelData.text
                            onClicked: console.log("[QML] Robot" + robotColumn.robotId + " Reference Point Check:", modelData.point)
                        }
                    }
                }
            }

            SectionLabel { text: "조그" }

            RobotJogPanel {
                robotId: robotColumn.robotId
                jogMode: robotColumn.jogMode
                jogModel: robotColumn.jogModel
            }

            SectionLabel { text: "로봇 I/O" }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 144
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
                        model: robotColumn.ioModel

                        PlainButton {
                            text: modelData.text
                            onClicked: console.log("[QML]", modelData.log)
                        }
                    }
                }
            }

            Item { Layout.fillHeight: true }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        // ============================================================
        // Upper Area : 4 Columns
        // 로봇 1 | 로봇 2 | 정상확인/초기화 | 갠트리/피커
        // ============================================================
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            RobotColumn {
                robotId: 1
                titleText: "로봇 1 (FR10)"
                jogMode: root.robot1JogMode
                jogModel: root.robot1JogMode === "joint" ? root.robot1JointModel : root.robot1BaseModel
                ioModel: root.robot1IoModel
            }

            RobotColumn {
                robotId: 2
                titleText: "로봇 2 (FR5)"
                jogMode: root.robot2JogMode
                jogModel: root.robot2JogMode === "joint" ? root.robot2JointModel : root.robot2BaseModel
                ioModel: root.robot2IoModel
            }

            // --------------------------------------------------------
            // Column 3: 전체 제어 / 정상확인
            // --------------------------------------------------------
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 1.25

                radius: 8
                color: "white"
                border.color: "#d7dde6"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 8

                    Text {
                        text: "전체 제어 / 정상확인"
                        color: "#2563eb"
                        font.pixelSize: 18
                        font.bold: true
                    }

                    SectionLabel { text: "로봇 제어 모드" }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 70
                        radius: 6
                        color: "#f8fafc"
                        border.color: "#e2e8f0"

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 8

                            ModeButton {
                                text: "수동 모드"
                                selected: root.robotManualMode
                                onClicked: {
                                    root.robotManualMode = true
                                    console.log("[QML] Robot manual mode")
                                }
                            }

                            ModeButton {
                                text: "자동 모드"
                                selected: !root.robotManualMode
                                onClicked: {
                                    root.robotManualMode = false
                                    console.log("[QML] Robot auto mode")
                                }
                            }
                        }
                    }

                    SectionLabel { text: "복구 / 초기화" }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 108
                        radius: 6
                        color: "#f8fafc"
                        border.color: "#e2e8f0"

                        GridLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            columns: 2
                            rowSpacing: 6
                            columnSpacing: 6

                            PlainButton {
                                text: "전체 에러 해제"
                                Layout.columnSpan: 2
                                onClicked: console.log("[QML] Clear all errors")
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
                    }

                    SectionLabel { text: "로봇 동작 확인" }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 62
                        radius: 6
                        color: "#f8fafc"
                        border.color: "#e2e8f0"

                        GridLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            columns: 2
                            rowSpacing: 6
                            columnSpacing: 6

                            PlainButton { text: "로봇 1 확인"; onClicked: console.log("[QML] Check Robot1") }
                            PlainButton { text: "로봇 2 확인"; onClicked: console.log("[QML] Check Robot2") }
                        }
                    }

                    SectionLabel { text: "공정 단위 동작" }

                    Rectangle {
                        property int buttonCount: root.processUnitActionModel.length
                        property int columnCount: 2
                        property int rowCount: Math.ceil(buttonCount / columnCount)

                        Layout.fillWidth: true
                        Layout.preferredHeight: 62 + Math.max(0, rowCount - 1) * 46
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
                    }

                    SectionLabel { text: "툴 교체" }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 144
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
                                model: root.toolChangeModel

                                PlainButton {
                                    text: modelData.text
                                    onClicked: console.log("[QML]", modelData.log)
                                }
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }
            }

            // --------------------------------------------------------
            // Column 4: 갠트리/피커
            // --------------------------------------------------------
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 0.95

                radius: 8
                color: "white"
                border.color: "#d7dde6"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 8

                    Text {
                        text: "갠트리/피커 (X/Z/R)"
                        color: "#2563eb"
                        font.pixelSize: 18
                        font.bold: true
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
                        Layout.preferredHeight: 108
                        radius: 6
                        color: "#f8fafc"
                        border.color: "#e2e8f0"

                        GridLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            columns: 2
                            rowSpacing: 6
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
                        Layout.preferredHeight: 64
                        radius: 6
                        color: "#f8fafc"
                        border.color: "#e2e8f0"

                        GridLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            columns: 2
                            rowSpacing: 6
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

                    Item { Layout.fillHeight: true }
                }
            }
        }

        // ============================================================
        // Bottom Command Log
        // ============================================================
        /*
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 150
            radius: 8
            color: "white"
            border.color: "#d7dde6"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 6

                RowLayout {
                    Layout.fillWidth: true

                    Text {
                        text: "명령 로그"
                        color: "#2563eb"
                        font.pixelSize: 18
                        font.bold: true
                    }

                    Item { Layout.fillWidth: true }

                    PlainButton {
                        text: "로그 지우기"
                        Layout.preferredWidth: 110
                        Layout.fillWidth: false
                        onClicked: console.log("[QML] Clear log")
                    }
                }

                TextArea {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    readOnly: true
                    text: "[17:24:10] [INFO] RobotPanel.qml loaded\n[17:24:11] [INFO] Waiting for ViewModel connection...\n[17:24:20] [INFO] Robot 1 Connected\n[17:24:20] [INFO] Robot 2 Connected\n[17:24:20] [INFO] Gantry Connected"
                }
            }
        }*/
    }
}
