/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Palette
import QGroundControl.Controls
import QGroundControl.ScreenTools
import QGroundControl.Controllers
import QGroundControl.FactSystem
import QGroundControl.FactControls

/// Joystick Config
SetupPage {
    id:                 joystickPage
    pageComponent:      pageComponent
    pageName:           qsTr("Joystick")
    pageDescription:    "" // qsTr("Joystick Setup is used to configure and calibrate joysticks.")

    readonly property real  _maxButtons:         64
    readonly property real  _attitudeLabelWidth: ScreenTools.defaultFontPixelWidth * 12

    Connections {
        target: joystickManager
        onAvailableJoysticksChanged: {
            if(joystickManager.joysticks.length === 0) {
                summaryButton.checked = true
                setupView.showSummaryPanel()
            }
        }
    }

    Component {
        id: pageComponent
        Item {
            width:  availableWidth
            height: bar.height + joyLoader.height

            readonly property real  labelToMonitorMargin:   ScreenTools.defaultFontPixelWidth * 3

            property var  _activeJoystick:          joystickManager.activeJoystick
            property bool _allowJoystickSelection:  QGroundControl.corePlugin.options.allowJoystickSelection

            function setupPageCompleted() {
                controller.start()
            }

            JoystickConfigController {
                id:             controller
            }

            QGCTabBar {
                id:             bar
                width:          parent.width
                Component.onCompleted: {
                    if (_activeJoystick) {
                        if (_activeJoystick.axisCount == 0) {
                            currentIndex = _allowJoystickSelection ? 0 : 1
                        } else {
                            currentIndex = _activeJoystick.calibrated ? 0 : 2
                        }
                    } else {
                        currentIndex = 0
                    }
                }
                anchors.top:    parent.top
                QGCTabButton {
                    text:       qsTr("General")
                    visible:    _allowJoystickSelection
                }
                QGCTabButton {
                    text:       qsTr("Button Assigment")
                }
                QGCTabButton {
                    text:       qsTr("Calibration")
                    visible:    _activeJoystick.axisCount != 0
                }
                QGCTabButton {
                    text:       qsTr("Advanced")
                    visible:    _activeJoystick.axisCount != 0
                }
            }

            property var pages:  ["JoystickConfigGeneral.qml", "JoystickConfigButtons.qml", "JoystickConfigCalibration.qml", "JoystickConfigAdvanced.qml"]

            Loader {
                id:             joyLoader
                source:         pages[bar.currentIndex]
                width:          parent.width
                anchors.top:    bar.bottom
            }
        }
    }
}


