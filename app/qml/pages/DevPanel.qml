import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root
    anchors.fill: parent

    Rectangle {
        anchors.fill: parent
        radius: 8
        color: "white"
        border.color: "#d7dde6"

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 12

            Text {
                text: "DevPanel.qml"
                font.pixelSize: 30
                font.bold: true
                color: "#334155"
                horizontalAlignment: Text.AlignHCenter
                Layout.alignment: Qt.AlignHCenter
            }

            Text {
                text: "[DEBUG] Dev 탭 화면입니다.\n현재는 개발/테스트 인터페이스 미구현 상태입니다."
                font.pixelSize: 18
                color: "#64748b"
                horizontalAlignment: Text.AlignHCenter
                Layout.alignment: Qt.AlignHCenter
            }
        }
    }
}
