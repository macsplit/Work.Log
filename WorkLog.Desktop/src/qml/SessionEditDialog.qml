import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15
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
    width: Math.min(parent.width - Kirigami.Units.largeSpacing * 4, Kirigami.Units.gridUnit * 30)
    anchors.centerIn: parent

    onOpened: {
        descriptionField.forceActiveFocus()
    }

    onAccepted: {
        sessionDate = dateField.selectedDate
        timeHours = hoursSpinBox.value / 2.0
        description = descriptionField.text
        notes = notesField.text
        nextPlannedStage = nextStageField.text
        tagId = tagComboBox.currentIndex >= 0 ? TagModel.getIdByIndex(tagComboBox.currentIndex) : -1
    }

    contentItem: Kirigami.FormLayout {
        // Date picker
        RowLayout {
            Kirigami.FormData.label: i18n("Date:")

            QQC2.TextField {
                id: dateField
                property date selectedDate: root.sessionDate
                text: Qt.formatDate(selectedDate, "yyyy-MM-dd")
                inputMask: "9999-99-99"
                onTextChanged: {
                    var parsed = Date.fromLocaleString(Qt.locale(), text, "yyyy-MM-dd")
                    if (!isNaN(parsed.getTime())) {
                        selectedDate = parsed
                    }
                }
                Component.onCompleted: {
                    selectedDate = root.sessionDate
                }
            }

            QQC2.Button {
                icon.name: "view-calendar"
                QQC2.ToolTip.text: i18n("Today")
                QQC2.ToolTip.visible: hovered
                onClicked: {
                    dateField.selectedDate = new Date()
                    dateField.text = Qt.formatDate(dateField.selectedDate, "yyyy-MM-dd")
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
                currentIndex: TagModel.getIndexById(root.tagId)

                displayText: currentIndex >= 0 ? currentText : i18n("No tag")

                Component.onCompleted: {
                    currentIndex = TagModel.getIndexById(root.tagId)
                }

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
            implicitHeight: Kirigami.Units.gridUnit * 3

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

    TagManagementDialog {
        id: tagManagementDialog
    }
}
