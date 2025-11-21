import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.19 as Kirigami
import org.worklog 1.0

QQC2.Dialog {
    id: root

    title: i18n("Manage Tags")
    modal: true
    standardButtons: QQC2.Dialog.Close
    width: Math.min(parent.width - Kirigami.Units.largeSpacing * 4, Kirigami.Units.gridUnit * 25)
    anchors.centerIn: parent

    contentItem: ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        // New tag input
        RowLayout {
            Layout.fillWidth: true

            QQC2.TextField {
                id: newTagField
                Layout.fillWidth: true
                placeholderText: i18n("Enter new tag name...")
                onAccepted: addTagButton.clicked()
            }

            QQC2.Button {
                id: addTagButton
                icon.name: "list-add"
                text: i18n("Add")
                enabled: newTagField.text.trim().length > 0
                onClicked: {
                    var tagName = newTagField.text.trim()
                    if (tagName.length > 0) {
                        var result = Database.createTag(tagName)
                        if (result > 0) {
                            newTagField.text = ""
                        }
                    }
                }
            }
        }

        // Tag list
        QQC2.ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: Kirigami.Units.gridUnit * 12

            ListView {
                id: tagListView
                clip: true
                model: TagModel

                delegate: Kirigami.SwipeListItem {
                    width: ListView.view.width

                    contentItem: QQC2.Label {
                        text: model.tagName
                        elide: Text.ElideRight
                    }

                    actions: [
                        Kirigami.Action {
                            icon.name: "edit-delete"
                            text: i18n("Delete")
                            onTriggered: {
                                deleteConfirmDialog.tagId = model.tagId
                                deleteConfirmDialog.tagName = model.tagName
                                deleteConfirmDialog.open()
                            }
                        }
                    ]
                }

                QQC2.Label {
                    anchors.centerIn: parent
                    visible: tagListView.count === 0
                    text: i18n("No tags yet. Add one above.")
                    opacity: 0.7
                }
            }
        }
    }

    // Delete confirmation dialog
    QQC2.Dialog {
        id: deleteConfirmDialog

        property int tagId: -1
        property string tagName: ""

        title: i18n("Delete Tag")
        modal: true
        standardButtons: QQC2.Dialog.Yes | QQC2.Dialog.No
        anchors.centerIn: parent

        contentItem: QQC2.Label {
            text: i18n("Are you sure you want to delete the tag \"%1\"?\n\nSessions using this tag will have their tag removed.", deleteConfirmDialog.tagName)
            wrapMode: Text.Wrap
        }

        onAccepted: {
            Database.deleteTag(tagId)
        }
    }
}
