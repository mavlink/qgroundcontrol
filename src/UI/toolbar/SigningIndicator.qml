import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

Item {
    id:             control
    width:          signingIcon.width * 1.1
    anchors.top:    parent.top
    anchors.bottom: parent.bottom

    property bool showIndicator:    QGroundControl.mavlinkSigningKeys.keys.count > 0

    property var    _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property bool   _signed:        _activeVehicle ? _activeVehicle.mavlinkSigning : false

    QGCPalette { id: qgcPal }

    QGCColoredImage {
        id:                 signingIcon
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        width:              height
        sourceSize.height:  height
        source:             _signed ? "/InstrumentValueIcons/lock-closed.svg" : "/InstrumentValueIcons/lock-open.svg"
        fillMode:           Image.PreserveAspectFit
        color:              _signed ? qgcPal.colorGreen : qgcPal.text
    }

    MouseArea {
        anchors.fill:   parent
        onClicked:      mainWindow.showIndicatorDrawer(signingInfoPage, control)
    }

    Component {
        id: signingInfoPage

        ToolIndicatorPage {
            showExpand:         true
            contentComponent:   signingContentComponent
            expandedComponent:  signingExpandedComponent
        }
    }

    Component {
        id: signingContentComponent

        SettingsGroupLayout {
            heading: qsTr("MAVLink Signing")

            LabelledLabel {
                label:      qsTr("Status:")
                labelText:  {
                    if (!_activeVehicle)
                        return qsTr("No Vehicle")
                    return _signed ? qsTr("Active") : qsTr("Inactive")
                }
            }

            LabelledLabel {
                label:      qsTr("Active Key:")
                labelText:  _activeVehicle && _activeVehicle.mavlinkSigningKeyName !== "" ? _activeVehicle.mavlinkSigningKeyName : qsTr("None")
                visible:    _activeVehicle
            }

            LabelledLabel {
                label:      qsTr("Saved Keys:")
                labelText:  QGroundControl.mavlinkSigningKeys.keys.count
            }
        }
    }

    Component {
        id: signingExpandedComponent

        SigningKeyManager {
        }
    }
}
