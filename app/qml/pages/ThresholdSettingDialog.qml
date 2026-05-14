import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Popup {
    id: dialogRoot

    property real hostWidth: parent ? parent.width : 1200
    property real hostHeight: parent ? parent.height : 700

    // IoTPanel에서 전달받는 로봇별 임계값 배열
    property var thresholds: []

    // robotIndex: 1-based
    // thresholdData:
    // {
    //   temperature: { normalMax, warningMax, alarmMax },
    //   current:     { normalMax, warningMax, alarmMax }
    // }
    signal saveRequested(int robotIndex, var thresholdData)

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

    function thresholdFor(robotIndex) {
        var idx = robotIndex - 1

        if (!dialogRoot.thresholds || idx < 0 || idx >= dialogRoot.thresholds.length)
            return dialogRoot.defaultThreshold()

        var source = dialogRoot.thresholds[idx]
        var fallback = dialogRoot.defaultThreshold()

        return {
            temperature: source.temperature || fallback.temperature,
            current: source.current || source.torque || fallback.current
        }
    }

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    width: Math.min(dialogRoot.hostWidth * 0.86, 1180)
    height: Math.min(dialogRoot.hostHeight * 0.76, 600)
    x: Math.round((dialogRoot.hostWidth - width) / 2)
    y: Math.round((dialogRoot.hostHeight - height) / 2)

    background: Rectangle {
        radius: 10
        color: "white"
        border.color: "#d7dde6"
    }

    contentItem: Item {
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 14

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 32

                Text {
                    text: "임계값 설정"
                    color: "#0f172a"
                    font.pixelSize: 20
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                DialogToolButton {
                    text: "닫기"
                    Layout.preferredWidth: 70
                    onClicked: dialogRoot.close()
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 14

                ThresholdRobotCard {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    robotTitle: "Robot 1 (FR10)"
                    robotIndex: 1
                    thresholdData: dialogRoot.thresholdFor(1)
                }

                ThresholdRobotCard {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    robotTitle: "Robot 2 (FR5)"
                    robotIndex: 2
                    thresholdData: dialogRoot.thresholdFor(2)
                }
            }
        }
    }

    component DialogToolButton: Button {
        id: toolButton

        Layout.preferredHeight: 28
        Layout.minimumHeight: 28
        Layout.maximumHeight: 28
        Layout.fillHeight: false

        font.pixelSize: 12

        contentItem: Text {
            text: toolButton.text
            color: "#334155"
            font.pixelSize: 12
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        background: Rectangle {
            radius: 5
            color: toolButton.pressed ? "#cbd5e1"
                 : toolButton.hovered ? "#e2e8f0"
                 : "#f1f5f9"
            border.color: "#cbd5e1"
        }
    }

    component ThresholdRobotCard: Rectangle {
        id: thresholdCard

        property string robotTitle: ""
        property int robotIndex: 0
        property var thresholdData: dialogRoot.defaultThreshold()

        radius: 8
        color: "#f8fafc"
        border.color: "#e2e8f0"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 12

            Text {
                Layout.fillWidth: true
                text: thresholdCard.robotTitle + " - 임계값 설정"
                color: "#2563eb"
                font.pixelSize: 18
                font.bold: true
                elide: Text.ElideRight
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 12

                ThresholdGroup {
                    id: tempGroup

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    title: "온도 임계값 (°C)"
                    normalValue: thresholdCard.thresholdData.temperature.normalMax
                    warningValue: thresholdCard.thresholdData.temperature.warningMax
                    alarmValue: thresholdCard.thresholdData.temperature.alarmMax
                    unitText: "°C"
                }

                ThresholdGroup {
                    id: currentGroup

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    title: "전류 임계값 (A)"
                    normalValue: thresholdCard.thresholdData.current.normalMax
                    warningValue: thresholdCard.thresholdData.current.warningMax
                    alarmValue: thresholdCard.thresholdData.current.alarmMax
                    unitText: "A"
                }
            }

            ExportButton {
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                text: "저장"
                buttonColor: "#16a34a"

                onClicked: {
                    var thresholdData = {
                        temperature: tempGroup.toThresholdData(),
                        current: currentGroup.toThresholdData()
                    }

                    console.log("[QML] threshold save requested, robot = " + thresholdCard.robotIndex)
                    dialogRoot.saveRequested(thresholdCard.robotIndex, thresholdData)
                }
            }
        }
    }

    component ThresholdGroup: Rectangle {
        id: groupRoot

        property string title: ""
        property real normalValue: 0
        property real warningValue: 0
        property real alarmValue: 0
        property string unitText: ""

        function toNumber(textValue, fallbackValue) {
            var v = Number(textValue)
            return isNaN(v) ? fallbackValue : v
        }

        function toThresholdData() {
            return {
                normalMax: groupRoot.toNumber(normalInput.text, groupRoot.normalValue),
                warningMax: groupRoot.toNumber(warningInput.text, groupRoot.warningValue),
                alarmMax: groupRoot.toNumber(alarmInput.text, groupRoot.alarmValue)
            }
        }

        radius: 6
        color: "white"
        border.color: "#d7dde6"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8

            Text {
                Layout.fillWidth: true
                text: groupRoot.title
                color: "#334155"
                font.pixelSize: 15
                font.bold: true
            }

            ThresholdInput {
                id: normalInput

                Layout.fillWidth: true
                labelText: "정상 MAX"
                valueText: String(groupRoot.normalValue)
                unitText: groupRoot.unitText
            }

            ThresholdInput {
                id: warningInput

                Layout.fillWidth: true
                labelText: "주의 MAX"
                valueText: String(groupRoot.warningValue)
                unitText: groupRoot.unitText
            }

            ThresholdInput {
                id: alarmInput

                Layout.fillWidth: true
                labelText: "경고 MAX"
                valueText: String(groupRoot.alarmValue)
                unitText: groupRoot.unitText
            }

            Item { Layout.fillHeight: true }
        }
    }

    component ThresholdInput: ColumnLayout {
        id: inputRoot

        property string labelText: ""
        property string valueText: ""
        property string unitText: ""
        property alias text: thresholdTextField.text

        spacing: 4

        Text {
            Layout.fillWidth: true
            text: inputRoot.labelText
            color: "#64748b"
            font.pixelSize: 12
        }

        TextField {
            id: thresholdTextField

            Layout.fillWidth: true
            Layout.preferredHeight: 34
            text: inputRoot.valueText
            font.pixelSize: 13
            selectByMouse: true

            validator: DoubleValidator {
                bottom: 0
                top: 9999
                decimals: 3
            }

            background: Rectangle {
                radius: 5
                color: "#f8fafc"
                border.color: thresholdTextField.activeFocus ? "#2563eb" : "#cbd5e1"
            }

            rightPadding: 30

            Text {
                anchors.right: parent.right
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                text: inputRoot.unitText
                color: "#94a3b8"
                font.pixelSize: 11
            }
        }
    }

    component ExportButton: Button {
        id: exportButton

        property color buttonColor: "#2563eb"

        Layout.fillWidth: true
        Layout.preferredHeight: 34
        Layout.fillHeight: false

        font.pixelSize: 12

        contentItem: Text {
            text: exportButton.text
            color: "white"
            font.pixelSize: 12
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        background: Rectangle {
            radius: 5
            color: exportButton.pressed ? Qt.darker(exportButton.buttonColor, 1.15)
                 : exportButton.hovered ? Qt.lighter(exportButton.buttonColor, 1.08)
                 : exportButton.buttonColor
        }
    }
}
