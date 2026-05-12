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
            anchors.fill: parent
            anchors.margins: 24
            spacing: 18

            Text {
                text: "IoT 패널"
                color: "#2563eb"
                font.pixelSize: 26
                font.bold: true
            }

            Text {
                text: "축별 온도 / 토크 / 예지보전 모니터링 영역"
                color: "#334155"
                font.pixelSize: 18
            }

            GridLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                columns: 3
                rowSpacing: 14
                columnSpacing: 14

                Repeater {
                    model: ["Robot 1", "Robot 2", "Gantry X/Z"]

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 8
                        color: "#f8fafc"
                        border.color: "#e2e8f0"

                        Text {
                            anchors.centerIn: parent
                            text: modelData + "\nMonitoring Chart Area"
                            horizontalAlignment: Text.AlignHCenter
                            font.pixelSize: 22
                            color: "#334155"
                        }
                    }
                }
            }
        }
    }
}
