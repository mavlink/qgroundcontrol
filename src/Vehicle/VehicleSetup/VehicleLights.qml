/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick              2.3
// import QtQuick.Controls     1.2
import QtQuick.Layouts      1.2

import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0
import QGroundControl               1.0

SetupPage {
    id:             tuningPage
    pageComponent:  tuningPageComponent

    Component {
        id: tuningPageComponent

        Column {
            width: availableWidth
            height: availableHeight

            property real _margins: ScreenTools.defaultFontPixelHeight
            property bool _loadComplete: false
            property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
            property int currentIndex: 1

            function setLightState(channel) {
                currentIndex = channel
                _activeVehicle.vehicleLight_one(channel)
            }

            Column {
                anchors.left:       parent.left
                anchors.right:      parent.right
                spacing:            _margins
                visible:            !advanced

                QGCLabel {
                    text:       qsTr("Vehicle Lights Setup")
                    font.family: ScreenTools.demiboldFontFamily
                }

                Flow {
                    id:                 flowLayout
                    Layout.fillWidth:   true
                    spacing:            _margins

                    Rectangle {
                        height: switchLabel.height + autoTuneRect.height
                        width:  autoTuneRect.width
                        color:  qgcPal.window

                        QGCLabel {
                            id:                 switchLabel
                            text:               qsTr("Lights")
                            font.family:        ScreenTools.demiboldFontFamily
                        }

                        Rectangle {
                            id:             autoTuneRect
                            width:          autoTuneColumn.x + autoTuneColumn.width + _margins
                            height:         autoTuneColumn.y + autoTuneColumn.height + _margins
                            anchors.top:    switchLabel.bottom
                            // color:          qgcPal.windowShade

                            radius: 10

                            gradient: Gradient {
                            GradientStop { position: 0.0; color: qgcPal.gradientDarkerWindow }
                            GradientStop { position: 1.0; color: qgcPal.gradientLightWindow }
                        }

                            Column {
                                id:                 autoTuneColumn
                                anchors.margins:    _margins
                                anchors.left:       parent.left
                                anchors.top:        parent.top
                                spacing:            _margins

                                Row {
                                    spacing:    _margins

                                    QGCLabel {
                                        anchors.baseline:   lightOneCombo.baseline
                                        text:               qsTr("Light On / Off:")
                                    }

                                    QGCComboBox {
                                        id:             lightOneCombo
                                        width:          ScreenTools.defaultFontPixelWidth * 14
                                        model:          [qsTr("Off"), qsTr("On")]
                                        currentIndex:   _activeVehicle.factVehicleLight()

                                        onActivated: {
                                            var channel = index
                                            _activeVehicle.setfactVehicleLight(index)
                                            setLightState(channel)
                                        }
                                    }
                                }
                            }
                        } // Rectangle - Switch
                    } // Rectangle - Switch On / Off
                } // On / off
            }

        }
    }
}
