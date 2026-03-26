import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

Item {
    Layout.fillWidth: true
    implicitHeight:   _layout.implicitHeight

    property var  _activeVehicle:   QGroundControl.multiVehicleManager.activeVehicle
    property var  _offlineVehicle:  QGroundControl.multiVehicleManager.offlineEditingVehicle
    property var  _remoteIDManager: _activeVehicle ? _activeVehicle.remoteIDManager : null
    property Fact _operatorIDFact:  QGroundControl.settingsManager.remoteIDSettings.operatorID
    property real _textFieldWidth:  ScreenTools.defaultFontPixelWidth * 24

    property bool isEURegion: QGroundControl.settingsManager.remoteIDSettings.region.rawValue === RemoteIDSettings.RegionOperation.EU

    RowLayout {
        id:                 _layout
        anchors.left:       parent.left
        anchors.right:      parent.right
        spacing:            ScreenTools.defaultFontPixelWidth * 2

        QGCLabel {
            Layout.fillWidth:   true
            text:               _operatorIDFact.shortDescription
        }

        QGCTextField {
            id:                     operatorIDTextField
            Layout.preferredWidth:  _textFieldWidth
            Layout.fillWidth:       true
            text:                   _operatorIDFact.valueString
            visible:                _operatorIDFact.visible
            maximumLength:          20

            property bool operatorIDInvalid: ((isEURegion || QGroundControl.settingsManager.remoteIDSettings.sendOperatorID.value)
                                              && _remoteIDManager && !_remoteIDManager.operatorIDGood)

            onOperatorIDInvalidChanged: {
                if (operatorIDInvalid) {
                    operatorIDTextField.showValidationError(qsTr("Invalid Operator ID"), _operatorIDFact.valueString, false)
                } else {
                    operatorIDTextField.clearValidationError(false)
                }
            }

            onTextChanged: {
                if (_activeVehicle) {
                    _remoteIDManager.checkOperatorID(text)
                } else {
                    _offlineVehicle.remoteIDManager.checkOperatorID(text)
                }
                _operatorIDFact.value = text
            }

            onEditingFinished: {
                if (_activeVehicle) {
                    _remoteIDManager.setOperatorID()
                } else {
                    _offlineVehicle.remoteIDManager.setOperatorID()
                }
            }
        }
    }
}
