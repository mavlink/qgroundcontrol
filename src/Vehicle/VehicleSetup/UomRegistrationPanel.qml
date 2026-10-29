pragma ComponentBehavior: Bound

import QGroundControl
import QGroundControl.Controls

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: root

    required property var manager

    function reasonBytes(text) {
        try {
            return encodeURIComponent(text).replace(/%[0-9A-F]{2}/g, "x").length;
        } catch (error) {
            return 201;
        }
    }

    objectName: "uomRegistrationPanel"
    spacing: ScreenTools.defaultFontPixelHeight * 0.5

    QGCLabel {
        font.bold: true
        text: qsTr("UOM real-name registration and activation")
    }

    QGCLabel {
        Layout.fillWidth: true
        text: qsTr("Complete real-name registration and activation before first use. Registration is completed on the official UOM website.")
        wrapMode: Text.WordWrap
    }

    GridLayout {
        Layout.fillWidth: true
        columnSpacing: ScreenTools.defaultFontPixelWidth
        columns: 2

        QGCLabel {
            text: qsTr("Product model")
        }

        QGCLabel {
            text: root.manager ? root.manager.productModel : "—"
        }

        QGCLabel {
            text: qsTr("Aircraft product code (SN)")
        }

        QGCLabel {
            Layout.fillWidth: true
            objectName: "uomConnectedSerial"
            text: root.manager && root.manager.serial.length ? root.manager.serial : qsTr("Not available")
            textFormat: Text.PlainText
            wrapMode: Text.WrapAnywhere
        }

        QGCLabel {
            text: qsTr("Environment")
        }

        QGCLabel {
            objectName: "uomCurrentEnvironment"
            text: root.manager && root.manager.environment === "official" ? qsTr("Official") : qsTr("Integration — isolated test data")
        }

        QGCLabel {
            text: qsTr("Real-name registration")
        }

        QGCLabel {
            Layout.fillWidth: true
            objectName: "uomRegistrationState"
            text: root.manager ? root.manager.registrationText : qsTr("Unknown")
            textFormat: Text.PlainText
            wrapMode: Text.WordWrap
        }

        QGCLabel {
            text: qsTr("Device activation")
        }

        QGCLabel {
            Layout.fillWidth: true
            objectName: "uomDeviceActivationState"
            text: root.manager ? root.manager.deviceActivationText : qsTr("Unknown")
            textFormat: Text.PlainText
            wrapMode: Text.WordWrap
        }

        QGCLabel {
            text: qsTr("Latest UOM acknowledgement")
        }

        QGCLabel {
            Layout.fillWidth: true
            objectName: "uomReportState"
            text: root.manager ? root.manager.reportText : qsTr("Unknown")
            textFormat: Text.PlainText
            wrapMode: Text.WordWrap
        }

        QGCLabel {
            text: qsTr("Registration checked at")
        }

        QGCLabel {
            text: root.manager && root.manager.lastVerifiedAt.length ? root.manager.lastVerifiedAt : qsTr("Never")
        }

        QGCLabel {
            text: qsTr("Activation reported at")
        }

        QGCLabel {
            text: root.manager && root.manager.lastReportedAt.length ? root.manager.lastReportedAt : qsTr("Not recorded")
        }
    }

    QGCLabel {
        Layout.fillWidth: true
        objectName: "uomRegistrationStatus"
        text: root.manager ? root.manager.statusText : qsTr("Connect an aircraft to manage UOM registration")
        textFormat: Text.PlainText
        wrapMode: Text.WordWrap
    }

    QGCLabel {
        Layout.fillWidth: true
        objectName: "uomCredentialStatus"
        text: root.manager ? root.manager.credentialStatus : ""
        textFormat: Text.PlainText
        wrapMode: Text.WordWrap
    }

    QGCLabel {
        Layout.fillWidth: true
        objectName: "uomSafetyStatus"
        text: root.manager ? root.manager.safetyStatus : ""
        textFormat: Text.PlainText
        wrapMode: Text.WordWrap
    }

    Flow {
        Layout.fillWidth: true
        spacing: ScreenTools.defaultFontPixelWidth

        QGCButton {
            objectName: "uomOpenWebsiteButton"
            text: qsTr("Open official UOM website")

            onClicked: Qt.openUrlExternally("https://uom.caac.gov.cn/")
        }

        QGCButton {
            enabled: root.manager && root.manager.canConfigure
            objectName: "uomConfigureButton"
            text: qsTr("Configure UOM connection")

            onClicked: configurationFactory.open({
                targetManager: root.manager
            })
        }

        QGCButton {
            enabled: root.manager && root.manager.canVerify
            objectName: "uomVerifyButton"
            visible: root.manager && root.manager.available
            text: qsTr("Check real-name registration")

            onClicked: {
                if (root.manager) {
                    root.manager.verifyRegistration();
                }
            }
        }

        QGCButton {
            enabled: root.manager && root.manager.canActivate
            objectName: "uomActivateButton"
            visible: root.manager && root.manager.available
            text: qsTr("Activate aircraft")

            onClicked: activationFactory.open({
                targetManager: root.manager,
                targetSerial: root.manager.serial,
                targetUid: root.manager.uid
            })
        }

        QGCButton {
            enabled: root.manager && root.manager.canDeactivate
            objectName: "uomDeactivateButton"
            visible: root.manager && root.manager.available
            text: qsTr("Deactivate and cancel registration")

            onClicked: cancellationFactory.open({
                targetManager: root.manager,
                targetSerial: root.manager.serial,
                targetUid: root.manager.uid
            })
        }

        QGCButton {
            enabled: root.manager && root.manager.canRetry
            objectName: "uomRetryButton"
            text: qsTr("Retry pending operation")
            visible: root.manager && root.manager.available && root.manager.pending

            onClicked: retryFactory.open({
                targetManager: root.manager,
                targetSerial: root.manager.serial,
                targetUid: root.manager.uid
            })
        }
    }

    QGCPopupDialogFactory {
        id: configurationFactory

        dialogComponent: configurationComponent
    }

    Component {
        id: configurationComponent

        QGCPopupDialog {
            id: configurationDialog

            readonly property bool identityEditable: targetManager && targetManager.canConfigure && !targetManager.pending
            readonly property bool profileChanged: targetManager && (producerInput.text.trim() !== targetManager.producerId || verifyInput.text.trim() !== targetManager.verifyUrl || reportInput.text.trim() !== targetManager.reportUrl || cancelInput.text.trim() !== targetManager.cancelUrl)
            readonly property bool profileEditable: targetManager && targetManager.canConfigure
            required property var targetManager

            acceptButtonEnabled: targetManager && targetManager.canConfigure && producerInput.text.trim().length > 0 && serialInput.text.trim().length > 0 && verifyInput.text.trim().length > 0 && reportInput.text.trim().length > 0 && cancelInput.text.trim().length > 0 && (!targetManager.pending || !profileChanged || profileRepairConfirmation.checked)
            buttons: Dialog.Save | Dialog.Cancel
            objectName: "uomConfigurationDialog"
            title: qsTr("Configure UOM connection")

            onAccepted: {
                if (targetManager && targetManager.canConfigure) {
                    targetManager.configure(environmentInput.currentIndex === 0 ? "integration" : "official", producerInput.text.trim(), serialInput.text.trim(), verifyInput.text.trim(), reportInput.text.trim(), cancelInput.text.trim(), keyInput.text.trim());
                }
                keyInput.clear();
            }
            onClosed: keyInput.clear()

            ColumnLayout {
                width: ScreenTools.defaultFontPixelWidth * 62

                QGCLabel {
                    Layout.fillWidth: true
                    text: qsTr("Use the producer ID and key ID issued for this environment. Official URLs are provided by UOM after approval.")
                    wrapMode: Text.WordWrap
                }

                QGCLabel {
                    Layout.fillWidth: true
                    text: qsTr("An operation is pending. You may correct its producer ID, URLs or key. The environment, aircraft and original operation stay fixed. Continue will read the aircraft and check UOM registration again.")
                    visible: configurationDialog.targetManager && configurationDialog.targetManager.pending
                    wrapMode: Text.WordWrap
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2

                    QGCLabel {
                        text: qsTr("Environment")
                    }

                    QGCComboBox {
                        id: environmentInput

                        Layout.fillWidth: true
                        currentIndex: configurationDialog.targetManager && configurationDialog.targetManager.environment === "official" ? 1 : 0
                        enabled: configurationDialog.identityEditable
                        model: [qsTr("Integration"), qsTr("Official")]
                        objectName: "uomEnvironmentInput"

                        onActivated: index => {
                            var profile = configurationDialog.targetManager.configurationForEnvironment(index === 0 ? "integration" : "official");
                            producerInput.text = profile.producerId;
                            serialInput.text = profile.expectedSerial;
                            verifyInput.text = profile.verifyUrl;
                            reportInput.text = profile.reportUrl;
                            cancelInput.text = profile.cancelUrl;
                            keyInput.clear();
                        }
                    }

                    QGCLabel {
                        text: qsTr("Producer ID")
                    }

                    QGCTextField {
                        id: producerInput

                        Layout.fillWidth: true
                        enabled: configurationDialog.profileEditable
                        objectName: "uomProducerInput"
                        text: configurationDialog.targetManager ? configurationDialog.targetManager.producerId : ""
                    }

                    QGCLabel {
                        text: qsTr("Expected aircraft SN")
                    }

                    QGCTextField {
                        id: serialInput

                        Layout.fillWidth: true
                        enabled: configurationDialog.identityEditable
                        inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                        maximumLength: 20
                        objectName: "uomExpectedSerialInput"
                        text: configurationDialog.targetManager ? configurationDialog.targetManager.expectedSerial : ""
                    }

                    QGCLabel {
                        text: qsTr("Registration query URL")
                    }

                    QGCTextField {
                        id: verifyInput

                        Layout.fillWidth: true
                        enabled: configurationDialog.profileEditable
                        objectName: "uomVerifyUrlInput"
                        text: configurationDialog.targetManager ? configurationDialog.targetManager.verifyUrl : ""
                    }

                    QGCLabel {
                        text: qsTr("Activation report URL")
                    }

                    QGCTextField {
                        id: reportInput

                        Layout.fillWidth: true
                        enabled: configurationDialog.profileEditable
                        objectName: "uomReportUrlInput"
                        text: configurationDialog.targetManager ? configurationDialog.targetManager.reportUrl : ""
                    }

                    QGCLabel {
                        text: qsTr("Registration cancellation URL")
                    }

                    QGCTextField {
                        id: cancelInput

                        Layout.fillWidth: true
                        enabled: configurationDialog.profileEditable
                        objectName: "uomCancelUrlInput"
                        text: configurationDialog.targetManager ? configurationDialog.targetManager.cancelUrl : ""
                    }

                    QGCLabel {
                        text: qsTr("Key ID")
                    }

                    QGCTextField {
                        id: keyInput

                        Layout.fillWidth: true
                        echoMode: TextInput.Password
                        enabled: configurationDialog.targetManager && configurationDialog.targetManager.canConfigure
                        inputMethodHints: Qt.ImhSensitiveData | Qt.ImhNoPredictiveText
                        objectName: "uomKeyInput"
                        placeholderText: qsTr("Leave blank to keep this environment's saved key")
                    }
                }

                QGCCheckBox {
                    id: profileRepairConfirmation

                    Layout.fillWidth: true
                    objectName: "uomProfileRepairConfirmation"
                    text: qsTr("I confirm these corrections for the pending operation. Continuing may send its original cancellation request.")
                    visible: configurationDialog.targetManager && configurationDialog.targetManager.pending && configurationDialog.profileChanged
                }

                QGCLabel {
                    Layout.fillWidth: true
                    text: qsTr("The expected SN identifies one aircraft. It does not provision or change the RID product code.")
                    wrapMode: Text.WordWrap
                }
            }

            Connections {
                function onManagerChanged() {
                    configurationDialog.close();
                }

                target: root
            }
        }
    }

    QGCPopupDialogFactory {
        id: activationFactory

        dialogComponent: activationComponent
    }

    Component {
        id: activationComponent

        QGCPopupDialog {
            id: activationDialog

            required property var targetManager
            required property string targetSerial
            required property string targetUid

            acceptButtonEnabled: targetManager && targetManager.canActivate && root.manager && root.manager.serial === targetSerial && root.manager.uid === targetUid
            buttons: Dialog.Ok | Dialog.Cancel
            title: qsTr("Activate aircraft")

            onAccepted: {
                if (acceptButtonEnabled) {
                    targetManager.activate();
                }
            }

            QGCLabel {
                text: qsTr("Activate aircraft %1 and report its confirmed device state to UOM?").arg(activationDialog.targetSerial)
                textFormat: Text.PlainText
                width: ScreenTools.defaultFontPixelWidth * 50
                wrapMode: Text.WordWrap
            }

            Connections {
                function onManagerChanged() {
                    activationDialog.close();
                }

                target: root
            }
        }
    }

    QGCPopupDialogFactory {
        id: cancellationFactory

        dialogComponent: cancellationComponent
    }

    Component {
        id: cancellationComponent

        QGCPopupDialog {
            id: cancellationDialog

            readonly property int reasonLength: root.reasonBytes(reasonInput.text.trim())
            required property var targetManager
            required property string targetSerial
            required property string targetUid

            acceptButtonEnabled: targetManager && targetManager.canDeactivate && root.manager && root.manager.serial === targetSerial && root.manager.uid === targetUid && reasonType.currentIndex >= 0 && reasonLength > 0 && reasonLength <= 200 && cancellationConfirmed.checked
            buttons: Dialog.Ok | Dialog.Cancel
            objectName: "uomCancellationDialog"
            title: qsTr("Deactivate and cancel registration")

            onAccepted: {
                if (acceptButtonEnabled) {
                    targetManager.deactivateAndCancel(["0", "3", "4"][reasonType.currentIndex], reasonInput.text.trim());
                }
            }

            ColumnLayout {
                width: ScreenTools.defaultFontPixelWidth * 55

                QGCLabel {
                    Layout.fillWidth: true
                    text: qsTr("Aircraft %1 will be deactivated and its UOM registration cancelled. Complete this before transferring ownership.").arg(cancellationDialog.targetSerial)
                    textFormat: Text.PlainText
                    wrapMode: Text.WordWrap
                }

                QGCComboBox {
                    id: reasonType

                    Layout.fillWidth: true
                    currentIndex: -1
                    model: [qsTr("Retired from use"), qsTr("Ownership transfer"), qsTr("Other reason")]
                    objectName: "uomCancellationTypeInput"
                }

                QGCTextField {
                    id: reasonInput

                    Layout.fillWidth: true
                    objectName: "uomCancellationReasonInput"
                    placeholderText: qsTr("Enter the cancellation reason")
                }

                QGCLabel {
                    Layout.fillWidth: true
                    text: qsTr("Reason size: %1 / 200 UTF-8 bytes").arg(cancellationDialog.reasonLength)
                    wrapMode: Text.WordWrap
                }

                QGCCheckBox {
                    id: cancellationConfirmed

                    Layout.fillWidth: true
                    objectName: "uomCancellationConfirmed"
                    text: qsTr("I confirm this aircraft and understand that registration will be cancelled")
                    wrapMode: Text.WordWrap
                }
            }

            Connections {
                function onManagerChanged() {
                    cancellationDialog.close();
                }

                target: root
            }
        }
    }

    QGCPopupDialogFactory {
        id: retryFactory

        dialogComponent: retryComponent
    }

    Component {
        id: retryComponent

        QGCPopupDialog {
            id: retryDialog

            required property var targetManager
            required property string targetSerial
            required property string targetUid

            acceptButtonEnabled: targetManager && targetManager.canRetry && root.manager && root.manager.serial === targetSerial && root.manager.uid === targetUid
            buttons: Dialog.Ok | Dialog.Cancel
            title: qsTr("Retry pending operation")

            onAccepted: {
                if (acceptButtonEnabled) {
                    targetManager.retryPending();
                }
            }

            QGCLabel {
                text: qsTr("Continue the pending operation for aircraft %1? A pending cancellation may resend the UOM registration cancellation request.").arg(retryDialog.targetSerial)
                textFormat: Text.PlainText
                width: ScreenTools.defaultFontPixelWidth * 50
                wrapMode: Text.WordWrap
            }

            Connections {
                function onManagerChanged() {
                    retryDialog.close();
                }

                target: root
            }
        }
    }
}
