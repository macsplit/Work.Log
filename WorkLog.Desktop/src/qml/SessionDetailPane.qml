import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.19 as Kirigami

Kirigami.Page {
    id: root

    property var session: null

    signal editRequested()
    signal deleteRequested()

    title: i18n("Session Details")
    leftPadding: Kirigami.Units.smallSpacing
    rightPadding: Kirigami.Units.smallSpacing
    topPadding: Kirigami.Units.smallSpacing
    bottomPadding: Kirigami.Units.smallSpacing

    actions.main: Kirigami.Action {
        icon.name: "document-edit"
        text: i18n("Edit")
        enabled: root.session !== null
        onTriggered: root.editRequested()
    }

    actions.right: Kirigami.Action {
        icon.name: "edit-delete"
        text: i18n("Delete")
        enabled: root.session !== null
        onTriggered: root.deleteRequested()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Kirigami.Units.smallSpacing
        visible: root.session !== null

        // Time header
        Kirigami.Heading {
            level: 2
            text: root.session ? root.session.timeHours + " " + i18np("hour", "hours", root.session.timeHours) : ""
            color: Kirigami.Theme.highlightColor
        }

        // Date
        RowLayout {
            Kirigami.Icon {
                source: "view-calendar-day"
                implicitWidth: Kirigami.Units.iconSizes.small
                implicitHeight: Kirigami.Units.iconSizes.small
            }
            QQC2.Label {
                text: root.session ? Qt.formatDate(root.session.date, "dddd, MMMM d, yyyy") : ""
            }
        }

        // Tag
        RowLayout {
            visible: root.session && root.session.tagName && root.session.tagName.length > 0

            Kirigami.Icon {
                source: "tag"
                implicitWidth: Kirigami.Units.iconSizes.small
                implicitHeight: Kirigami.Units.iconSizes.small
            }

            Rectangle {
                implicitWidth: sessionTagLabel.implicitWidth + Kirigami.Units.smallSpacing * 2
                implicitHeight: sessionTagLabel.implicitHeight + Kirigami.Units.smallSpacing
                radius: height / 2
                color: Kirigami.Theme.highlightColor
                opacity: 0.6

                QQC2.Label {
                    id: sessionTagLabel
                    anchors.centerIn: parent
                    text: root.session ? (root.session.tagName || "") : ""
                    color: Kirigami.Theme.textColor
                    font.bold: true
                }
            }
        }

        Kirigami.Separator {
            Layout.fillWidth: true
        }

        // Description section
        Kirigami.Heading {
            level: 4
            text: i18n("Description")
        }

        QQC2.Label {
            Layout.fillWidth: true
            text: root.session ? root.session.description : ""
            wrapMode: Text.Wrap
        }

        // Notes section
        Kirigami.Heading {
            level: 4
            text: i18n("Notes")
            visible: root.session && root.session.notes && root.session.notes.length > 0
        }

        QQC2.Label {
            Layout.fillWidth: true
            visible: root.session && root.session.notes && root.session.notes.length > 0
            text: root.session ? (root.session.notes || "") : ""
            wrapMode: Text.Wrap
            opacity: 0.8
        }

        // Next planned stage section
        Kirigami.Heading {
            level: 4
            text: i18n("Next Planned Stage")
            visible: root.session && root.session.nextPlannedStage && root.session.nextPlannedStage.length > 0
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: root.session && root.session.nextPlannedStage && root.session.nextPlannedStage.length > 0
            type: Kirigami.MessageType.Information
            text: root.session ? (root.session.nextPlannedStage || "") : ""
        }

        Item {
            Layout.fillHeight: true
        }
    }

    // Placeholder when no session selected
    ColumnLayout {
        anchors.centerIn: parent
        visible: root.session === null
        spacing: Kirigami.Units.smallSpacing
        width: Math.min(parent.width * 0.8, Kirigami.Units.gridUnit * 18)

        Kirigami.Icon {
            source: "document-preview"
            Layout.alignment: Qt.AlignHCenter
            implicitWidth: Kirigami.Units.iconSizes.large
            implicitHeight: Kirigami.Units.iconSizes.large
        }

        QQC2.Label {
            Layout.alignment: Qt.AlignHCenter
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * 0.9
            text: i18n("No session selected")
        }

        QQC2.Label {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * 0.85
            text: i18n("Select a session from the list to view its details.")
        }
    }
}
