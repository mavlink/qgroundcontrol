/*!
 * @file
 * @brief ST16 Settings Panel
 * @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick                  2.5
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2
import QtQuick.Dialogs          1.2
import QtQuick.Layouts          1.2
import QtGraphicalEffects       1.0

import QGroundControl                       1.0
import QGroundControl.FactSystem            1.0
import QGroundControl.FactControls          1.0
import QGroundControl.Controls              1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.Palette               1.0
import QGroundControl.Controllers           1.0
import TyphoonHQuickInterface               1.0
import TyphoonHQuickInterface.Widgets       1.0

QGCView {
    id:                 qgcView
    viewPanel:          panel
    color:              qgcPal.window
    anchors.fill:       parent
    anchors.margins:    ScreenTools.defaultFontPixelWidth

    property var    _activeVehicle:   QGroundControl.multiVehicleManager.activeVehicle
    property real   _buttonWidth:     ScreenTools.defaultFontPixelWidth * 16
    property real   _textWidth:       ScreenTools.defaultFontPixelWidth * 40
    property bool   _importAction:    false

    QGCPalette      { id: qgcPal }

    function getCalText() {
        if(_activeVehicle) {
            return qsTr("Vehicle must be powered off and unbound for RC Calibration.")
        }
        if(TyphoonHQuickInterface.m4State === TyphoonHQuickInterface.M4_STATE_FACTORY_CAL) {
            return qsTr("Move all switches to their limits. Leave them in their middle position.")
        }
        return qsTr("Ready to start calibration.")
    }

    MessageDialog {
        id:                 confirmCal
        title:              qsTr("RC Calibration")
        text:               qsTr("Move all analog switches to their limits several times. Switches must be left in their middle position before exting calibration.")
        standardButtons:    StandardButton.Ok | StandardButton.Cancel
        onAccepted: {
            TyphoonHQuickInterface.startCalibration()
            visible = false
        }
    }

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent
        Column {
            id:                 settingsColumn
            width:              qgcView.width * 0.5
            spacing:            ScreenTools.defaultFontPixelHeight
            anchors.margins:    ScreenTools.defaultFontPixelWidth
            anchors.centerIn:   parent
            QGCLabel {
                text:   qsTr("Switch Calibration")
            }
            Rectangle {
                width:          parent.width  + (ScreenTools.defaultFontPixelWidth  * 2)
                height:         calCol.height + (ScreenTools.defaultFontPixelHeight * 2)
                color:          qgcPal.windowShade
                Column {
                    id:                 calCol
                    spacing:            ScreenTools.defaultFontPixelHeight
                    anchors.centerIn:   parent
                    Row {
                        spacing:        ScreenTools.defaultFontPixelWidth * 2
                        SwitchCal {
                            text:       "J1"
                            calibrated: TyphoonHQuickInterface.J1Cal === TyphoonHQuickInterface.CalibrationStateRag
                        }
                        SwitchCal {
                            text:       "J2"
                            calibrated: TyphoonHQuickInterface.J2Cal === TyphoonHQuickInterface.CalibrationStateRag
                        }
                        SwitchCal {
                            text:       "J3"
                            calibrated: TyphoonHQuickInterface.J3Cal === TyphoonHQuickInterface.CalibrationStateRag
                        }
                        SwitchCal {
                            text:       "J4"
                            calibrated: TyphoonHQuickInterface.J4Cal === TyphoonHQuickInterface.CalibrationStateRag
                        }
                    }
                    Row {
                        spacing:        ScreenTools.defaultFontPixelWidth * 2
                        SwitchCal {
                            text:       "K1"
                            calibrated: TyphoonHQuickInterface.K1Cal === TyphoonHQuickInterface.CalibrationStateRag
                        }
                        SwitchCal {
                            text:       "K2"
                            calibrated: TyphoonHQuickInterface.K2Cal === TyphoonHQuickInterface.CalibrationStateRag
                        }
                        SwitchCal {
                            text:       "K3"
                            calibrated: TyphoonHQuickInterface.K3Cal === TyphoonHQuickInterface.CalibrationStateRag
                        }
                    }
                    Item {
                        width:  1
                        height: ScreenTools.defaultFontPixelHeight
                    }
                    Row {
                        spacing: ScreenTools.defaultFontPixelWidth * 4
                        anchors.horizontalCenter:   parent.horizontalCenter
                        QGCButton {
                            text:       "Start Calibration"
                            enabled:    !_activeVehicle && TyphoonHQuickInterface.calibrationComplete && TyphoonHQuickInterface.m4State !== TyphoonHQuickInterface.M4_STATE_FACTORY_CAL
                            width:      ScreenTools.defaultFontPixelWidth * 18
                            onClicked: {
                                confirmCal.open()
                            }
                        }
                        QGCButton {
                            text:       "Stop Calibration"
                            enabled:    !_activeVehicle && TyphoonHQuickInterface.calibrationComplete && TyphoonHQuickInterface.m4State === TyphoonHQuickInterface.M4_STATE_FACTORY_CAL
                            width:      ScreenTools.defaultFontPixelWidth * 18
                            onClicked: {
                                TyphoonHQuickInterface.stopCalibration()
                            }
                        }
                    }
                }
            }
            Item {
                width:  1
                height: ScreenTools.defaultFontPixelHeight
            }
            QGCLabel {
                text:   getCalText()
                anchors.horizontalCenter:   parent.horizontalCenter
            }
        }
    }
}
