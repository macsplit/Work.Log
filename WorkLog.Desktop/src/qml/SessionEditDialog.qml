import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15
import Qt.labs.calendar 1.0
import org.kde.kirigami 2.19 as Kirigami
import org.worklog 1.0

QQC2.Dialog {
    id: root

    property int sessionId: -1
    property date sessionDate: new Date()
    property real timeHours: 1.0
    property string description: ""
    property string notes: ""
    property string nextPlannedStage: ""
    property int tagId: -1

    title: sessionId < 0 ? i18n("New Work Session") : i18n("Edit Work Session")
    modal: true
    standardButtons: QQC2.Dialog.Save | QQC2.Dialog.Cancel
    width: Math.min(parent.width - Kirigami.Units.largeSpacing * 4, Kirigami.Units.gridUnit * 35)
    anchors.centerIn: parent
    topPadding: Kirigami.Units.smallSpacing
    bottomPadding: Kirigami.Units.gridUnit

    onTagIdChanged: tagComboBox.refreshSelection()

    onSessionDateChanged: dateField.applyDate(sessionDate)
    onTimeHoursChanged: hoursSpinBox.value = Math.round(timeHours * 2)
    onDescriptionChanged: descriptionField.text = description
    onNotesChanged: notesField.text = notes
    onNextPlannedStageChanged: nextStageField.text = nextPlannedStage

    onOpened: descriptionField.forceActiveFocus()

    onAccepted: {
        sessionDate = dateField.selectedDate
        timeHours = hoursSpinBox.value / 2.0
        description = descriptionField.text
        notes = notesField.text
        nextPlannedStage = nextStageField.text
        tagId = tagComboBox.currentIndex >= 0 ? TagModel.getIdByIndex(tagComboBox.currentIndex) : -1
    }

    contentItem: QQC2.ScrollView {
        clip: true
        implicitHeight: Kirigami.Units.gridUnit * 16

        contentItem: Flickable {
            id: formFlick
            clip: true
            contentWidth: Math.max(formLayout.implicitWidth, width)
            contentHeight: formLayout.implicitHeight
            boundsBehavior: Flickable.StopAtBounds

            Kirigami.FormLayout {
                id: formLayout
                width: formFlick.width

                // Date picker (text field with calendar button + quick-adjust buttons)
                RowLayout {
                    Kirigami.FormData.label: i18n("Date:")
                    spacing: Kirigami.Units.smallSpacing

                    QQC2.TextField {
                        id: dateField
                        Layout.fillWidth: true
                        property date selectedDate: root.sessionDate
                        inputMask: "9999-99-99"
                        function applyDate(newDate) {
                            if (!newDate || isNaN(newDate.getTime())) {
                                return
                            }
                            selectedDate = newDate
                            text = Qt.formatDate(selectedDate, "yyyy-MM-dd")
                        }
                        onTextChanged: {
                            var parsed = Date.fromLocaleString(Qt.locale(), text, "yyyy-MM-dd")
                            if (!isNaN(parsed.getTime())) {
                                selectedDate = parsed
                            }
                        }
                        Component.onCompleted: {
                            applyDate(root.sessionDate)
                        }
                    }

                    QQC2.ToolButton {
                        id: calendarButton
                        icon.name: "view-calendar"
                        icon.width: Kirigami.Units.iconSizes.small
                        icon.height: Kirigami.Units.iconSizes.small
                        display: QQC2.AbstractButton.IconOnly
                        QQC2.ToolTip.text: i18n("Open calendar")
                        QQC2.ToolTip.visible: hovered
                        onClicked: calendarDialog.open()
                    }

                    Item { width: Kirigami.Units.smallSpacing }  // Spacer

                    QQC2.ToolButton {
                        icon.name: "go-previous"
                        icon.width: Kirigami.Units.iconSizes.small
                        icon.height: Kirigami.Units.iconSizes.small
                        display: QQC2.AbstractButton.IconOnly
                        QQC2.ToolTip.text: i18n("Previous day")
                        QQC2.ToolTip.visible: hovered
                        onClicked: {
                            var d = new Date(dateField.selectedDate)
                            if (isNaN(d.getTime())) d = new Date()
                            d.setDate(d.getDate() - 1)
                            dateField.applyDate(d)
                        }
                    }

                    QQC2.ToolButton {
                        icon.name: "choice-rhomb"
                        icon.width: Kirigami.Units.iconSizes.small
                        icon.height: Kirigami.Units.iconSizes.small
                        display: QQC2.AbstractButton.IconOnly
                        QQC2.ToolTip.text: i18n("Today")
                        QQC2.ToolTip.visible: hovered
                        onClicked: dateField.applyDate(new Date())
                    }

                    QQC2.ToolButton {
                        icon.name: "go-next"
                        icon.width: Kirigami.Units.iconSizes.small
                        icon.height: Kirigami.Units.iconSizes.small
                        display: QQC2.AbstractButton.IconOnly
                        QQC2.ToolTip.text: i18n("Next day")
                        QQC2.ToolTip.visible: hovered
                        onClicked: {
                            var d = new Date(dateField.selectedDate)
                            if (isNaN(d.getTime())) d = new Date()
                            d.setDate(d.getDate() + 1)
                            dateField.applyDate(d)
                        }
                    }
                }

                // Time in hours
                RowLayout {
                    Kirigami.FormData.label: i18n("Time (hours):")

                    QQC2.SpinBox {
                        id: hoursSpinBox
                        from: 1  // 0.5 hours
                        to: 48   // 24 hours
                        stepSize: 1  // 0.5 hour steps
                        value: root.timeHours * 2

                        textFromValue: function(value, locale) {
                            return (value / 2.0).toFixed(1)
                        }

                        valueFromText: function(text, locale) {
                            return Math.round(parseFloat(text) * 2)
                        }

                        Component.onCompleted: {
                            value = root.timeHours * 2
                        }
                    }

                    QQC2.Label {
                        text: i18n("(0.5 hour increments)")
                        opacity: 0.7
                    }
                }

                // Tag selection
                RowLayout {
                    Kirigami.FormData.label: i18n("Tag (optional):")

                    QQC2.ComboBox {
                        id: tagComboBox
                        Layout.fillWidth: true
                        model: TagModel
                        textRole: "tagName"

                        function refreshSelection() {
                            currentIndex = TagModel.getIndexById(root.tagId)
                        }

                        displayText: currentIndex >= 0 ? currentText : i18n("No tag")

                        Component.onCompleted: refreshSelection()

                        // Allow clearing the selection
                        onActivated: {
                            if (index < 0) {
                                currentIndex = -1
                            }
                        }
                    }

                    QQC2.Button {
                        icon.name: "tag-edit"
                        text: i18n("Clear")
                        visible: tagComboBox.currentIndex >= 0
                        onClicked: tagComboBox.currentIndex = -1
                    }

                    QQC2.Button {
                        icon.name: "tag-new"
                        QQC2.ToolTip.text: i18n("Manage Tags")
                        QQC2.ToolTip.visible: hovered
                        onClicked: tagManagementDialog.open()
                    }
                }

                // Description
                QQC2.TextField {
                    id: descriptionField
                    Kirigami.FormData.label: i18n("Description:")
                    Layout.fillWidth: true
                    text: root.description
                    placeholderText: i18n("Describe the work you did...")
                }

                // Notes
                QQC2.ScrollView {
                    Kirigami.FormData.label: i18n("Notes (optional):")
                    Layout.fillWidth: true
                    implicitHeight: Kirigami.Units.gridUnit * 4

                    QQC2.TextArea {
                        id: notesField
                        text: root.notes
                        placeholderText: i18n("Additional notes...")
                        wrapMode: TextEdit.Wrap
                        Keys.onTabPressed: nextStageField.forceActiveFocus()
                        Keys.onBacktabPressed: descriptionField.forceActiveFocus()
                    }
                }

                // Next planned stage
                QQC2.ScrollView {
                    Kirigami.FormData.label: i18n("Next Planned Stage (optional):")
                    Layout.fillWidth: true
                    implicitHeight: Kirigami.Units.gridUnit * 2.5

                    QQC2.TextArea {
                        id: nextStageField
                        text: root.nextPlannedStage
                        placeholderText: i18n("What's planned next...")
                        wrapMode: TextEdit.Wrap
                        Keys.onTabPressed: dateField.forceActiveFocus()
                        Keys.onBacktabPressed: notesField.forceActiveFocus()
                    }
                }
            }
        }
    }

    TagManagementDialog {
        id: tagManagementDialog
    }

    QQC2.Dialog {
        id: calendarDialog
        modal: true
        anchors.centerIn: parent
        width: Kirigami.Units.gridUnit * 22
        height: Kirigami.Units.gridUnit * 16

        property int displayMonth: 0
        property int displayYear: 2025

        onOpened: {
            displayMonth = dateField.selectedDate.getMonth()
            displayYear = dateField.selectedDate.getFullYear()
        }

        contentItem: ColumnLayout {
            spacing: Kirigami.Units.smallSpacing
            implicitHeight: Kirigami.Units.gridUnit * 14

            // Month/Year navigation
            RowLayout {
                Layout.fillWidth: true

                QQC2.ToolButton {
                    icon.name: "go-previous"
                    onClicked: {
                        if (calendarDialog.displayMonth === 0) {
                            calendarDialog.displayMonth = 11
                            calendarDialog.displayYear--
                        } else {
                            calendarDialog.displayMonth--
                        }
                    }
                }

                QQC2.Label {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    font.bold: true
                    text: Qt.locale().monthName(calendarDialog.displayMonth) + " " + calendarDialog.displayYear
                }

                QQC2.ToolButton {
                    icon.name: "go-next"
                    onClicked: {
                        if (calendarDialog.displayMonth === 11) {
                            calendarDialog.displayMonth = 0
                            calendarDialog.displayYear++
                        } else {
                            calendarDialog.displayMonth++
                        }
                    }
                }
            }

            // Day of week headers
            DayOfWeekRow {
                Layout.fillWidth: true
                Layout.preferredWidth: Kirigami.Units.gridUnit * 18
                locale: Qt.locale()
                delegate: QQC2.Label {
                    text: model.shortName
                    horizontalAlignment: Text.AlignHCenter
                    opacity: 0.6
                }
            }

            // Month grid
            MonthGrid {
                id: monthGrid
                Layout.fillWidth: true
                Layout.preferredWidth: Kirigami.Units.gridUnit * 18
                Layout.preferredHeight: Kirigami.Units.gridUnit * 10
                month: calendarDialog.displayMonth
                year: calendarDialog.displayYear
                locale: Qt.locale()

                delegate: QQC2.AbstractButton {
                    id: dayDelegate
                    implicitWidth: monthGrid.width / 7
                    implicitHeight: monthGrid.height / 6

                    readonly property bool isSelected:
                        model.day === dateField.selectedDate.getDate() &&
                        model.month === dateField.selectedDate.getMonth() &&
                        model.year === dateField.selectedDate.getFullYear()
                    readonly property bool isCurrentMonth: model.month === monthGrid.month

                    contentItem: QQC2.Label {
                        text: model.day
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        opacity: isCurrentMonth ? 1.0 : 0.3
                        font.bold: isSelected
                        color: isSelected ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor
                    }

                    background: Rectangle {
                        visible: isSelected
                        color: Kirigami.Theme.highlightColor
                        radius: width / 2
                    }

                    onClicked: {
                        dateField.applyDate(model.date)
                        calendarDialog.close()
                    }
                }
            }

            // Bottom spacer
            Item { Layout.fillHeight: true }
        }
    }

    Connections {
        target: TagModel
        function onCountChanged() {
            tagComboBox.refreshSelection()
        }
    }
}
