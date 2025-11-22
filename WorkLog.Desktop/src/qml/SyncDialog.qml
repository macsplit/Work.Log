import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.19 as Kirigami
import org.worklog 1.0

QQC2.Dialog {
    id: syncDialog
    title: i18n("Cloud Sync")
    modal: true
    anchors.centerIn: parent
    width: Math.min(parent.width * 0.8, Kirigami.Units.gridUnit * 28)
    height: contentColumn.implicitHeight + Kirigami.Units.gridUnit * 6
    standardButtons: QQC2.Dialog.Close

    property bool configMode: !SyncManager.isConfigured

    onOpened: {
        // Reset state on open to ensure proper layout
        resultLabel.visible = false
        configMode = !SyncManager.isConfigured
    }

    Connections {
        target: SyncManager
        function onSyncCompleted(success, message) {
            resultLabel.text = message
            resultLabel.visible = true
        }
        function onConnectionTestCompleted(success, message) {
            resultLabel.text = message
            resultLabel.visible = true
        }
    }

    contentItem: ColumnLayout {
        id: contentColumn
        spacing: Kirigami.Units.largeSpacing

        // Status Section
        Kirigami.FormLayout {
            visible: !configMode

            QQC2.Label {
                Kirigami.FormData.label: i18n("Status:")
                text: SyncManager.isConfigured ? i18n("Configured") : i18n("Not Configured")
                color: SyncManager.isConfigured ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.neutralTextColor
            }

            QQC2.Label {
                Kirigami.FormData.label: i18n("Profile ID:")
                text: SyncManager.getProfileId() || "-"
                visible: SyncManager.isConfigured
            }

            QQC2.Label {
                Kirigami.FormData.label: i18n("AWS Region:")
                text: SyncManager.getAwsRegion() || "-"
                visible: SyncManager.isConfigured
            }

            QQC2.Label {
                Kirigami.FormData.label: i18n("Last Sync:")
                text: SyncManager.lastSyncTime || i18n("Never")
            }
        }

        // Result message
        QQC2.Label {
            id: resultLabel
            visible: false
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
        }

        // Sync actions
        RowLayout {
            visible: !configMode && SyncManager.isConfigured
            Layout.alignment: Qt.AlignHCenter
            spacing: Kirigami.Units.largeSpacing

            QQC2.Button {
                text: SyncManager.isSyncing ? i18n("Syncing...") : i18n("Sync Now")
                icon.name: "view-refresh"
                enabled: !SyncManager.isSyncing
                onClicked: {
                    resultLabel.visible = false
                    SyncManager.sync()
                }
            }

            QQC2.Button {
                text: i18n("Test Connection")
                icon.name: "network-connect"
                enabled: !SyncManager.isSyncing
                onClicked: {
                    resultLabel.visible = false
                    SyncManager.testConnection()
                }
            }

            QQC2.Button {
                text: i18n("Edit Configuration")
                icon.name: "configure"
                onClicked: configMode = true
            }
        }

        // Configure button for unconfigured state
        QQC2.Button {
            visible: !configMode && !SyncManager.isConfigured
            Layout.alignment: Qt.AlignHCenter
            text: i18n("Configure Sync")
            icon.name: "configure"
            onClicked: configMode = true
        }

        // Configuration Form
        Kirigami.FormLayout {
            visible: configMode
            Layout.fillWidth: true

            QQC2.TextField {
                id: profileIdField
                Kirigami.FormData.label: i18n("Profile ID:")
                placeholderText: i18n("e.g., my-worklog-profile")
                text: SyncManager.getProfileId()
            }

            QQC2.TextField {
                id: accessKeyField
                Kirigami.FormData.label: i18n("AWS Access Key ID:")
                placeholderText: i18n("AKIAIOSFODNN7EXAMPLE")
                text: SyncManager.getAwsAccessKeyId()
            }

            QQC2.TextField {
                id: secretKeyField
                Kirigami.FormData.label: i18n("AWS Secret Access Key:")
                echoMode: TextInput.Password
                placeholderText: SyncManager.hasSecretKey() ? i18n("(unchanged)") : i18n("Your secret key")
            }

            QQC2.ComboBox {
                id: regionCombo
                Kirigami.FormData.label: i18n("AWS Region:")
                model: [
                    { text: "US East (N. Virginia)", value: "us-east-1" },
                    { text: "US East (Ohio)", value: "us-east-2" },
                    { text: "US West (N. California)", value: "us-west-1" },
                    { text: "US West (Oregon)", value: "us-west-2" },
                    { text: "EU (Ireland)", value: "eu-west-1" },
                    { text: "EU (London)", value: "eu-west-2" },
                    { text: "EU (Frankfurt)", value: "eu-central-1" },
                    { text: "Asia Pacific (Tokyo)", value: "ap-northeast-1" },
                    { text: "Asia Pacific (Singapore)", value: "ap-southeast-1" },
                    { text: "Asia Pacific (Sydney)", value: "ap-southeast-2" }
                ]
                textRole: "text"
                valueRole: "value"
                Component.onCompleted: {
                    var region = SyncManager.getAwsRegion()
                    for (var i = 0; i < model.length; i++) {
                        if (model[i].value === region) {
                            currentIndex = i
                            break
                        }
                    }
                }
            }

            RowLayout {
                spacing: Kirigami.Units.largeSpacing

                QQC2.Button {
                    text: i18n("Save Configuration")
                    icon.name: "document-save"
                    onClicked: {
                        SyncManager.saveConfiguration(
                            accessKeyField.text,
                            secretKeyField.text,
                            regionCombo.currentValue,
                            profileIdField.text
                        )
                        resultLabel.text = i18n("Configuration saved!")
                        resultLabel.visible = true
                        configMode = false
                    }
                }

                QQC2.Button {
                    text: i18n("Cancel")
                    visible: SyncManager.isConfigured
                    onClicked: configMode = false
                }
            }
        }

        // Security notice
        QQC2.Label {
            visible: configMode
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            font.italic: true
            opacity: 0.7
            text: i18n("Your credentials are stored locally and only sent to AWS for authentication.")
        }
    }
}
