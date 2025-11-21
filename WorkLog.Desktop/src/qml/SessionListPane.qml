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

    title: Qt.formatDate(currentDate, "dddd, MMMM d, yyyy")
    leftPadding: Kirigami.Units.smallSpacing
    rightPadding: Kirigami.Units.smallSpacing

    header: ColumnLayout {
        spacing: 0

        Kirigami.Heading {
            Layout.margins: Kirigami.Units.smallSpacing
            level: 4
            text: i18n("Work Sessions")
        }

        Kirigami.Separator {
            Layout.fillWidth: true
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

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            visible: SessionModel.count === 0
            text: i18n("No sessions for this day")
            explanation: i18n("Click the + button to add a work session")
            icon.name: "appointment-new"
        }
    }

    footer: ColumnLayout {
        spacing: 0

        Kirigami.Separator {
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.margins: Kirigami.Units.smallSpacing

            QQC2.Label {
                text: i18n("Total: %1 hours", calculateTotal())
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

            Item { Layout.fillWidth: true }

            QQC2.Label {
                text: i18np("%1 session", "%1 sessions", SessionModel.count)
                opacity: 0.7
            }
        }
    }
}
