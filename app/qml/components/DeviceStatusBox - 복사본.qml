import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root

    property string connectionText: "연결됨"
    property string servoText: "ON"
    property string estopText: "0"
    property string errorText: "0"

    property color normalColor: "#16a34a"
    property color warningColor: "#dc2626"

    Layout.fillWidth: true
    Layout.preferredHeight: 96

    radius: 6
    color: "#f8fafc"
    border.color: "#e2e8f0"

    GridLayout {
        anchors.fill: parent
        anchors.margins: 8
        columns: 2
        rowSpacing: 4
        columnSpacing: 10

        Text {
            text: "연결"
            color: "#111827"
            font.pixelSize: 14
        }

        Text {
            text: root.connectionText
            color: root.connectionText === "연결됨" ? root.normalColor : root.warningColor
            font.pixelSize: 14
        }

        Text {
            text: "서보"
            color: "#111827"
            font.pixelSize: 14
        }

        Text {
            text: root.servoText
            color: root.servoText === "ON" ? root.normalColor : root.warningColor
            font.pixelSize: 14
        }

        Text {
            text: "비상정지"
            color: "#111827"
            font.pixelSize: 14
        }

        Text {
            text: root.estopText
            color: root.estopText === "0" ? root.normalColor : root.warningColor
            font.pixelSize: 14
        }

        Text {
            text: "에러"
            color: "#111827"
            font.pixelSize: 14
        }

        Text {
            text: root.errorText
            color: root.errorText === "0" ? root.normalColor : root.warningColor
            font.pixelSize: 14
        }
    }
}
