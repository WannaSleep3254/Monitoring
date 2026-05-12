import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root

    // ----------------------------
    // Status text
    // ----------------------------
    property string connectionText: "연결됨"
    property string servoText: "ON"
    property string estopText: "0"
    property string errorText: "0"

    // ----------------------------
    // Control button text
    // ----------------------------
    property string connectButtonText: "통신 해제"
    property string servoButtonText: "서보 OFF"
    property string alarmResetButtonText: "알람 리셋"

    // ----------------------------
    // Colors
    // ----------------------------
    property color normalColor: "#16a34a"
    property color warningColor: "#dc2626"
    property color labelColor: "#111827"
    property color backgroundColor: "#f8fafc"
    property color borderColor: "#e2e8f0"

    // ----------------------------
    // Signals
    // RobotPanel.qml에서 onConnectClicked 등으로 받을 수 있음
    // ----------------------------
    signal connectClicked()
    signal servoClicked()
    signal alarmResetClicked()

    Layout.fillWidth: true
    Layout.preferredHeight: 138

    radius: 6
    color: backgroundColor
    border.color: borderColor

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 6

        GridLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 76

            columns: 2
            rowSpacing: 3
            columnSpacing: 10

            Text {
                text: "연결"
                color: root.labelColor
                font.pixelSize: 14
            }

            Text {
                text: root.connectionText
                color: root.connectionText === "연결됨" ? root.normalColor : root.warningColor
                font.pixelSize: 14
            }

            Text {
                text: "서보"
                color: root.labelColor
                font.pixelSize: 14
            }

            Text {
                text: root.servoText
                color: root.servoText === "ON" ? root.normalColor : root.warningColor
                font.pixelSize: 14
            }

            Text {
                text: "비상정지"
                color: root.labelColor
                font.pixelSize: 14
            }

            Text {
                text: root.estopText
                color: root.estopText === "0" ? root.normalColor : root.warningColor
                font.pixelSize: 14
            }

            Text {
                text: "에러"
                color: root.labelColor
                font.pixelSize: 14
            }

            Text {
                text: root.errorText
                color: root.errorText === "0" ? root.normalColor : root.warningColor
                font.pixelSize: 14
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            spacing: 6

            Button {
                id: buttonConnect
                text: root.connectButtonText
                Layout.fillWidth: true
                Layout.preferredHeight: 32

                font.pixelSize: 12

                onClicked: root.connectClicked()
            }

            Button {
                id: buttonServo
                text: root.servoButtonText
                Layout.fillWidth: true
                Layout.preferredHeight: 32

                font.pixelSize: 12

                onClicked: root.servoClicked()
            }

            Button {
                id: buttonAlarmReset
                text: root.alarmResetButtonText
                Layout.fillWidth: true
                Layout.preferredHeight: 32

                font.pixelSize: 12

                onClicked: root.alarmResetClicked()
            }
        }
    }
}
