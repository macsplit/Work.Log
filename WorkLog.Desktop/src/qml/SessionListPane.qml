import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.19 as Kirigami
import org.worklog 1.0

Kirigami.ScrollablePage {
    id: root

    property date currentDate: new Date()
    property int selectedIndex: -1

    signal sessionSelected(var session)
    signal editSession(var session)
    signal deleteSession(var session)

    title: Qt.formatDate(currentDate, "dddd, MMMM d, yyyy")
    leftPadding: Kirigami.Units.smallSpacing
    rightPadding: Kirigami.Units.smallSpacing

    header: Column {
        width: parent.width
        spacing: 0

        Kirigami.Heading {
            leftPadding: Kirigami.Units.smallSpacing
            rightPadding: Kirigami.Units.smallSpacing
            topPadding: Kirigami.Units.smallSpacing
            bottomPadding: Kirigami.Units.smallSpacing
            level: 4
            text: i18n("Work Sessions")
        }

        Kirigami.Separator {
            width: parent.width
        }
    }

    ListView {
        id: sessionList
        model: SessionModel
        currentIndex: root.selectedIndex

        delegate: QQC2.ItemDelegate {
            id: sessionDelegate
            width: sessionList.width
            highlighted: sessionList.currentIndex === index

            contentItem: ColumnLayout {
                spacing: Kirigami.Units.smallSpacing

                RowLayout {
                    Layout.fillWidth: true

                    QQC2.Label {
                        text: model.timeHours + "h"
                        font.bold: true
                        // Use contrasting color when item is highlighted
                        color: sessionDelegate.highlighted ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.highlightColor
                    }

                    // Tag chip
                    Rectangle {
                        visible: model.tagName && model.tagName.length > 0
                        implicitWidth: tagLabel.implicitWidth + Kirigami.Units.smallSpacing * 2
                        implicitHeight: tagLabel.implicitHeight + Kirigami.Units.smallSpacing
                        radius: height / 2
                        color: "transparent"
                        border.width: 1
                        border.color: sessionDelegate.highlighted ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.highlightColor

                        QQC2.Label {
                            id: tagLabel
                            anchors.centerIn: parent
                            text: model.tagName || ""
                            font.pixelSize: Kirigami.Theme.smallFont.pixelSize
                            font.bold: true
                            color: sessionDelegate.highlighted ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.highlightColor
                        }
                    }

                    Item { Layout.fillWidth: true }

                    QQC2.ToolButton {
                        icon.name: "document-edit"
                        display: QQC2.AbstractButton.IconOnly
                        QQC2.ToolTip.text: i18n("Edit")
                        QQC2.ToolTip.visible: hovered
                        onClicked: {
                            var session = SessionModel.get(index)
                            root.editSession(session)
                        }
                    }

                    QQC2.ToolButton {
                        icon.name: "edit-delete"
                        display: QQC2.AbstractButton.IconOnly
                        QQC2.ToolTip.text: i18n("Delete")
                        QQC2.ToolTip.visible: hovered
                        onClicked: {
                            var session = SessionModel.get(index)
                            root.deleteSession(session)
                        }
                    }
                }

                QQC2.Label {
                    Layout.fillWidth: true
                    text: model.description
                    wrapMode: Text.Wrap
                    elide: Text.ElideRight
                    maximumLineCount: 2
                }

                QQC2.Label {
                    Layout.fillWidth: true
                    visible: model.notes && model.notes.length > 0
                    text: model.notes || ""
                    opacity: 0.7
                    font.italic: true
                    wrapMode: Text.Wrap
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }
            }

            onClicked: {
                root.selectedIndex = index
                var session = SessionModel.get(index)
                root.sessionSelected(session)
            }
        }

        ColumnLayout {
            anchors.centerIn: parent
            visible: SessionModel.count === 0
            spacing: Kirigami.Units.smallSpacing
            width: Math.min(parent.width * 0.8, Kirigami.Units.gridUnit * 18)

            Kirigami.Icon {
                source: "appointment-new"
                Layout.alignment: Qt.AlignHCenter
                implicitWidth: Kirigami.Units.iconSizes.large
                implicitHeight: Kirigami.Units.iconSizes.large
            }

            QQC2.Label {
                Layout.alignment: Qt.AlignHCenter
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * 0.9
                text: i18n("No sessions for this day")
            }

            QQC2.Label {
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * 0.85
                text: i18n("Click the + button to add a work session")
                opacity: 0.7
            }
        }
    }

    footer: Column {
        width: parent.width
        spacing: 0

        Kirigami.Separator {
            width: parent.width
        }

        // Day total row
        Item {
            width: parent.width
            height: footerRow.height + Kirigami.Units.smallSpacing * 2

            Row {
                id: footerRow
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: Kirigami.Units.smallSpacing
                spacing: Kirigami.Units.smallSpacing

                QQC2.Label {
                    text: i18n("Day: %1 hours", calculateTotal())
                    font.bold: true

                    function calculateTotal() {
                        var total = 0
                        for (var i = 0; i < SessionModel.count; i++) {
                            var session = SessionModel.get(i)
                            total += session.timeHours
                        }
                        return total
                    }
                }
            }

            QQC2.Label {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: Kirigami.Units.smallSpacing
                text: i18np("%1 session", "%1 sessions", SessionModel.count)
                opacity: 0.7
            }
        }

        // Week tag totals strip (only visible when week is selected)
        Column {
            width: parent.width
            visible: HierarchyModel.selectedWeek >= 0

            Kirigami.Separator {
                width: parent.width
            }

            Flow {
                width: parent.width
                padding: Kirigami.Units.smallSpacing
                spacing: Kirigami.Units.smallSpacing

                QQC2.Label {
                    text: i18n("Week:")
                    font.bold: true
                }

                Repeater {
                    // Refresh when hierarchy changes
                    model: (HierarchyModel.refreshCounter, HierarchyModel.selectedWeek >= 0)
                           ? HierarchyModel.getTagTotalsForSelectedWeek() : []

                    Rectangle {
                        implicitWidth: tagTotalLabel.implicitWidth + Kirigami.Units.smallSpacing * 2
                        implicitHeight: tagTotalLabel.implicitHeight + Kirigami.Units.smallSpacing
                        radius: height / 2
                        color: "transparent"
                        border.width: 1
                        border.color: Kirigami.Theme.disabledTextColor

                        QQC2.Label {
                            id: tagTotalLabel
                            anchors.centerIn: parent
                            text: modelData.tagName + " (" + modelData.totalHours.toFixed(1) + "h)"
                            font.pixelSize: Kirigami.Theme.smallFont.pixelSize
                        }
                    }
                }
            }
        }
    }
}
