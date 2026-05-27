import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Popup {
    id: dlg

    property int hostWidth: 1280
    property int hostHeight: 720

    property bool axisLegendReverseOrder: false

    property real robot1TemperatureYMin: 30
    property real robot1TemperatureYMax: 80
    property int  robot1TemperatureDecimals: 0
    property int  robot1TemperatureXPointCount: 60

    property real robot1TorqueYMin: 0
    property real robot1TorqueYMax: 2
    property int  robot1TorqueDecimals: 1
    property int  robot1TorqueXPointCount: 60

    property real robot2TemperatureYMin: 30
    property real robot2TemperatureYMax: 80
    property int  robot2TemperatureDecimals: 0
    property int  robot2TemperatureXPointCount: 60

    property real robot2TorqueYMin: 0
    property real robot2TorqueYMax: 2
    property int  robot2TorqueDecimals: 1
    property int  robot2TorqueXPointCount: 60

    signal applyRequested(var settings)
    signal resetRequested()

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    width: Math.min(1120, Math.max(940, hostWidth * 0.68))
    height: Math.min(760, Math.max(620, hostHeight * 0.78))

    x: (hostWidth - width) / 2
    y: (hostHeight - height) / 2

    padding: 0

    background: Rectangle {
        radius: 8
        color: "#ffffff"
        border.color: "#e0e4ea"
        border.width: 1
    }

    function numberOrFallback(text, fallback) {
        var value = Number(text)
        return isNaN(value) ? fallback : value
    }

    function intOrFallback(text, fallback, minValue) {
        var value = Math.round(Number(text))
        if (isNaN(value))
            return fallback
        return Math.max(minValue, value)
    }

    function makeSettings() {
        return {
            axisLegendReverseOrder: reverseCheck.checked,

            robot1TemperatureYMin: numberOrFallback(r1Temp.yMinText, robot1TemperatureYMin),
            robot1TemperatureYMax: numberOrFallback(r1Temp.yMaxText, robot1TemperatureYMax),
            robot1TemperatureDecimals: intOrFallback(r1Temp.decimalsText, robot1TemperatureDecimals, 0),
            robot1TemperatureXPointCount: intOrFallback(r1Temp.xPointCountText, robot1TemperatureXPointCount, 2),

            robot1TorqueYMin: numberOrFallback(r1Torque.yMinText, robot1TorqueYMin),
            robot1TorqueYMax: numberOrFallback(r1Torque.yMaxText, robot1TorqueYMax),
            robot1TorqueDecimals: intOrFallback(r1Torque.decimalsText, robot1TorqueDecimals, 0),
            robot1TorqueXPointCount: intOrFallback(r1Torque.xPointCountText, robot1TorqueXPointCount, 2),

            robot2TemperatureYMin: numberOrFallback(r2Temp.yMinText, robot2TemperatureYMin),
            robot2TemperatureYMax: numberOrFallback(r2Temp.yMaxText, robot2TemperatureYMax),
            robot2TemperatureDecimals: intOrFallback(r2Temp.decimalsText, robot2TemperatureDecimals, 0),
            robot2TemperatureXPointCount: intOrFallback(r2Temp.xPointCountText, robot2TemperatureXPointCount, 2),

            robot2TorqueYMin: numberOrFallback(r2Torque.yMinText, robot2TorqueYMin),
            robot2TorqueYMax: numberOrFallback(r2Torque.yMaxText, robot2TorqueYMax),
            robot2TorqueDecimals: intOrFallback(r2Torque.decimalsText, robot2TorqueDecimals, 0),
            robot2TorqueXPointCount: intOrFallback(r2Torque.xPointCountText, robot2TorqueXPointCount, 2)
        }
    }

    function applySettings() {
        var settings = makeSettings()

        if (settings.robot1TemperatureYMax <= settings.robot1TemperatureYMin)
            return
        if (settings.robot1TorqueYMax <= settings.robot1TorqueYMin)
            return
        if (settings.robot2TemperatureYMax <= settings.robot2TemperatureYMin)
            return
        if (settings.robot2TorqueYMax <= settings.robot2TorqueYMin)
            return

        applyRequested(settings)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 22
        spacing: 12

        // Header
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            Layout.fillHeight: false

            Text {
                Layout.fillWidth: true
                text: "그래프 표시 설정"
                font.family: "Asta Sans"
                font.pixelSize: 22
                font.bold: true
                color: "#212121"
                verticalAlignment: Text.AlignVCenter
            }

            DialogActionButton {
                text: "닫기"
                buttonWidth: 88
                variant: "default"
                onClicked: dlg.close()
            }
        }

        // Axis order
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 54
            Layout.fillHeight: false

            radius: 6
            color: "#f8fafc"
            border.color: "#e0e4ea"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                CheckBox {
                    id: reverseCheck
                    checked: dlg.axisLegendReverseOrder
                }

                Text {
                    Layout.fillWidth: true
                    text: "축별 현재값 순서 반전 표시 (J6 → J1)"
                    font.family: "Asta Sans"
                    font.pixelSize: 14
                    color: "#212121"
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        // Body
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ColumnLayout {
                width: dlg.width - 44
                spacing: 14

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 14

                    GraphScaleSettingBlock {
                        id: r1Temp
                        Layout.fillWidth: true
                        title: "Robot 1 - 온도 그래프"
                        yMinText: String(dlg.robot1TemperatureYMin)
                        yMaxText: String(dlg.robot1TemperatureYMax)
                        decimalsText: String(dlg.robot1TemperatureDecimals)
                        xPointCountText: String(dlg.robot1TemperatureXPointCount)
                    }

                    GraphScaleSettingBlock {
                        id: r1Torque
                        Layout.fillWidth: true
                        title: "Robot 1 - 부하 그래프"
                        yMinText: String(dlg.robot1TorqueYMin)
                        yMaxText: String(dlg.robot1TorqueYMax)
                        decimalsText: String(dlg.robot1TorqueDecimals)
                        xPointCountText: String(dlg.robot1TorqueXPointCount)
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 14

                    GraphScaleSettingBlock {
                        id: r2Temp
                        Layout.fillWidth: true
                        title: "Robot 2 - 온도 그래프"
                        yMinText: String(dlg.robot2TemperatureYMin)
                        yMaxText: String(dlg.robot2TemperatureYMax)
                        decimalsText: String(dlg.robot2TemperatureDecimals)
                        xPointCountText: String(dlg.robot2TemperatureXPointCount)
                    }

                    GraphScaleSettingBlock {
                        id: r2Torque
                        Layout.fillWidth: true
                        title: "Robot 2 - 부하 그래프"
                        yMinText: String(dlg.robot2TorqueYMin)
                        yMaxText: String(dlg.robot2TorqueYMax)
                        decimalsText: String(dlg.robot2TorqueDecimals)
                        xPointCountText: String(dlg.robot2TorqueXPointCount)
                    }
                }
            }
        }

        // Footer
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 42
            Layout.fillHeight: false

            Item { Layout.fillWidth: true }

            DialogActionButton {
                text: "기본값 복원"
                buttonWidth: 120
                variant: "default"
                onClicked: {
                    dlg.resetRequested()
                    dlg.close()
                }
            }

            DialogActionButton {
                text: "적용"
                buttonWidth: 90
                variant: "success"
                onClicked: {
                    applySettings()
//                    dlg.close()
                }
            }

            DialogActionButton {
                text: "닫기"
                buttonWidth: 90
                variant: "default"
                onClicked: dlg.close()
            }
        }
    }

    component GraphScaleSettingBlock: Rectangle {
        id: block

        property string title: ""
        property alias yMinText: yMinField.text
        property alias yMaxText: yMaxField.text
        property alias decimalsText: decimalsField.text
        property alias xPointCountText: xPointCountField.text

        Layout.preferredWidth: 430
        Layout.minimumWidth: 360
        Layout.preferredHeight: 210
        Layout.minimumHeight: 210
        Layout.fillHeight: false

        radius: 6
        color: "#f8fafc"
        border.color: "#e0e4ea"
        border.width: 1

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 10

            Text {
                Layout.fillWidth: true
                Layout.preferredHeight: 24
                text: block.title
                font.family: "Asta Sans"
                font.pixelSize: 15
                font.bold: true
                color: "#1976d2"
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }

            GridLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                columns: 2
                columnSpacing: 12
                rowSpacing: 8

                SettingLabel { text: "Y 최소값" }
                SettingTextField { id: yMinField }

                SettingLabel { text: "Y 최대값" }
                SettingTextField { id: yMaxField }

                SettingLabel { text: "소수점" }
                SettingTextField { id: decimalsField }

                SettingLabel { text: "X 표시 개수" }
                SettingTextField { id: xPointCountField }
            }
        }
    }

    component SettingLabel: Text {
        Layout.preferredWidth: 90
        Layout.alignment: Qt.AlignVCenter
        font.family: "Asta Sans"
        font.pixelSize: 12
        color: "#64748b"
        verticalAlignment: Text.AlignVCenter
    }

    component SettingTextField: TextField {
        Layout.fillWidth: true
        Layout.preferredHeight: 34
        selectByMouse: true
        font.family: "Asta Sans"
        font.pixelSize: 14
        color: "#212121"

        background: Rectangle {
            radius: 4
            color: "#ffffff"
            border.color: "#cbd5e1"
            border.width: 1
        }
    }

    component DialogActionButton: Button {
        id: btn

        property string variant: "default"
        property int buttonWidth: 90

        Layout.preferredWidth: buttonWidth
        Layout.preferredHeight: 34
        Layout.minimumHeight: 34
        Layout.maximumHeight: 34

        contentItem: Text {
            text: btn.text
            font.family: "Asta Sans"
            font.pixelSize: 13
            font.bold: btn.variant === "success" || btn.variant === "primary"
            color: {
                if (!btn.enabled)
                    return "#9ca3af"

                if (btn.variant === "success" || btn.variant === "primary")
                    return "#ffffff"

                return "#374151"
            }
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        background: Rectangle {
            radius: 4
            color: {
                if (!btn.enabled)
                    return "#e5e7eb"

                if (btn.variant === "success")
                    return btn.down ? "#15803d" : (btn.hovered ? "#16a34a" : "#16a34a")

                if (btn.variant === "primary")
                    return btn.down ? "#1d4ed8" : (btn.hovered ? "#2563eb" : "#2563eb")

                return btn.down ? "#cbd5e1" : (btn.hovered ? "#e5e7eb" : "#f8fafc")
            }
            border.color: {
                if (!btn.enabled)
                    return "#d1d5db"

                if (btn.variant === "success")
                    return "#16a34a"

                if (btn.variant === "primary")
                    return "#2563eb"

                return "#cbd5e1"
            }
            border.width: 1
        }
    }
}
