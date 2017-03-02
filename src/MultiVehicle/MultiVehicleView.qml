/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.4
import QtQuick.Controls 1.3
import QtQuick.Dialogs  1.2
import QtQuick.Layouts  1.2

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controllers   1.0
import QGroundControl.FactSystem    1.0

/// Multi-Vehicle View
QGCView {
    id:         qgcView
    viewPanel:  panel

    property real   _margins: ScreenTools.defaultFontPixelWidth
    property var    _fileDialogController

    readonly property string _loadingText: qsTr("Loading...")

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent

        Rectangle {
            anchors.fill:   parent
            color:          qgcPal.window

            QGCFlickable {
                anchors.fill:       parent
                contentHeight:      vehicleColumn.height
                flickableDirection: Flickable.VerticalFlick
                clip:               true

                Column {
                    id:                 vehicleColumn
                    anchors.margins:    _margins
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    anchors.top:        parent.top
                    spacing:            _margins

                    QGCLabel { text: qsTr("All Vehicles") }

                    Repeater {
                        model: QGroundControl.multiVehicleManager.vehicles

                        Column {
                            anchors.left:   parent.left
                            anchors.right:  parent.right
                            spacing:        ScreenTools.defaultFontPixelHeight / 2

                            MissionController {
                                id: missionController

                                Component.onCompleted: startStaticActiveVehicle(object)

                                property bool missionAvailable: visualItems && visualItems.count > 1

                                function loadFromSelectedFile() {
                                    if (ScreenTools.isMobile) {
                                        _fileDialogController = missionController
                                        qgcView.showDialog(mobileFilePicker, qsTr("Select Mission File"), qgcView.showDialogDefaultWidth, StandardButton.Yes | StandardButton.Cancel)
                                    } else {
                                        missionController.loadFromFilePicker()
                                        missionController.sendToVehicle()
                                    }
                                }
                            } // MissionController

                            GeoFenceController {
                                id: geoFenceController

                                Component.onCompleted: startStaticActiveVehicle(object)

                                property bool fenceAvailable: fenceSupported && (circleSupported || polygonSupported)

                                function loadFromSelectedFile() {
                                    if (ScreenTools.isMobile) {
                                        _fileDialogController = geoFenceController
                                        qgcView.showDialog(mobileFilePicker, qsTr("Select Fence File"), qgcView.showDialogDefaultWidth, StandardButton.Yes | StandardButton.Cancel)
                                    } else {
                                        geoFenceController.loadFromFilePicker()
                                        geoFenceController.sendToVehicle()
                                    }
                                }
                            } // GeoFenceController

                            RallyPointController {
                                id: rallyPointController

                                Component.onCompleted: startStaticActiveVehicle(object)

                                property bool pointsAvailable: rallyPointsSupported && points.count

                                function loadFromSelectedFile() {
                                    if (ScreenTools.isMobile) {
                                        _fileDialogController = rallyPointController
                                        qgcView.showDialog(mobileFilePicker, qsTr("Select Rally Point File"), qgcView.showDialogDefaultWidth, StandardButton.Yes | StandardButton.Cancel)
                                    } else {
                                        rallyPointController.loadFromFilePicker()
                                        rallyPointController.sendToVehicle()
                                    }
                                }
                            } // RallyPointController

                            QGCLabel {
                                text: "Vehicle #" + object.id
                            }

                            Rectangle {
                                anchors.left:   parent.left
                                anchors.right:  parent.right
                                height:         vehicleDisplayColumn.height + (_margins * 2)
                                color:          qgcPal.windowShade

                                Column {
                                    id:                 vehicleDisplayColumn
                                    anchors.margins:    _margins
                                    anchors.left:       parent.left
                                    anchors.right:      parent.right
                                    anchors.top:        parent.top

                                    Row {
                                        id:                 indicatorRow
                                        spacing:            _margins
                                        visible:            !object.connectionLost

                                        Rectangle {
                                            width:          missionLabel.contentWidth + _margins
                                            height:         ScreenTools.defaultFontPixelHeight + _margins
                                            radius:         height / 4
                                            color:          missionController.missionAvailable ? "green" : qgcPal.window
                                            border.width:   1
                                            border.color:   qgcPal.text

                                            QGCLabel {
                                                id:                 missionLabel
                                                anchors.margins:    _margins / 2
                                                anchors.left:       parent.left
                                                anchors.top:        parent.top
                                                text:               missionController.syncInProgress ? _loadingText : qsTr("Mission")
                                            }

                                            MouseArea {
                                                anchors.fill:   parent
                                                enabled:        !missionController.syncInProgress
                                                onClicked:      missionController.loadFromSelectedFile()
                                            }
                                        }

                                        Rectangle {
                                            width:          fenceLabel.contentWidth + _margins
                                            height:         ScreenTools.defaultFontPixelHeight + _margins
                                            radius:         height / 4
                                            color:          geoFenceController.fenceAvailable ? "green" : qgcPal.window
                                            border.width:   1
                                            border.color:   qgcPal.text

                                            QGCLabel {
                                                id:                 fenceLabel
                                                anchors.margins:    _margins / 2
                                                anchors.left:       parent.left
                                                anchors.top:        parent.top
                                                text:               geoFenceController.syncInProgress ? _loadingText : qsTr("Fence")
                                            }

                                            MouseArea {
                                                anchors.fill:   parent
                                                enabled:        !geoFenceController.syncInProgress
                                                onClicked:      geoFenceController.loadFromSelectedFile()
                                            }
                                        }

                                        Rectangle {
                                            width:          rallyLabel.contentWidth + _margins
                                            height:         ScreenTools.defaultFontPixelHeight + _margins
                                            radius:         height / 4
                                            color:          rallyPointController.pointsAvailable ? "green" : qgcPal.window
                                            border.width:   1
                                            border.color:   qgcPal.text

                                            QGCLabel {
                                                id:                 rallyLabel
                                                anchors.margins:    _margins / 2
                                                anchors.left:       parent.left
                                                anchors.top:        parent.top
                                                text:               rallyPointController.syncInProgress ? _loadingText : qsTr("Rally")
                                            }

                                            MouseArea {
                                                anchors.fill:   parent
                                                enabled:        !rallyPointController.syncInProgress
                                                onClicked:      rallyPointController.loadFromSelectedFile()
                                            }
                                        }

                                        FlightModeDropdown { activeVehicle: object }

                                        GuidedBar { activeVehicle: object }
                                    } // Row - contents display

                                    Flow {
                                        anchors.left:       parent.left
                                        anchors.right:      parent.right
                                        layoutDirection:    Qt.LeftToRight
                                        spacing:            _margins

                                        Repeater {
                                            model: [ "battery.voltage", "battery.percentRemaining", "altitudeRelative", "altitudeAMSL", "groundSpeed", "heading"]

                                            Column {
                                                property Fact fact: object.getFact(modelData)

                                                QGCLabel {
                                                    anchors.horizontalCenter:    parent.horizontalCenter
                                                    text:                       fact.shortDescription
                                                }
                                                Row {
                                                    anchors.horizontalCenter:    parent.horizontalCenter
                                                    //spacing:                    ScreenTools.defaultFontPixelWidth

                                                    QGCLabel {
                                                        text: fact.enumOrValueString
                                                    }
                                                    QGCLabel {
                                                        text: fact.units
                                                    }
                                                }
                                            }
                                        } // Repeater - Small
                                    } // Flow
                                } // Column
                            } // Rectangle - contents display
                        } // Column - layout for vehicle
                    } // Repeater - vehicle repeater
                } // Column
            } // QGCFlickable
        } // Rectangle - View background
    } // QGCViewPanel

    Component {
        id: mobileFilePicker

        QGCMobileFileDialog {
            openDialog:         true
            fileExtension:      _fileDialogController.fileExtension

            onFilenameReturned: {
                _fileDialogController.loadFromFile(filename)
                _fileDialogController.sendToVehicle()
            }
        }
    }
} // QGCView
