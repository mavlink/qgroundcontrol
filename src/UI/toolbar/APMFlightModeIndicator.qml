/****************************************************************************
 *
 * (c) 2009-2022 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.MultiVehicleManager
import QGroundControl.ScreenTools
import QGroundControl.Palette
import QGroundControl.FactSystem
import QGroundControl.FactControls

FlightModeIndicator {
    waitForParameters:      true
    expandedPageComponent:  activeVehicle.multiRotor ? copterComponent : undefined

    property var  activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

    Component {
        id: copterComponent

        SettingsGroupLayout {
            Layout.fillWidth:   true
            heading:            qsTr("Return to Launch")

            property Fact rtlAltFact: controller.getParameterFact(-1, "RTL_ALT")

            FactPanelController { id: controller }

            RowLayout {
                Layout.fillWidth:   true
                spacing:            ScreenTools.defaultFontPixelWidth * 2

                QGCLabel {
                    id:                 label  
                    Layout.fillWidth:   true
                    text:               qsTr("Return At")
                }

                QGCComboBox {
                    id:             returnAtCombo
                    sizeToContents: true
                    model:          [ qsTr("Current alttiude"), qsTr("Specified altitude") ]

                    function setCurrentIndex() {
                        if (rtlAltFact.value === 0) {
                            returnAtCombo.currentIndex = 0
                        } else {
                            returnAtCombo.currentIndex = 1
                        }
                    }

                    Component.onCompleted: setCurrentIndex()

                    onActivated: (index) => {
                        if (index === 0) {
                            rtlAltFact.rawValue = 0
                        } else {
                            rtlAltFact.rawValue = 1500
                        }
                    }

                    Connections {
                        target:             rtlAltFact
                        onRawValueChanged:  returnAtCombo.setCurrentIndex()
                    }
                }

                FactTextField {
                    fact:       rtlAltFact
                    enabled:    rtlAltFact.rawValue !== 0
                }
            }
        }
    }
}
