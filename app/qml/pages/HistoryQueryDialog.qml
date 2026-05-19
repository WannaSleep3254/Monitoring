import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Popup {
    id: dialogRoot

    property real hostWidth: parent ? parent.width : 1200
    property real hostHeight: parent ? parent.height : 700

    property var viewModel: null
    property bool hasViewModel: viewModel !== null && viewModel !== undefined

    signal exportCsvRequested(var rows, string robotFilter, string periodFilter, string typeFilter, string searchText)

    signal alarmConfirmRequested(var alarmRow)
    signal alarmActionRequested(var alarmRow)


    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    width: Math.min(dialogRoot.hostWidth * 0.90, 1280)
    height: Math.min(dialogRoot.hostHeight * 0.82, 660)
    x: Math.round((dialogRoot.hostWidth  - width)  / 2)
    y: Math.round((dialogRoot.hostHeight - height) / 2)

    background: Rectangle {
        radius: 10
        color: "#ffffff"
        border.color: "#e0e4ea"
    }

    onOpened: {
        dialogRoot.selectedHistoryIndex = 0
        dialogRoot.requestHistoryQuery()
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
                    text: "이력 조회 / 내보내기"
                    color: "#212121"
                    font.family:    "Asta Sans"
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

            // ------------------------------------------------------------
            // 조회 조건
            // ------------------------------------------------------------
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: historyPeriodCombo.currentText === "사용자 지정" ? 98 : 62
                Layout.fillHeight: false
                spacing: 14

                ColumnLayout {
                    Layout.preferredWidth: 250
                    spacing: 6

                    Text {
                        text: "로봇 선택"
                        color: "#374151"
                        font.family:    "Asta Sans"
                        font.pixelSize: 13
                        font.bold: true
                    }

                    ComboBox {
                        id: historyRobotCombo
                        Layout.fillWidth: true
                        Layout.preferredHeight: 34
                        model: ["전체", "Robot 1", "Robot 2"]
                        font.family:    "Asta Sans"
                        font.pixelSize: 12

                        onCurrentTextChanged: dialogRoot.requestHistoryQuery()

                        contentItem: Text {
                            leftPadding:        8
                            text:               historyRobotCombo.displayText
                            font.family:        "Asta Sans"
                            font.pixelSize:     12
                            color:              "#212121"
                            verticalAlignment:  Text.AlignVCenter
                            elide:              Text.ElideRight
                        }

                        background: Rectangle {
                            radius:       5
                            color:        "#fafafa"
                            border.color: historyRobotCombo.activeFocus ? "#1976d2" : "#d1d5db"
                            border.width: 1
                        }
                    }
                }

                ColumnLayout {
                    Layout.preferredWidth: 230
                    spacing: 6

                    Text {
                        text: "기간"
                        color: "#374151"
                        font.family:    "Asta Sans"
                        font.pixelSize: 13
                        font.bold: true
                    }

                    ComboBox {
                        id: historyPeriodCombo
                        Layout.fillWidth: true
                        Layout.preferredHeight: 34
                        model: ["오늘", "최근 7일", "최근 30일", "최근 90일", "사용자 지정"]
                        font.family:    "Asta Sans"
                        font.pixelSize: 12

                        onCurrentTextChanged: {
                            if (historyPeriodCombo.currentText === "사용자 지정") {
                                var now = new Date()
                                var from = new Date(now)
                                from.setDate(now.getDate() - 7)

                                if (dialogRoot.customFromDateText === "")
                                    dialogRoot.customFromDateText = dialogRoot.toDateText(from)

                                if (dialogRoot.customToDateText === "")
                                    dialogRoot.customToDateText = dialogRoot.toDateText(now)
                            }

                            dialogRoot.requestHistoryQuery()
                        }

                        contentItem: Text {
                            leftPadding:        8
                            text:               historyPeriodCombo.displayText
                            font.family:        "Asta Sans"
                            font.pixelSize:     12
                            color:              "#212121"
                            verticalAlignment:  Text.AlignVCenter
                            elide:              Text.ElideRight
                        }

                        background: Rectangle {
                            radius:       5
                            color:        "#fafafa"
                            border.color: historyPeriodCombo.activeFocus ? "#1976d2" : "#d1d5db"
                            border.width: 1
                        }
                    }
                    // 사용자 지정 기간 입력 필드
                    RowLayout {
                        visible: historyPeriodCombo.currentText === "사용자 지정"
                        Layout.fillWidth: true
                        Layout.preferredHeight: 30
                        spacing: 6

                        TextField {
                            id: customFromDateField
                            Layout.fillWidth: true
                            Layout.preferredHeight: 28
                            text: dialogRoot.customFromDateText
                            placeholderText: "YYYY-MM-DD"
                            selectByMouse: true
                            font.family: "Asta Sans"
                            font.pixelSize: 11

                            onTextChanged: dialogRoot.customFromDateText = text

                            onEditingFinished: {
                                dialogRoot.requestHistoryQuery()
                            }

                            background: Rectangle {
                                radius: 5
                                color: "#ffffff"
                                border.color: parent.activeFocus ? "#2563eb" : "#d1d5db"
                            }
                        }

                        Text {
                            text: "~"
                            color: "#64748b"
                            font.family: "Asta Sans"
                            font.pixelSize: 12
                            verticalAlignment: Text.AlignVCenter
                        }

                        TextField {
                            id: customToDateField
                            Layout.fillWidth: true
                            Layout.preferredHeight: 28
                            text: dialogRoot.customToDateText
                            placeholderText: "YYYY-MM-DD"
                            selectByMouse: true
                            font.family: "Asta Sans"
                            font.pixelSize: 11

                            onTextChanged: dialogRoot.customToDateText = text

                            onEditingFinished: {
                                dialogRoot.requestHistoryQuery()
                            }

                            background: Rectangle {
                                radius: 5
                                color: "#ffffff"
                                border.color: parent.activeFocus ? "#2563eb" : "#d1d5db"
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.preferredWidth: 300
                    spacing: 6

                    Text {
                        text: "이력 구분"
                        color: "#374151"
                        font.family:    "Asta Sans"
                        font.pixelSize: 13
                        font.bold: true
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 34
                        spacing: 4

                        HistorySegmentButton {
                            text: "전체"
                            selected: dialogRoot.historyFilterType === "전체"
                            onClicked: {
                                dialogRoot.historyFilterType = "전체"
                                dialogRoot.requestHistoryQuery()
                            }
                        }

                        HistorySegmentButton {
                            text: "알람 이력"
                            selected: dialogRoot.historyFilterType === "알람 이력"
                            onClicked: {
                                dialogRoot.historyFilterType = "알람 이력"
                                //dialogRoot.selectedHistoryIndex = 0
                                dialogRoot.requestHistoryQuery()
                            }
                        }

                        HistorySegmentButton {
                            text: "조치 이력"
                            selected: dialogRoot.historyFilterType === "조치 이력"
                            onClicked: {
                                dialogRoot.historyFilterType = "조치 이력"
                                //dialogRoot.selectedHistoryIndex = 0
                                dialogRoot.requestHistoryQuery()
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Text {
                        text: "검색"
                        color: "#374151"
                        font.family:    "Asta Sans"
                        font.pixelSize: 13
                        font.bold: true
                    }

                    TextField {
                        id: historySearchField
                        Layout.fillWidth: true
                        Layout.preferredHeight: 34
                        placeholderText: "설명 / 조치내용 검색"
                        color:           "#212121"
                        font.family:     "Asta Sans"
                        font.pixelSize:  12
                        selectByMouse:   true

                        onTextChanged: dialogRoot.selectedHistoryIndex = 0
                        onAccepted: dialogRoot.requestHistoryQuery()

                        background: Rectangle {
                            radius: 5
                            color: "#fafafa"
                            border.color: historySearchField.activeFocus ? "#1976d2" : "#d1d5db"
                        }
                    }
                }
            }

            // ------------------------------------------------------------
            // CSV 내보내기 + 요약 카운트
            // ------------------------------------------------------------
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 34
                Layout.fillHeight: false
                spacing: 10

                ExportButton {
                    text: "CSV 다운로드"
                    buttonColor: "#2563eb"
                    Layout.fillWidth: false
                    Layout.preferredWidth: 150
                    onClicked: {
                            var rows = dialogRoot.filteredHistoryRows(historySearchField.text,
                                                                       historyRobotCombo.currentText,
                                                                       dialogRoot.historyFilterType)
                            console.log("[QML] CSV export requested, rows = " + rows.length)
                            dialogRoot.exportCsvRequested(rows,
                                                          historyRobotCombo.currentText,
                                                          historyPeriodCombo.currentText,
                                                          dialogRoot.historyFilterType,
                                                          historySearchField.text)
                        }
                }

                ExportButton {
                    text: "90일 이전 삭제"
                    buttonColor: "#64748b"
                    Layout.fillWidth: false
                    Layout.preferredWidth: 130

                    onClicked: {
                        cleanupConfirmPopup.open()
                    }
                }

                Item { Layout.fillWidth: true }

                HistorySummaryCard {
                    labelText: "알람"
                    valueText: dialogRoot.historyCount("알람", "") + " 건"
                    accentColor: "#f97316"
                }

                HistorySummaryCard {
                    labelText: "조치"
                    valueText: dialogRoot.historyCount("조치", "") + " 건"
                    accentColor: "#2563eb"
                }

                HistorySummaryCard {
                    labelText: "완료"
                    valueText: dialogRoot.historyCount("", "완료") + " 건"
                    accentColor: "#16a34a"
                }
            }

            // ------------------------------------------------------------
            // 본문: 좌측 테이블 + 우측 상세 패널
            // ------------------------------------------------------------
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 14

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 6
                    color: "#ffffff"
                    border.color: "#e0e4ea"
                    clip: true

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0

                        // 헤더 + 데이터: 단일 Flickable(가로+세로)
                        Flickable {
                            id: tableFlick
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            contentWidth:  Math.max(width, 600)
                            contentHeight: tableRows.implicitHeight
                            flickableDirection: Flickable.HorizontalAndVerticalFlick
                            boundsBehavior: Flickable.DragAndOvershootBounds
                            clip: true

                            ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }
                            ScrollBar.vertical:   ScrollBar { policy: ScrollBar.AsNeeded }

                            Column {
                                id: tableRows
                                width: tableFlick.contentWidth
                                spacing: 0

                                HistoryTableRow {
                                    width: parent.width
                                    height: 34
                                    isHeader: true
                                    timeText: "일시"
                                    robotText: "로봇"
                                    kindText: "구분"
                                    axisText: "축"
                                    tempText: "온도"
                                    torqueText: "토크"
                                    statusText: "상태"
                                    descText: "설명"
                                    actionStatusText: "조치상태"
                                }

                                Repeater {
                                    model: dialogRoot.filteredHistoryRows(historySearchField.text,
                                                                    historyRobotCombo.currentText,
                                                                    dialogRoot.historyFilterType)

                                    HistoryTableRow {
                                        width: tableRows.width
                                        height: 34
                                        isHeader: false
                                        selected: index === dialogRoot.selectedHistoryIndex

                                        timeText: dialogRoot.displayDateTime(modelData)
                                        robotText: modelData.robot
                                        kindText: modelData.kind
                                        axisText: modelData.axis
                                        tempText: modelData.temp
                                        torqueText: modelData.torque
                                        statusText: modelData.status
                                        descText: modelData.desc
                                        actionStatusText: modelData.actionStatus

                                        onRowClicked: {
                                            dialogRoot.selectedHistoryIndex = index
                                            dialogRoot.actionEditMode = false
                                            dialogRoot.actionTargetItem = null
                                        }
                                    }
                                }
                            }
                        }

                        // 푸터: 건수
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 36
                            Layout.fillHeight: false
                            color: "#fafafa"
                            border.color: "#e2e8f0"

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 12
                                spacing: 8

                                Text {
                                    property var rows: dialogRoot.filteredHistoryRows(historySearchField.text,
                                                                               historyRobotCombo.currentText,
                                                                               dialogRoot.historyFilterType)
                                    text: "총 " + rows.length + "건"
                                    color: "#374151"
                                    font.family:    "Asta Sans"
                                    font.pixelSize: 12
                                }

                                Text {
                                    property var rows: dialogRoot.filteredHistoryRows(historySearchField.text,
                                                                               historyRobotCombo.currentText,
                                                                               dialogRoot.historyFilterType)
                                    text: rows.length > 0
                                          ? " |  " + (dialogRoot.clampHistoryIndex(rows) + 1) + " / " + rows.length
                                          : " |  0 / 0"
                                    color: "#9e9e9e"
                                    font.family:    "Asta Sans"
                                    font.pixelSize: 12
                                }

                                Item { Layout.fillWidth: true }
                            }
                        }
                    }
                }

                Rectangle {
                    id: historyDetailPanel

                    property var rows: dialogRoot.filteredHistoryRows(historySearchField.text,
                                                                historyRobotCombo.currentText,
                                                                dialogRoot.historyFilterType)
                    property int safeIndex: dialogRoot.clampHistoryIndex(rows)
                    property var selectedItem: safeIndex >= 0 ? rows[safeIndex] : null

                    Layout.preferredWidth: 280
                    Layout.fillHeight: true
                    radius: 6
                    color: "#ffffff"
                    border.color: "#e0e4ea"
                    clip: true

                    Flickable {
                        id: detailFlick
                        anchors.fill: parent
                        contentWidth:  width
                        contentHeight: detailCol.implicitHeight
                        flickableDirection: Flickable.VerticalFlick
                        boundsBehavior: Flickable.DragAndOvershootBounds
                        clip: false

                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                        ColumnLayout {
                            id: detailCol
                            width:   detailFlick.width - 24
                            anchors.top:   parent.top
                            anchors.left:  parent.left
                            anchors.leftMargin:  12
                            anchors.topMargin:   12
                            spacing: 8

                            Text {
                                Layout.fillWidth: true
                                text: "선택 이력 상세"
                                color: "#212121"
                                font.family:    "Asta Sans"
                                font.pixelSize: 16
                                font.bold: true
                            }

                            HistoryDetailItem {
                                labelText: "이력 종류"
                                valueText: historyDetailPanel.selectedItem ? historyDetailPanel.selectedItem.kind + " 이력" : "-"
                            }

                            HistoryDetailItem {
                                labelText: "발생 일시"
                                valueText: historyDetailPanel.selectedItem
                                           ? dialogRoot.displayDateTime(historyDetailPanel.selectedItem)
                                           : "-"
                            }

                            HistoryDetailItem {
                                labelText: "로봇"
                                valueText: historyDetailPanel.selectedItem ? historyDetailPanel.selectedItem.robot : "-"
                            }

                            HistoryDetailItem {
                                labelText: "대상 축"
                                valueText: historyDetailPanel.selectedItem ? historyDetailPanel.selectedItem.axis : "-"
                            }

                            HistoryDetailItem {
                                labelText: "위험도"
                                valueText: historyDetailPanel.selectedItem
                                           ? (historyDetailPanel.selectedItem.status === "주의" ? "주의 (82)"
                                              : historyDetailPanel.selectedItem.status === "경고" ? "경고 (91)"
                                              : "정상")
                                           : "-"
                                valueColor: historyDetailPanel.selectedItem && historyDetailPanel.selectedItem.status === "경고" ? "#dc2626"
                                           : historyDetailPanel.selectedItem && historyDetailPanel.selectedItem.status === "주의" ? "#f97316"
                                           : "#16a34a"
                            }

                            HistoryDetailItem {
                                labelText: "원인"
                                valueText: historyDetailPanel.selectedItem ? historyDetailPanel.selectedItem.cause : "-"
                            }

                            HistoryDetailItem {
                                labelText: "조치 상태"
                                valueText: historyDetailPanel.selectedItem ? historyDetailPanel.selectedItem.actionStatus : "-"
                                valueColor: historyDetailPanel.selectedItem && historyDetailPanel.selectedItem.actionStatus === "완료" ? "#16a34a"
                                           : historyDetailPanel.selectedItem && historyDetailPanel.selectedItem.actionStatus === "확인중" ? "#2563eb"
                                           : "#f97316"
                            }

                            RowLayout {
                                visible: dialogRoot.actionEditMode
                                Layout.fillWidth: true
                                Layout.preferredHeight: 34
                                spacing: 8

                                Text {
                                    Layout.preferredWidth: 82
                                    text: "저장 상태"
                                    font.family: "Asta Sans"
                                    font.pixelSize: 12
                                    font.bold: true
                                    color: "#9e9e9e"
                                    elide: Text.ElideRight
                                }

                                ComboBox {
                                    id: actionStatusCombo
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 30
                                    model: ["완료", "보류", "확인중"]
                                    currentIndex: model.indexOf(dialogRoot.actionStatusText)

                                    font.family: "Asta Sans"
                                    font.pixelSize: 12

                                    onCurrentTextChanged: {
                                        dialogRoot.actionStatusText = currentText
                                    }

                                    background: Rectangle {
                                        radius: 5
                                        color: "#ffffff"
                                        border.color: parent.activeFocus ? "#2563eb" : "#d1d5db"
                                    }
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Layout.preferredHeight: dialogRoot.actionEditMode ? 34 : 24

                                Text {
                                    Layout.preferredWidth: 82
                                    text: "조치자"
                                    font.family: "Asta Sans"
                                    font.pixelSize: 12
                                    font.bold: true
                                    color: "#9e9e9e"
                                    elide: Text.ElideRight
                                }

                                Text {
                                    visible: !dialogRoot.actionEditMode
                                    Layout.fillWidth: true
                                    text: historyDetailPanel.selectedItem
                                          ? historyDetailPanel.selectedItem.operatorName
                                          : "-"
                                    font.family: "Asta Sans"
                                    font.pixelSize: 12
                                    color: "#374151"
                                    elide: Text.ElideRight
                                }

                                TextField {
                                    visible: dialogRoot.actionEditMode
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 30
                                    text: dialogRoot.actionOperatorText
                                    placeholderText: "조치자 입력"
                                    selectByMouse: true
                                    font.family: "Asta Sans"
                                    font.pixelSize: 12

                                    onTextChanged: dialogRoot.actionOperatorText = text

                                    background: Rectangle {
                                        radius: 5
                                        color: "#ffffff"
                                        border.color: parent.activeFocus ? "#2563eb" : "#d1d5db"
                                    }
                                }
                            }
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: dialogRoot.actionEditMode ? 92 : 60
                                Layout.minimumHeight: 0
                                Layout.fillHeight: false
                                radius: 6
                                color: "#fafafa"
                                border.color: "#e2e8f0"
                                clip: true

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 8
                                    spacing: 6

                                    Text {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 16
                                        Layout.fillHeight: false
                                        text: "조치 내용"
                                        color: "#9e9e9e"
                                        font.family:    "Asta Sans"
                                        font.pixelSize: 12
                                        font.bold: true
                                        verticalAlignment: Text.AlignVCenter
                                    }

                                    Text {
                                        visible: !dialogRoot.actionEditMode
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        text: historyDetailPanel.selectedItem
                                              ? historyDetailPanel.selectedItem.actionContent
                                              : "-"
                                        color: "#374151"
                                        font.family: "Asta Sans"
                                        font.pixelSize: 12
                                        wrapMode: Text.WordWrap
                                        elide: Text.ElideRight
                                        maximumLineCount: 2
                                    }

                                    TextArea {
                                        visible: dialogRoot.actionEditMode
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        text: dialogRoot.actionContentText
                                        placeholderText: "조치 내용을 입력하세요"
                                        selectByMouse: true
                                        wrapMode: TextArea.Wrap
                                        font.family: "Asta Sans"
                                        font.pixelSize: 12

                                        onTextChanged: dialogRoot.actionContentText = text

                                        background: Rectangle {
                                            radius: 5
                                            color: "#ffffff"
                                            border.color: parent.activeFocus ? "#2563eb" : "#d1d5db"
                                        }
                                    }
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 54
                                Layout.minimumHeight: 54
                                Layout.fillHeight: false
                                spacing: 8

                                visible: !dialogRoot.actionEditMode

                                HistoryActionCardButton {
                                    titleText: "확인"
                                    bodyText: "알람 확인"
                                    accentColor: "#2563eb"

                                    enabled: dialogRoot.canConfirmAlarm(historyDetailPanel.selectedItem)

                                    onClicked: {
                                        if (!dialogRoot.canConfirmAlarm(historyDetailPanel.selectedItem))
                                            return

                                        dialogRoot.alarmConfirmRequested(historyDetailPanel.selectedItem)
                                    }
                                }

                                HistoryActionCardButton {
                                    titleText: "조치 등록"
                                    bodyText: "조치내용 입력"
                                    accentColor: "#16a34a"

                                    enabled: dialogRoot.canRegisterAction(historyDetailPanel.selectedItem)

                                    onClicked: {
                                        if (!dialogRoot.canRegisterAction(historyDetailPanel.selectedItem))
                                            return

                                        dialogRoot.actionTargetItem = historyDetailPanel.selectedItem
                                        dialogRoot.actionOperatorText = "작업자"
                                        dialogRoot.actionContentText = ""
                                        dialogRoot.actionMemoText = historyDetailPanel.selectedItem
                                                                    ? historyDetailPanel.selectedItem.cause
                                                                    : ""
                                        dialogRoot.actionStatusText = "완료"
                                        dialogRoot.actionEditMode = true
                                    }
                                }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 54
                                Layout.minimumHeight: 54
                                Layout.fillHeight: false
                                spacing: 8
                                visible: dialogRoot.actionEditMode

                                HistoryActionCardButton {
                                    titleText: "저장"
                                    bodyText: "조치 이력 저장"
                                    accentColor: "#16a34a"

                                    enabled: dialogRoot.actionContentText.trim().length > 0

                                    onClicked: {
                                        var content = dialogRoot.actionContentText.trim()

                                        if (content.length <= 0)
                                            return

                                        var target = dialogRoot.actionTargetItem

                                        if (!target) {
                                            console.warn("[QML] action save failed: target item is null")
                                            return
                                        }

                                        var alarmId = 0
                                        var robotId = 0

                                        if (target.kind === "알람")
                                            alarmId = Number(target.id)
                                        else if (target.kind === "조치")
                                            alarmId = Number(target.alarmId)

                                        robotId = Number(target.robotId)

                                        if (alarmId <= 0 || robotId <= 0) {
                                            console.warn("[QML] action save failed: invalid target",
                                                         "alarmId =", alarmId,
                                                         "robotId =", robotId,
                                                         "kind =", target.kind)
                                            return
                                        }

                                        if (!dialogRoot.hasViewModel || !dialogRoot.viewModel.saveAction) {
                                            console.warn("[QML] action save failed: viewModel.saveAction is not available")
                                            return
                                        }

                                        var actionData = {
                                            "alarmId": alarmId,
                                            "robotId": robotId,
                                            "status": dialogRoot.actionStatusText,
                                            "operatorName": dialogRoot.actionOperatorText,
                                            "actionContent": content,
                                            "memo": dialogRoot.actionMemoText
                                        }

                                        console.log("[QML] action save requested:",
                                                    "alarmId =", alarmId,
                                                    "robotId =", robotId,
                                                    "status =", dialogRoot.actionStatusText,
                                                    "operator =", dialogRoot.actionOperatorText,
                                                    "content =", content)

                                        if (!dialogRoot.viewModel.saveAction(actionData)) {
                                            console.warn("[QML] action save failed:",
                                                         dialogRoot.viewModel.lastError)
                                            return
                                        }

                                        dialogRoot.actionEditMode = false
                                        dialogRoot.actionTargetItem = null
                                        dialogRoot.actionOperatorText = "작업자"
                                        dialogRoot.actionContentText = ""
                                        dialogRoot.actionMemoText = ""
                                        dialogRoot.actionStatusText = "완료"

                                        dialogRoot.requestHistoryQuery()
                                    }
                                }

                                HistoryActionCardButton {
                                    titleText: "취소"
                                    bodyText: "입력 취소"
                                    accentColor: "#64748b"

                                    onClicked: {
                                        dialogRoot.actionEditMode = false
                                        dialogRoot.actionTargetItem = null
                                        dialogRoot.actionContentText = ""
                                        dialogRoot.actionMemoText = ""
                                        dialogRoot.actionStatusText = "완료"
                                    }
                                }
                            }
                        } // end ColumnLayout
                    } // end Flickable
                }
            }
        }
    }
    // ===== 90일 이전 이력 삭제 확인 팝업 =====
    Popup {
        id: cleanupConfirmPopup

        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        width: 380
        height: 190
        x: Math.round((dialogRoot.width - width) / 2)
        y: Math.round((dialogRoot.height - height) / 2)

        background: Rectangle {
            radius: 8
            color: "#ffffff"
            border.color: "#e2e8f0"
        }

        contentItem: ColumnLayout {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 12

            Text {
                Layout.fillWidth: true
                text: "90일 이전 이력 삭제"
                color: "#111827"
                font.family: "Asta Sans"
                font.pixelSize: 16
                font.bold: true
            }

            Text {
                Layout.fillWidth: true
                text: "90일 이전 알람/조치 이력을 삭제하시겠습니까?\n삭제된 이력은 복구할 수 없습니다."
                color: "#475569"
                font.family: "Asta Sans"
                font.pixelSize: 12
                wrapMode: Text.WordWrap
            }

            Item { Layout.fillHeight: true }

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 34
                spacing: 8

                Item { Layout.fillWidth: true }

                DialogToolButton {
                    text: "취소"
                    Layout.preferredWidth: 72
                    onClicked: cleanupConfirmPopup.close()
                }

                ExportButton {
                    text: "삭제"
                    buttonColor: "#dc2626"
                    Layout.fillWidth: false
                    Layout.preferredWidth: 72

                    onClicked: {
                        if (!dialogRoot.hasViewModel ||
                            !dialogRoot.viewModel.deleteOldHistoryDays) {
                            console.warn("[QML] deleteOldHistoryDays is not available")
                            return
                        }

                        if (!dialogRoot.viewModel.deleteOldHistoryDays(90)) {
                            console.warn("[QML] delete old history failed:",
                                         dialogRoot.viewModel.lastError)

                            cleanupResultPopup.titleText = "삭제 실패"
                            cleanupResultPopup.messageText =
                                    "90일 이전 이력 삭제에 실패했습니다.\n" +
                                    dialogRoot.viewModel.lastError

                            cleanupConfirmPopup.close()
                            cleanupResultPopup.open()
                            return
                        }

                        cleanupConfirmPopup.close()
                        dialogRoot.requestHistoryQuery()

                        var summary = dialogRoot.viewModel.lastCleanupSummary

                        var alarmsDeleted = summary && summary.alarmsDeleted !== undefined
                                ? Number(summary.alarmsDeleted)
                                : 0

                        var actionsDeleted = summary && summary.actionsDeleted !== undefined
                                ? Number(summary.actionsDeleted)
                                : 0

                        var totalDeleted = summary && summary.totalDeleted !== undefined
                                ? Number(summary.totalDeleted)
                                : 0

                        cleanupResultPopup.titleText = "삭제 완료"
                        cleanupResultPopup.messageText =
                                "90일 이전 알람/조치 이력 삭제 처리가 완료되었습니다.\n" +
                                "삭제된 알람: " + alarmsDeleted + "건\n" +
                                "삭제된 조치: " + actionsDeleted + "건\n" +
                                "총 삭제: " + totalDeleted + "건"

                        cleanupResultPopup.open()
                    }
                }
            }
        }
    }
    // ===== 삭제 결과 알림 팝업 =====
    Popup {
        id: cleanupResultPopup

        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        width: 340
        height: 150
        x: Math.round((dialogRoot.width - width) / 2)
        y: Math.round((dialogRoot.height - height) / 2)

        property string titleText: "삭제 완료"
        property string messageText: ""

        background: Rectangle {
            radius: 8
            color: "#ffffff"
            border.color: "#e2e8f0"
        }

        contentItem: ColumnLayout {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 12

            Text {
                Layout.fillWidth: true
                text: cleanupResultPopup.titleText
                color: "#111827"
                font.family: "Asta Sans"
                font.pixelSize: 16
                font.bold: true
            }

            Text {
                Layout.fillWidth: true
                text: cleanupResultPopup.messageText
                color: "#475569"
                font.family: "Asta Sans"
                font.pixelSize: 12
                wrapMode: Text.WordWrap
            }

            Item { Layout.fillHeight: true }

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 34

                Item { Layout.fillWidth: true }

                DialogToolButton {
                    text: "확인"
                    Layout.preferredWidth: 72
                    onClicked: cleanupResultPopup.close()
                }
            }
        }
    }
    // ===== 이력 조회 샘플 데이터 =====
    // kind:
    // - "알람" : 센서/위험도 기반으로 GUI가 자동 생성한 이력
    // - "조치" : 알람에 대한 작업자 확인/완료/보류 이력
    property var sampleHistoryRows: [
        {
            time: "10:43:21",
            robot: "Robot 1",
            kind: "알람",
            axis: "J3",
            temp: "41.8",
            torque: "15.6",
            status: "주의",
            desc: "J3 토크 임계치 접근",
            actionStatus: "요청",
            cause: "J3 토크 상승",
            operatorName: "시스템",
            actionContent: "J3 축 토크 추세 점검 권장",
            recordMode: "GUI 자동 기록"
        },
        {
            time: "10:44:05",
            robot: "Robot 1",
            kind: "조치",
            axis: "J3",
            temp: "-",
            torque: "-",
            status: "정상",
            desc: "작업자 확인 시작",
            actionStatus: "확인중",
            cause: "J3 토크 상승",
            operatorName: "작업자 A",
            actionContent: "J3 축 주변 간섭 여부 확인 중",
            recordMode: "작업자 수동 입력"
        },
        {
            time: "10:48:30",
            robot: "Robot 1",
            kind: "조치",
            axis: "J3",
            temp: "-",
            torque: "-",
            status: "정상",
            desc: "J3 축 추세 점검 완료",
            actionStatus: "완료",
            cause: "J3 토크 상승",
            operatorName: "작업자 A",
            actionContent: "간섭 없음 확인, 모니터링 지속",
            recordMode: "작업자 수동 입력"
        },
        {
            time: "10:43:20",
            robot: "Robot 2",
            kind: "알람",
            axis: "J4",
            temp: "47.1",
            torque: "16.2",
            status: "경고",
            desc: "J4 토크 초과 감지",
            actionStatus: "요청",
            cause: "J4 토크 초과",
            operatorName: "시스템",
            actionContent: "J4 축 이상 징후 감지, 점검 요청",
            recordMode: "GUI 자동 기록"
        },
        {
            time: "10:47:10",
            robot: "Robot 2",
            kind: "조치",
            axis: "J4",
            temp: "-",
            torque: "-",
            status: "정상",
            desc: "점검 요청 및 이상 징후 확인",
            actionStatus: "완료",
            cause: "J4 토크 초과",
            operatorName: "작업자 B",
            actionContent: "J4 축 동작음 및 간섭 여부 확인 완료",
            recordMode: "작업자 수동 입력"
        }
    ]
    property var historyRows: dialogRoot.hasViewModel
                              ? dialogRoot.viewModel.historyRows
                              : dialogRoot.sampleHistoryRows

    property string historyFilterType: "전체"
    property int selectedHistoryIndex: 0

    property bool actionEditMode: false
    property var actionTargetItem: null

    property string actionOperatorText: "작업자"
    property string actionContentText: ""
    property string actionMemoText: ""
    property string actionStatusText: "완료"
    // 사용자 지정날짜 입력용
    property string customFromDateText: ""
    property string customToDateText: ""
    // =============================
    function pad2(v) {
        return v < 10 ? "0" + v : "" + v
    }

    function toLocalIso(dt) {
        return dt.getFullYear() + "-"
                + pad2(dt.getMonth() + 1) + "-"
                + pad2(dt.getDate()) + "T"
                + pad2(dt.getHours()) + ":"
                + pad2(dt.getMinutes()) + ":"
                + pad2(dt.getSeconds())
    }

    function toDateText(dt) {
        return dt.getFullYear() + "-"
                + pad2(dt.getMonth() + 1) + "-"
                + pad2(dt.getDate())
    }

    function isValidDateText(text) {
        if (!/^\d{4}-\d{2}-\d{2}$/.test(text))
            return false

        var parts = text.split("-")
        var y = Number(parts[0])
        var m = Number(parts[1])
        var d = Number(parts[2])

        var date = new Date(y, m - 1, d)

        return date.getFullYear() === y
               && date.getMonth() === m - 1
               && date.getDate() === d
    }

    function customStartIso(text) {
        return text + "T00:00:00"
    }

    function customEndIso(text) {
        return text + "T23:59:59"
    }

    function historyFilterMap() {
        var filter = {}

        if (historyRobotCombo.currentText === "Robot 1")
            filter.robotId = 1
        else if (historyRobotCombo.currentText === "Robot 2")
            filter.robotId = 2
        else
            filter.robotId = 0

        filter.kind = dialogRoot.historyFilterType
        filter.searchText = historySearchField.text
        filter.limit = 500

        var now = new Date()
        var from = new Date(now)

        if (historyPeriodCombo.currentText === "오늘") {
            from.setHours(0, 0, 0, 0)
            filter.from = toLocalIso(from)
            filter.to = toLocalIso(now)
        } else if (historyPeriodCombo.currentText === "최근 7일") {
            from.setDate(now.getDate() - 7)
            filter.from = toLocalIso(from)
            filter.to = toLocalIso(now)
        } else if (historyPeriodCombo.currentText === "최근 30일") {
            from.setDate(now.getDate() - 30)
            filter.from = toLocalIso(from)
            filter.to = toLocalIso(now)
        } else if (historyPeriodCombo.currentText === "최근 90일") {
            from.setDate(now.getDate() - 90)
            filter.from = toLocalIso(from)
            filter.to = toLocalIso(now)
        } else if (historyPeriodCombo.currentText === "사용자 지정") {
            if (dialogRoot.isValidDateText(dialogRoot.customFromDateText) &&
                dialogRoot.isValidDateText(dialogRoot.customToDateText)) {
                filter.from = dialogRoot.customStartIso(dialogRoot.customFromDateText)
                filter.to = dialogRoot.customEndIso(dialogRoot.customToDateText)
            }
        }

        return filter
    }

    function requestHistoryQuery() {
        dialogRoot.selectedHistoryIndex = 0

        if (!dialogRoot.hasViewModel || !dialogRoot.viewModel.queryHistory)
            return

        if (!dialogRoot.viewModel.queryHistory(dialogRoot.historyFilterMap())) {
            console.warn("[QML] queryHistory failed")
        }
    }

    function displayDateTime(item) {
        if (!item)
            return "-"

        var iso = ""

        if (item.sortTime !== undefined && item.sortTime !== "")
            iso = item.sortTime
        else if (item.occurredAt !== undefined && item.occurredAt !== "")
            iso = item.occurredAt
        else if (item.actionAt !== undefined && item.actionAt !== "")
            iso = item.actionAt

        if (iso === undefined || iso === "")
            return item.time !== undefined ? item.time : "-"

        if (iso.length >= 19)
            return iso.substr(0, 10) + " " + iso.substr(11, 8)

        return iso
    }

    function isAlarmItem(item) {
        return item && item.kind === "알람"
    }

    function canConfirmAlarm(item) {
        return isAlarmItem(item)
               && item.actionStatus === "요청"
    }

    function isActionItem(item) {
        return item && item.kind === "조치"
    }

    function canRegisterAction(item) {
        if (!item)
            return false

        if (isAlarmItem(item))
            return item.actionStatus !== "완료"

        if (isActionItem(item))
            return item.actionStatus === "확인중"
                   || item.actionStatus === "보류"

        return false
    }

    function filteredHistoryRows(keyword, robotFilter, typeFilter) {
        var rows = []
        var key = (keyword || "").toLowerCase()

        for (var i = 0; i < dialogRoot.historyRows.length; ++i) {
            var item = dialogRoot.historyRows[i]

            if (robotFilter && robotFilter !== "전체" && item.robot !== robotFilter)
                continue

            if (typeFilter === "알람 이력" && item.kind !== "알람")
                continue

            if (typeFilter === "조치 이력" && item.kind !== "조치")
                continue

            if (key.length > 0) {
                var target = (item.time + " " + item.robot + " " + item.kind + " " +
                              item.axis + " " + item.status + " " + item.desc + " " +
                              item.actionStatus + " " + item.cause + " " +
                              item.operatorName + " " + item.actionContent).toLowerCase()

                if (target.indexOf(key) < 0)
                    continue
            }

            rows.push(item)
        }

        return rows
    }

    function clampHistoryIndex(rows) {
        if (!rows || rows.length === 0)
            return -1

        if (dialogRoot.selectedHistoryIndex < 0)
            dialogRoot.selectedHistoryIndex = 0

        if (dialogRoot.selectedHistoryIndex >= rows.length)
            dialogRoot.selectedHistoryIndex = rows.length - 1

        return dialogRoot.selectedHistoryIndex
    }

    function historyCount(kind, actionStatus) {
        var count = 0

        for (var i = 0; i < dialogRoot.historyRows.length; ++i) {
            var item = dialogRoot.historyRows[i]

            if (kind && item.kind !== kind)
                continue

            if (actionStatus && item.actionStatus !== actionStatus)
                continue

            count++
        }

        return count
    }


    // ============================================================
    // [컴포넌트] DialogToolButton
    // 사용 위치: 다이얼로그 닫기 버튼
    // ============================================================
    component DialogToolButton: Button {
        id: toolButton

        Layout.preferredHeight: 28
        Layout.minimumHeight: 28
        Layout.maximumHeight: 28
        Layout.fillHeight: false

        font.family:    "Asta Sans"

        font.pixelSize: 12

        contentItem: Text {
            text: toolButton.text
            color: "#374151"
            font.family:    "Asta Sans"
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

    // ============================================================
    // [컴포넌트] ExportButton
    // 사용 위치:
    // - 이력 조회 다이얼로그 내보내기 버튼
    // - 임계값 저장 버튼
    // ============================================================
    component ExportButton: Button {
        id: exportButton

        property color buttonColor: "#2563eb"

        Layout.fillWidth: true
        Layout.preferredHeight: 34
        Layout.fillHeight: false

        font.family:    "Asta Sans"

        font.pixelSize: 12

        contentItem: Text {
            text: exportButton.text
            color: "#ffffff"
            font.family:    "Asta Sans"
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

    // ============================================================
    // [컴포넌트] HistoryActionCardButton
    // 사용 위치: 이력 조회 다이얼로그의 확인 / 조치 버튼
    // ============================================================

    component HistoryActionCardButton: Button {
        id: cardButton

        property color accentColor: "#2563eb"
        property string titleText: "확인"
        property string bodyText: "알람 확인"

        Layout.fillWidth: true
        Layout.preferredHeight: 54
        Layout.minimumHeight: 54
        Layout.maximumHeight: 54
        Layout.fillHeight: false

        padding: 0
        hoverEnabled: true
        opacity: enabled ? 1.0 : 0.45

        background: Rectangle {
            radius: 6
            color: !cardButton.enabled ? "#f1f5f9"
                 : cardButton.pressed ? "#eef2f7"
                 : cardButton.hovered ? "#f8fafc"
                 : "#fafafa"
            border.color: cardButton.hovered ? "#cbd5e1" : "#e0e4ea"
            border.width: 1
        }

        contentItem: Item {
            RowLayout {
                anchors.fill: parent
                anchors.margins: 6
                spacing: 6

                Rectangle {
                    Layout.preferredWidth: 6
                    Layout.fillHeight: true
                    radius: 3
                    color: cardButton.accentColor
                    opacity: 0.9
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 2

                    Text {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 16
                        Layout.fillHeight: false
                        text: cardButton.titleText
                        color: cardButton.accentColor
                        font.family: "Asta Sans"
                        font.pixelSize: 12
                        font.bold: true
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }

                    Text {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 15
                        Layout.fillHeight: false
                        text: cardButton.bodyText
                        color: "#374151"
                        font.family: "Asta Sans"
                        font.pixelSize: 10
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        wrapMode: Text.WordWrap
                        maximumLineCount: 2
                        elide: Text.ElideRight
                    }
                }
            }
        }
    }

    // ============================================================
    // [컴포넌트] HistorySegmentButton
    // 사용 위치: 이력 조회 다이얼로그의 이력 구분 선택
    // ============================================================
    component HistorySegmentButton: Button {
        id: segRoot

        property bool selected: false

        Layout.fillWidth: true
        Layout.preferredHeight: 34
        Layout.fillHeight: false

        contentItem: Text {
            text:                segRoot.text
            color:               segRoot.selected ? "white" : "#374151"
            font.family:         "Asta Sans"
            font.pixelSize:      12
            font.bold:           segRoot.selected
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment:   Text.AlignVCenter
            elide:               Text.ElideRight
        }

        background: Rectangle {
            radius: 4
            color: segRoot.selected ? "#2563eb"
                 : segRoot.hovered ? "#e2e8f0"
                 : "#f8fafc"
            border.color: "#cbd5e1"
        }
    }

    // ============================================================
    // [컴포넌트] HistorySummaryCard
    // 사용 위치: 이력 조회 다이얼로그 상단 요약 카운트
    // ============================================================
    component HistorySummaryCard: Rectangle {
        id: summaryRoot

        property string labelText: ""
        property string valueText: ""
        property color accentColor: "#2563eb"

        Layout.preferredWidth: 96
        Layout.preferredHeight: 34
        Layout.fillHeight: false

        radius: 5
        color: "#fafafa"
        border.color: "#e0e4ea"

        RowLayout {
            anchors.fill: parent
            anchors.margins: 6
            spacing: 5

            Rectangle {
                width: 8
                height: 8
                radius: 4
                color: summaryRoot.accentColor
            }

            Text {
                text: summaryRoot.labelText + " " + summaryRoot.valueText
                color: "#374151"
                font.family:    "Asta Sans"
                font.pixelSize: 12
                font.bold: true
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }
    }

    // ============================================================
    // [컴포넌트] HistoryTableRow
    // 사용 위치: 이력 조회 다이얼로그
    // 용도:
    // - 시간 / 로봇 / 구분 / 축 / 온도 / 토크 / 상태 / 설명 / 조치상태
    // - Row + 명시적 width/height 기반으로 열 깨짐 방지
    // ============================================================
    component HistoryTableRow: Rectangle {
        id: rowRoot

        property bool isHeader: false
        property bool selected: false

        property string timeText: ""
        property string robotText: ""
        property string kindText: ""
        property string axisText: ""
        property string tempText: ""
        property string torqueText: ""
        property string statusText: ""
        property string descText: ""
        property string actionStatusText: ""

        signal rowClicked()

        implicitHeight: 34
        color: rowRoot.isHeader ? "#f1f5f9"
             : rowRoot.selected ? "#eff6ff"
             : "white"
        border.color: rowRoot.selected ? "#bfdbfe" : "#e2e8f0"
        clip: true

        Row {
            anchors.fill: parent
            spacing: 0

            HistoryTableCell {
                width: 142  //88
                height: rowRoot.height
                textValue: rowRoot.timeText
                isHeader: rowRoot.isHeader
            }

            HistoryTableCell {
                width: 82
                height: rowRoot.height
                textValue: rowRoot.robotText
                isHeader: rowRoot.isHeader
            }

            HistoryKindCell {
                width: 68
                height: rowRoot.height
                kindText: rowRoot.kindText
                isHeader: rowRoot.isHeader
            }

            HistoryTableCell {
                width: 48
                height: rowRoot.height
                textValue: rowRoot.axisText
                isHeader: rowRoot.isHeader
            }

            HistoryTableCell {
                width: 62
                height: rowRoot.height
                textValue: rowRoot.tempText
                isHeader: rowRoot.isHeader
            }

            HistoryTableCell {
                width: 62
                height: rowRoot.height
                textValue: rowRoot.torqueText
                isHeader: rowRoot.isHeader
            }

            HistoryStatusCell {
                width: 72
                height: rowRoot.height
                statusText: rowRoot.statusText
                isHeader: rowRoot.isHeader
            }

            HistoryTableCell {
                //width: Math.max(120, rowRoot.width - 88 - 82 - 68 - 48 - 62 - 62 - 72 - 78)
                width: Math.max(120, rowRoot.width - 142 - 82 - 68 - 48 - 62 - 62 - 72 - 78)
                height: rowRoot.height
                textValue: rowRoot.descText
                isHeader: rowRoot.isHeader
            }

            HistoryActionCell {
                width: 78
                height: rowRoot.height
                actionText: rowRoot.actionStatusText
                isHeader: rowRoot.isHeader
            }
        }

        MouseArea {
            anchors.fill: parent
            enabled: !rowRoot.isHeader
            hoverEnabled: true
            onClicked: rowRoot.rowClicked()
        }
    }

    // ============================================================
    // [컴포넌트] HistoryTableCell
    // ============================================================
    component HistoryTableCell: Rectangle {
        id: cellRoot

        property string textValue: ""
        property bool isHeader: false

        color: "transparent"
        border.color: "#e2e8f0"
        clip: true

        Text {
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8

            text: cellRoot.textValue
            color: cellRoot.isHeader ? "#334155" : "#475569"
            font.family:    "Asta Sans"
            font.pixelSize: 11
            font.bold: cellRoot.isHeader
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignLeft
            elide: Text.ElideRight
        }
    }

    // ============================================================
    // [컴포넌트] HistoryKindCell
    // ============================================================
    component HistoryKindCell: Rectangle {
        id: kindRoot

        property string kindText: ""
        property bool isHeader: false

        color: "transparent"
        border.color: "#e2e8f0"
        clip: true

        Text {
            visible: kindRoot.isHeader
            anchors.fill: parent
            anchors.leftMargin: 8
            text: kindRoot.kindText
            color: "#374151"
            font.family:    "Asta Sans"
            font.pixelSize: 11
            font.bold: true
            verticalAlignment: Text.AlignVCenter
        }

        Rectangle {
            visible: !kindRoot.isHeader
            anchors.centerIn: parent
            width: 42
            height: 20
            radius: 4
            color: kindRoot.kindText === "알람" ? "#fee2e2" : "#dbeafe"
            border.color: kindRoot.kindText === "알람" ? "#fecaca" : "#bfdbfe"

            Text {
                anchors.centerIn: parent
                text: kindRoot.kindText
                color: kindRoot.kindText === "알람" ? "#dc2626" : "#2563eb"
                font.family:    "Asta Sans"
                font.pixelSize: 11
                font.bold: true
            }
        }
    }

    // ============================================================
    // [컴포넌트] HistoryStatusCell
    // ============================================================
    component HistoryStatusCell: Rectangle {
        id: statusRoot

        property string statusText: ""
        property bool isHeader: false

        color: "transparent"
        border.color: "#e2e8f0"
        clip: true

        Text {
            visible: statusRoot.isHeader
            anchors.fill: parent
            anchors.leftMargin: 8
            text: statusRoot.statusText
            color: "#374151"
            font.family:    "Asta Sans"
            font.pixelSize: 11
            font.bold: true
            verticalAlignment: Text.AlignVCenter
        }

        Rectangle {
            visible: !statusRoot.isHeader
            anchors.centerIn: parent
            width: 44
            height: 20
            radius: 4
            color: statusRoot.statusText === "경고" ? "#fee2e2"
                 : statusRoot.statusText === "주의" ? "#ffedd5"
                 : statusRoot.statusText === "조치" ? "#dbeafe"
                 : "#dcfce7"

            Text {
                anchors.centerIn: parent
                text: statusRoot.statusText
                color: statusRoot.statusText === "경고" ? "#dc2626"
                     : statusRoot.statusText === "주의" ? "#f97316"
                     : statusRoot.statusText === "조치" ? "#2563eb"
                     : "#15803d"
                font.family: "Asta Sans"
                font.pixelSize: 11
                font.bold: true
            }
        }
    }

    // ============================================================
    // [컴포넌트] HistoryActionCell
    // ============================================================
    component HistoryActionCell: Rectangle {
        id: actionRoot

        property string actionText: ""
        property bool isHeader: false

        color: "transparent"
        border.color: "#e2e8f0"
        clip: true

        Text {
            visible: actionRoot.isHeader
            anchors.fill: parent
            anchors.leftMargin: 8
            text: actionRoot.actionText
            color: "#374151"
            font.family:    "Asta Sans"
            font.pixelSize: 11
            font.bold: true
            verticalAlignment: Text.AlignVCenter
        }

        Rectangle {
            visible: !actionRoot.isHeader
            anchors.centerIn: parent
            width: 48
            height: 20
            radius: 4
            color: actionRoot.actionText === "완료" ? "#dcfce7"
                 : actionRoot.actionText === "확인중" ? "#dbeafe"
                 : actionRoot.actionText === "보류" ? "#fef3c7"
                 : actionRoot.actionText === "요청" ? "#ffedd5"
                 : "#f1f5f9"

            Text {
                anchors.centerIn: parent
                text: actionRoot.actionText
                color: actionRoot.actionText === "완료" ? "#16a34a"
                     : actionRoot.actionText === "확인중" ? "#2563eb"
                     : actionRoot.actionText === "보류" ? "#d97706"
                     : actionRoot.actionText === "요청" ? "#f97316"
                     : "#64748b"
                font.family: "Asta Sans"
                font.pixelSize: 11
                font.bold: true
            }
        }
    }

    // ============================================================
    // [컴포넌트] HistoryDetailItem
    // 사용 위치: 선택 이력 상세 패널
    // ============================================================
    component HistoryDetailItem: RowLayout {
        id: detailRoot

        property string labelText: ""
        property string valueText: ""
        property color  valueColor: "#374151"

        Layout.fillWidth: true
        Layout.preferredHeight: 24

        Text {
            Layout.preferredWidth: 82
            text:           detailRoot.labelText
            font.family:    "Asta Sans"
            font.pixelSize: 12
            font.bold:      true
            color:          "#9e9e9e"
            elide:          Text.ElideRight
        }

        Text {
            Layout.fillWidth: true
            text:           detailRoot.valueText
            font.family:    "Asta Sans"
            font.pixelSize: 12
            font.bold:      detailRoot.valueColor !== "#374151"
            color:          detailRoot.valueColor
            elide:          Text.ElideRight
        }
    }

    // ============================================================
    // [컴포넌트] HistoryModeCard
    // 사용 위치: 선택 이력 상세 패널 하단
    //
    // 디자인 의도:
    // - 이전 시안과 동일하게 '카드형 안내 영역'으로 표시
    // - 텍스트만 떠 보이지 않도록 높이/배경/테두리를 명시
    // ============================================================
    component HistoryModeCard: Rectangle {
        id: modeRoot

        property string titleText: ""
        property string bodyText: ""
        property color accentColor: "#2563eb"

        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.preferredHeight: 54
        Layout.minimumHeight: 0

        radius: 6
        color: "#fafafa"
        border.color: "#e0e4ea"
        clip: true

        RowLayout {
            anchors.fill: parent
            anchors.margins: 6
            spacing: 6

            Rectangle {
                Layout.preferredWidth: 6
                Layout.fillHeight: true
                radius: 3
                color: modeRoot.accentColor
                opacity: 0.9
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 2

                Text {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 16
                    Layout.fillHeight: false
                    text: modeRoot.titleText
                    color: modeRoot.accentColor
                    font.family:    "Asta Sans"
                    font.pixelSize: 12
                    font.bold: true
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }

                Text {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 15
                    Layout.fillHeight: false
                    text: modeRoot.bodyText
                    color: "#374151"
                    font.family:    "Asta Sans"
                    font.pixelSize: 10
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    wrapMode: Text.WordWrap
                    maximumLineCount: 2
                    elide: Text.ElideRight
                }
            }
        }
    }

}


