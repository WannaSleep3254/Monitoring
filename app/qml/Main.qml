import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15

ApplicationWindow {
    id: root

    width: 1920
    height: 1080
    visible: true
    title: "AVN Manager"

    color: "#eef2f7"

    property string currentPage: "Main"
    property string currentTimeText: "18:36:06"
    property string currentDateText: "2026년 05월 08일 (금)"
    property string systemStateText: "대기 중"
    property string currentRecipeText: "TK1_RH"

    function pageTitle() {
        if (currentPage === "Main")
            return "메인"
        if (currentPage === "Vision")
            return "비전"
        if (currentPage === "Robot")
            return "로봇"
        if (currentPage === "IoT")
            return "IoT"
        if (currentPage === "Dev")
            return "개발"

        return "메인"
    }

    function pageSource() {
        if (currentPage === "Main")
            return "qrc:/qml/pages/MainPanel.qml"
        if (currentPage === "Vision")
            return "qrc:/qml/pages/VisionPanel.qml"
        if (currentPage === "Robot")
            return "qrc:/qml/pages/RobotPanel.qml"
        if (currentPage === "IoT")
            return "qrc:/qml/pages/IoTPanel.qml"
        if (currentPage === "Dev")
            return "qrc:/qml/pages/DevPanel.qml"

        return "qrc:/qml/pages/MainPanel.qml"
    }

    Timer {
        interval: 1000
        running: true
        repeat: true

        onTriggered: {
            var now = new Date()

            var hh = String(now.getHours()).padStart(2, "0")
            var mm = String(now.getMinutes()).padStart(2, "0")
            var ss = String(now.getSeconds()).padStart(2, "0")
            root.currentTimeText = hh + ":" + mm + ":" + ss

            var yyyy = now.getFullYear()
            var month = String(now.getMonth() + 1).padStart(2, "0")
            var day = String(now.getDate()).padStart(2, "0")
            var week = ["일", "월", "화", "수", "목", "금", "토"][now.getDay()]
            root.currentDateText = yyyy + "년 " + month + "월 " + day + "일 (" + week + ")"
        }
    }

    // ------------------------------------------------------------
    // Common small button component
    // ------------------------------------------------------------
    component HeaderControlButton: Button {
        id: controlButton

        property color buttonColor: "#2563eb"
        property color hoverColor: Qt.darker(buttonColor, 1.1)
        property string symbolText: ""

        implicitWidth: 58
        implicitHeight: 46

        contentItem: Text {
            text: controlButton.symbolText
            color: "white"
            font.pixelSize: 22
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            radius: 8
            color: controlButton.down ? controlButton.hoverColor : controlButton.buttonColor
            border.color: Qt.darker(controlButton.buttonColor, 1.15)
        }
    }

    component SideMenuButton: Button {
        id: sideButton

        property bool selected: false

        Layout.fillWidth: true
        Layout.preferredHeight: 70

        contentItem: ColumnLayout {
            anchors.fill: parent
            spacing: 4

            Text {
                text: sideButton.text
                color: sideButton.selected ? "white" : "#cbd5e1"
                font.pixelSize: 14
                font.bold: sideButton.selected
                horizontalAlignment: Text.AlignHCenter
                Layout.alignment: Qt.AlignHCenter
            }
        }

        background: Rectangle {
            radius: 0
            color: sideButton.selected ? "#1d4ed8" : "#0b1f33"
            border.color: sideButton.selected ? "#3b82f6" : "#0b1f33"
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ============================================================
        // Left Sidebar
        // ============================================================
        Rectangle {
            id: sideBar

            Layout.preferredWidth: 108
            Layout.fillHeight: true

            color: "#0b1f33"

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 70
                    color: "#0b1f33"

                    Text {
                        anchors.centerIn: parent
                        text: "AVN"
                        color: "white"
                        font.pixelSize: 22
                        font.bold: true
                    }
                }

                SideMenuButton {
                    text: "Main"
                    selected: root.currentPage === "Main"
                    onClicked: root.currentPage = "Main"
                }

                SideMenuButton {
                    text: "Vision"
                    selected: root.currentPage === "Vision"
                    onClicked: root.currentPage = "Vision"
                }

                SideMenuButton {
                    text: "Robot"
                    selected: root.currentPage === "Robot"
                    onClicked: root.currentPage = "Robot"
                }

                SideMenuButton {
                    text: "IoT"
                    selected: root.currentPage === "IoT"
                    onClicked: root.currentPage = "IoT"
                }

                SideMenuButton {
                    text: "Deep Learning"
                    selected: root.currentPage === "DeepLearning"
                    onClicked: console.log("[QML] Deep Learning page placeholder")
                }

                SideMenuButton {
                    text: "Dev"
                    selected: root.currentPage === "Dev"
                    onClicked: root.currentPage = "Dev"
                }

                Item {
                    Layout.fillHeight: true
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 132
                    Layout.margins: 8
                    radius: 6
                    color: "#102b45"
                    border.color: "#244766"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 5

                        Text {
                            text: "연결 상태"
                            color: "#cbd5e1"
                            font.pixelSize: 11
                            font.bold: true
                        }

                        Text {
                            text: "Robot 1: Connected"
                            color: "#22c55e"
                            font.pixelSize: 10
                        }

                        Text {
                            text: "Robot 2: Connected"
                            color: "#22c55e"
                            font.pixelSize: 10
                        }

                        Text {
                            text: "Gantry: Connected"
                            color: "#22c55e"
                            font.pixelSize: 10
                        }
                    }
                }
            }
        }

        // ============================================================
        // Main Area
        // ============================================================
        Rectangle {
            id: mainArea

            Layout.fillWidth: true
            Layout.fillHeight: true

            color: "#eef2f7"

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // ====================================================
                // Top Header Bar
                // ====================================================
                Rectangle {
                    id: topHeader

                    Layout.fillWidth: true
                    Layout.preferredHeight: 74

                    color: "white"
                    border.color: "#d7dde6"

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 18
                        spacing: 14

                        // System state
                        Rectangle {
                            Layout.preferredWidth: 120
                            Layout.preferredHeight: 48
                            radius: 8
                            color: "#7f1d1d"
                            border.color: "#991b1b"

                            RowLayout {
                                anchors.centerIn: parent
                                spacing: 8

                                Rectangle {
                                    width: 10
                                    height: 10
                                    radius: 5
                                    color: "#ef4444"
                                }

                                Text {
                                    text: root.systemStateText
                                    color: "white"
                                    font.pixelSize: 18
                                    font.bold: true
                                }
                            }
                        }

                        // Control buttons
                        ColumnLayout {
                            Layout.preferredWidth: 46
                            Layout.preferredHeight: 48
                            spacing: 2

                            Text {
                                text: "설비"
                                color: "#64748b"
                                font.pixelSize: 11
                                horizontalAlignment: Text.AlignHCenter
                                Layout.alignment: Qt.AlignHCenter
                            }

                            Text {
                                text: "제어"
                                color: "#64748b"
                                font.pixelSize: 11
                                horizontalAlignment: Text.AlignHCenter
                                Layout.alignment: Qt.AlignHCenter
                            }
                        }

                        HeaderControlButton {
                            buttonColor: "#16a34a"
                            symbolText: "▶"
                            onClicked: console.log("[QML] Start clicked")
                        }

                        HeaderControlButton {
                            buttonColor: "#991b1b"
                            symbolText: "■"
                            onClicked: console.log("[QML] Stop clicked")
                        }

                        HeaderControlButton {
                            buttonColor: "#854d0e"
                            symbolText: "↻"
                            onClicked: console.log("[QML] Reset clicked")
                        }

                        // Recipe / product
                        Rectangle {
                            Layout.preferredWidth: 190
                            Layout.preferredHeight: 48
                            radius: 8
                            color: "#f1f5f9"
                            border.color: "#d7dde6"

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 14
                                anchors.rightMargin: 14
                                anchors.topMargin: 5
                                anchors.bottomMargin: 5
                                spacing: 2

                                Text {
                                    text: "제품명"
                                    color: "#64748b"
                                    font.pixelSize: 11
                                }

                                Text {
                                    text: root.currentRecipeText
                                    color: "#111827"
                                    font.pixelSize: 17
                                    font.bold: true
                                }
                            }
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        Button {
                            Layout.preferredWidth: 44
                            Layout.preferredHeight: 44

                            contentItem: Text {
                                text: "⛶"
                                color: "#334155"
                                font.pixelSize: 22
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            background: Rectangle {
                                radius: 8
                                color: "#f8fafc"
                                border.color: "#cbd5e1"
                            }

                            onClicked: {
                                if (root.visibility === Window.FullScreen)
                                    root.visibility = Window.Windowed
                                else
                                    root.visibility = Window.FullScreen
                            }
                        }

                        ColumnLayout {
                            Layout.preferredWidth: 160
                            Layout.preferredHeight: 56
                            spacing: 2

                            Text {
                                text: root.currentTimeText
                                color: "#111827"
                                font.pixelSize: 26
                                font.bold: true
                                horizontalAlignment: Text.AlignRight
                                Layout.fillWidth: true
                            }

                            Text {
                                text: root.currentDateText
                                color: "#64748b"
                                font.pixelSize: 11
                                horizontalAlignment: Text.AlignRight
                                Layout.fillWidth: true
                            }
                        }
                    }
                }

                // ====================================================
                // Page Content Area
                // ====================================================
                Rectangle {
                    id: contentFrame

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.margins: 12

                    radius: 8
                    color: "transparent"

                    Loader {
                        id: pageLoader
                        anchors.fill: parent
                        source: root.pageSource()

                        onLoaded: {
                            console.log("[QML] Page loaded:", source)
                        }
                    }
                }
            }
        }
    }
}
