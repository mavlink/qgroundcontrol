/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightDisplay
import QGroundControl.FlightMap
import QGroundControl.Palette
import QGroundControl.ScreenTools


Item {

    Rectangle {
        id:                     topRightPanel
        anchors.fill:           parent
        width:                  parent.width
        height:                 parent.height
        color:                  qgcPal.toolbarBackground
        visible:                !QGroundControl.videoManager.fullScreen && togglePanelBtn.checked

        property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle

        QGCPalette { id: qgcPal }

        MultiVehicleList {
            id:                    multiVehicleList
            anchors.top:           parent.top
            anchors.bottom:        parent.verticalCenter
            anchors.right:         parent.right
            anchors.left:          parent.left
            anchors.margins:       _toolsMargin
        }

        TerrainProgress {
            id:           terrainProgress
            anchors.top:  multiVehicleList.bottom
        }

        ColumnLayout {
            id:                     selectionGroup
            anchors.top:            terrainProgress.bottom
            anchors.right:          parent.right
            anchors.left:           parent.left
            anchors.margins:        _margins
            Layout.alignment:       Qt.AlignHCenter

            QGCLabel {
                text: qsTr("Multi Vehicle Selection")
            }

            QGCFlickable {
                Layout.fillWidth:       true
                height:                 ScreenTools.defaultFontPixelWidth * 6
                contentWidth:           selectionRowLayout.width
                flickableDirection:     Flickable.HorizontalFlick

                RowLayout {
                    id:             selectionRowLayout
                    spacing:        _margins

                    QGCButton {
                        text:       qsTr("Select All")
                        enabled:    multiVehicleList.getSelectedVehicles().length !== QGroundControl.multiVehicleManager.vehicles.count
                        onClicked:  multiVehicleList.selectAll()
                    }

                    QGCButton {
                        text:       qsTr("Deselect All")
                        enabled:    multiVehicleList.getSelectedVehicles().length > 0
                        onClicked:  multiVehicleList.deselectAll()
                    }

                    QGCButton {
                        text:       qsTr("Activate")
                        enabled:    multiVehicleList.getSelectedVehicles().length === 1 && _activeVehicle !== multiVehicleList.getSelectedVehicles()[0]
                        onClicked:  multiVehicleList.activateVehicle()
                    }

                }
            }

        }

        ColumnLayout {
            id:                     actionGroup
            anchors.top:            selectionGroup.bottom
            anchors.right:          parent.right
            anchors.left:           parent.left
            anchors.margins:        _margins

            QGCLabel {
                text: qsTr("Multi Vehicle Actions")
            }

            QGCFlickable {
                Layout.fillWidth:       true
                height:                 ScreenTools.defaultFontPixelWidth * 6
                contentWidth:           actionRowLayout.width
                flickableDirection:     Flickable.HorizontalFlick

                RowLayout {
                    id:             actionRowLayout
                    spacing:        _margins

                    QGCButton {
                        text:       qsTr("Arm")
                        enabled:    multiVehicleList.armAvailable()
                        onClicked:  multiVehicleList.armSelectedVehicles()
                    }

                    QGCButton {
                        text:       qsTr("Disarm")
                        enabled:    multiVehicleList.disarmAvailable()
                        onClicked:  multiVehicleList.disarmSelectedVehicles()
                    }

                    QGCButton {
                        text:       qsTr("Start")
                        enabled:    multiVehicleList.startAvailable()
                        onClicked:  multiVehicleList.startSelectedVehicles()
                    }

                    QGCButton {
                        text:       qsTr("Pause")
                        enabled:    multiVehicleList.pauseAvailable()
                        onClicked:  multiVehicleList.pauseSelectedVehicles()
                    }
                }
            }
        }

        ColumnLayout {
            id:                     vehicleModeGroup
            anchors.top:            actionGroup.bottom
            anchors.right:          parent.right
            anchors.left:           parent.left
            anchors.margins:        _margins
            Layout.alignment: Qt.AlignHCenter

            QGCLabel {
                text: qsTr("Multi Vehicle Modes")
            }

            QGCFlickable {
                Layout.fillWidth:       true
                height:                 ScreenTools.defaultFontPixelWidth * 6
                contentWidth:           vehicleModeRowLayout.width
                flickableDirection:     Flickable.HorizontalFlick

                RowLayout {
                    id:             vehicleModeRowLayout
                    spacing:        _margins

                    QGCButton {
                        text:       qsTr("RTL")
                        enabled:    multiVehicleList.rtlAvailable()
                        onClicked:  multiVehicleList.rtlSelectedVehicles()
                    }

                    QGCButton {
                        text:       qsTr("Take control")
                        enabled:    multiVehicleList.takeControlAvailable()
                        onClicked:  multiVehicleList.takeControlSelectedVehicles()
                    }

                }
            }

        }

    }

    QGCButton {
        id:                          togglePanelBtn
        anchors.top:                 parent.top
        anchors.horizontalCenter:    parent.horizontalCenter
        anchors.topMargin:           topRightPanel.visible ? parent.height : 0
        width:                       _rightPanelWidth / 5
        height:                      _rightPanelWidth / 18
        checkable:                   true

        background: Rectangle {
            radius:                  parent.height / 2
            color:                   qgcPal.toolbarBackground
            border.color:            qgcPal.text
            border.width:            1
        }
    }

    // We use a Loader to load the photoVideoControlComponent only when the active vehicle is not null
    // This make it easier to implement PhotoVideoControl without having to check for the mavlink camera
    // to be null all over the place
    Loader {
        id:                 photoVideoControlLoader
        anchors.top:        togglePanelBtn.bottom
        sourceComponent:    globals.activeVehicle && !togglePanelBtn.checked ? photoVideoControlComponent : undefined

        property real rightEdgeCenterInset: visible ? parent.width - x : 0

        Component {
            id: photoVideoControlComponent

            PhotoVideoControl {
            }
        }
    }

}
