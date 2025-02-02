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
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactSystem
import QGroundControl.FactControls
import QGroundControl.Controls
import QGroundControl.MultiVehicleManager
import QGroundControl.ScreenTools
import QGroundControl.Palette

RowLayout {
    id:         control
    spacing:    0

    property bool   showIndicator:        _multipleVehicles
    property var    _activeVehicle:       QGroundControl.multiVehicleManager.activeVehicle
    property bool   _multipleVehicles:    QGroundControl.multiVehicleManager.vehicles.count > 1
    property var    _vehicleModel:        [ ]

    Connections {
        target:         QGroundControl.multiVehicleManager.vehicles
        onCountChanged: _updateVehicleModel()
    }

    Component.onCompleted:      _updateVehicleModel()
    on_ActiveVehicleChanged:    _updateVehicleModel()

    RowLayout {
        Layout.fillWidth: true

        QGCColoredImage {
            width:      ScreenTools.defaultFontPixelWidth * 4
            height:     ScreenTools.defaultFontPixelHeight * 1.33
            fillMode:   Image.PreserveAspectFit
            mipmap:     true
            color:      qgcPal.text
            source:     "/InstrumentValueIcons/airplane.svg"
        }

        QGCLabel {
            text:               _activeVehicle ? qsTr("Vehicle") + " " + _activeVehicle.id : qsTr("N/A")
            font.pointSize:     ScreenTools.mediumFontPointSize
            Layout.alignment:   Qt.AlignCenter

            MouseArea {
                anchors.fill:   parent
                onClicked:      mainWindow.showIndicatorDrawer(vehicleSelectorDrawer, control)
            }
        }
    }

    Component {
        id: vehicleSelectorDrawer

        ToolIndicatorPage {
            showExpand: true

            contentComponent: Component {
                ColumnLayout {
                    spacing: ScreenTools.defaultFontPixelWidth / 2

                    Repeater {
                        model: _vehicleModel

                        QGCButton {
                            text:               modelData
                            Layout.fillWidth:   true

                            onClicked: {
                                var vehicleId = modelData.split(" ")[1]
                                var vehicle = QGroundControl.multiVehicleManager.getVehicleById(vehicleId)
                                QGroundControl.multiVehicleManager.activeVehicle = vehicle
                                mainWindow.closeIndicatorDrawer()
                            }
                        }
                    }
                }
            }

            expandedComponent: Component {
                SettingsGroupLayout {
                    Layout.fillWidth: true

                    FactCheckBoxSlider {
                        Layout.fillWidth:   true
                        text:               qsTr("Enable Multi-Vehicle Panel")
                        fact:               _enableMultiVehiclePanel
                        visible:            _enableMultiVehiclePanel.visible

                        property Fact _enableMultiVehiclePanel: QGroundControl.settingsManager.appSettings.enableMultiVehiclePanel
                    }
                }
            }
        }
    }

    function _updateVehicleModel() {
        var newModel = [ ]
        if (_multipleVehicles) {
            for (var i = 0; i < QGroundControl.multiVehicleManager.vehicles.count; i++) {
                var vehicle = QGroundControl.multiVehicleManager.vehicles.get(i)
                newModel.push(qsTr("Vehicle") + " " + vehicle.id)
            }
        }
        _vehicleModel = newModel
    }
}
