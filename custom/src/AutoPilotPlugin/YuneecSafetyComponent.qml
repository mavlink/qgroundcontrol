/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Layouts          1.2
import QtGraphicalEffects       1.0

import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0


SetupPage {
    id:             safetyPage
    pageComponent:  pageComponent

    Component {
        id: pageComponent

        Item {
            width:  Math.max(availableWidth, outerGrid.width)
            height: lastRect.y + lastRect.height

            FactPanelController {
                id:         controller
                factPanel:  safetyPage.viewPanel
            }

            property real _margins:         ScreenTools.defaultFontPixelHeight
            property real _editFieldWidth:  ScreenTools.defaultFontPixelWidth * 20
            property real _imageWidth:      ScreenTools.defaultFontPixelWidth * 15
            property real _imageHeight:     ScreenTools.defaultFontPixelHeight * 3

            property Fact _fenceAction:     controller.getParameterFact(-1, "GF_ACTION")
            property Fact _fenceRadius:     controller.getParameterFact(-1, "GF_MAX_HOR_DIST")
            property Fact _fenceAlt:        controller.getParameterFact(-1, "GF_MAX_VER_DIST")

            property Fact _maxVelUp:        controller.getParameterFact(-1, "MPC_Z_VEL_MAX_DN")
            property Fact _maxVelDn:        controller.getParameterFact(-1, "MPC_Z_VEL_MAX_UP")
            property Fact _maxVelHz:        controller.getParameterFact(-1, "MPC_VEL_MANUAL")

            ExclusiveGroup { id: homeLoiterGroup }

            Rectangle {
                x:      geoFenceGrid.x + outerGrid.x - _margins
                y:      geoFenceGrid.y + outerGrid.y - _margins
                width:  geoFenceGrid.width + (_margins * 2)
                height: geoFenceGrid.height + (_margins * 2)
                color:  qgcPal.windowShade
            }

            Rectangle {
                x:      returnHomeGrid.x + outerGrid.x - _margins
                y:      returnHomeGrid.y + outerGrid.y - _margins
                width:  returnHomeGrid.width + (_margins * 2)
                height: returnHomeGrid.height + (_margins * 2)
                color:  qgcPal.windowShade
            }

            Rectangle {
                x:      maxVertGrid.x + outerGrid.x - _margins
                y:      maxVertGrid.y + outerGrid.y - _margins
                width:  maxVertGrid.width + (_margins * 2)
                height: maxVertGrid.height + (_margins * 2)
                color:  qgcPal.windowShade
            }

            Rectangle {
                id:     lastRect
                x:      maxHorzGrid.x + outerGrid.x - _margins
                y:      maxHorzGrid.y + outerGrid.y - _margins
                width:  maxHorzGrid.width + (_margins * 2)
                height: maxHorzGrid.height + (_margins * 2)
                color:  qgcPal.windowShade
            }

            GridLayout {
                id:         outerGrid
                columns:    3
                anchors.horizontalCenter:   parent.horizontalCenter

                QGCLabel {
                    text:               qsTr("Geofence Failsafe Trigger")
                    Layout.columnSpan:  3
                }

                Item { width: 1; height: _margins; Layout.columnSpan: 3 }

                Item { width: _margins; height: 1 }

                GridLayout {
                    id:         geoFenceGrid
                    columns:    3

                    Image {
                        mipmap:             true
                        fillMode:           Image.PreserveAspectFit
                        source:             qgcPal.globalTheme === qgcPal.Light ? "/qmlimages/GeoFenceLight.svg" : "/qmlimages/GeoFence.svg"
                        Layout.rowSpan:     3
                        Layout.maximumWidth:    _imageWidth
                        Layout.maximumHeight:   _imageHeight
                        width:                  _imageWidth
                        height:                 _imageHeight
                    }

                    QGCLabel {
                        text:               qsTr("Action on breach:")
                        Layout.fillWidth:   true
                    }
                    FactComboBox {
                        fact:                   _fenceAction
                        indexModel:             false
                        Layout.minimumWidth:    _editFieldWidth
                    }

                    QGCCheckBox {
                        id:                 fenceRadiusCheckBox
                        text:               qsTr("Max Radius:")
                        checked:            _fenceRadius.value > 0
                        onClicked:          _fenceRadius.value = checked ? 100 : 0
                        Layout.fillWidth:   true
                    }
                    FactTextField {
                        fact:                   _fenceRadius
                        enabled:                fenceRadiusCheckBox.checked
                        Layout.minimumWidth:    _editFieldWidth
                    }

                    QGCCheckBox {
                        id:                 fenceAltMaxCheckBox
                        text:               qsTr("Max Altitude:")
                        checked:            _fenceAlt ? _fenceAlt.value > 0 : false
                        onClicked:          _fenceAlt.value = checked ? 100 : 0
                        Layout.fillWidth:   true
                    }
                    FactTextField {
                        fact:                   _fenceAlt
                        enabled:                fenceAltMaxCheckBox.checked
                        Layout.minimumWidth:    _editFieldWidth
                    }
                }

                Item { width: 1; height: _margins; Layout.columnSpan: 3 }

                QGCLabel {
                    text:               qsTr("Return Home Settings")
                    Layout.columnSpan:  3
                }

                Item { width: 1; height: _margins; Layout.columnSpan: 3 }

                Item { width: _margins; height: 1 }

                GridLayout {
                    id:         returnHomeGrid
                    columns:    3
                    QGCColoredImage {
                        color:                  qgcPal.text
                        mipmap:                 true
                        fillMode:               Image.PreserveAspectFit
                        source:                 controller.vehicle.fixedWing ? "/qmlimages/ReturnToHomeAltitude.svg" : "/qmlimages/ReturnToHomeAltitudeCopter.svg"
                        Layout.maximumWidth:    _imageWidth
                        Layout.maximumHeight:   _imageHeight
                        width:                  _imageWidth
                        height:                 _imageHeight
                    }
                    QGCLabel {
                        text:                   qsTr("Climb to altitude of:")
                        Layout.fillWidth:       true
                        Layout.alignment:       Qt.AlignVCenter
                    }
                    FactTextField {
                        fact:                   controller.getParameterFact(-1, "RTL_RETURN_ALT")
                        Layout.minimumWidth:    _editFieldWidth
                        Layout.alignment:       Qt.AlignVCenter
                    }
                }

                Item { width: 1; height: _margins; Layout.columnSpan: 3 }

                QGCLabel {
                    text:               qsTr("Max Vertical Velocity (Manual Flight)")
                    Layout.columnSpan:  3
                }

                Item { width: 1; height: _margins; Layout.columnSpan: 3 }

                Item { width: _margins; height: 1 }

                GridLayout {
                    id:         maxVertGrid
                    columns:    3
                    Image {
                        mipmap:                 true
                        fillMode:               Image.PreserveAspectFit
                        source:                 "/typhoonh/img/TyphoonUpDown.svg"
                        Layout.rowSpan:         3
                        Layout.maximumWidth:    _imageWidth
                        Layout.maximumHeight:   _imageHeight
                        width:                  _imageWidth
                        height:                 _imageHeight
                    }
                    QGCLabel {
                        text:                   qsTr("Max Climb Velocity:")
                        Layout.fillWidth:       true
                        Layout.alignment:       Qt.AlignVCenter
                    }
                    FactTextField {
                        fact:                   _maxVelUp
                        Layout.minimumWidth:    _editFieldWidth
                        Layout.alignment:       Qt.AlignVCenter
                    }
                    QGCLabel {
                        text:                   qsTr("Max Descent Velocity:")
                        Layout.fillWidth:       true
                        Layout.alignment:       Qt.AlignVCenter
                    }
                    FactTextField {
                        fact:                   _maxVelDn
                        Layout.minimumWidth:    _editFieldWidth
                        Layout.alignment:       Qt.AlignVCenter
                    }
                }

                Item { width: 1; height: _margins; Layout.columnSpan: 3 }

                QGCLabel {
                    text:               qsTr("Max Horizontal Velocity (Manual Flight)")
                    Layout.columnSpan:  3
                }

                Item { width: 1; height: _margins; Layout.columnSpan: 3 }

                Item { width: _margins; height: 1 }

                GridLayout {
                    id:         maxHorzGrid
                    columns:    3
                    Image {
                        mipmap:                 true
                        fillMode:               Image.PreserveAspectFit
                        source:                 "/typhoonh/img/TyphoonHorizontal.svg"
                        Layout.maximumWidth:    _imageWidth
                        Layout.maximumHeight:   _imageHeight
                        width:                  _imageWidth
                        height:                 _imageHeight
                    }
                    QGCLabel {
                        text:                   qsTr("Max Horizontal Velocity:")
                        Layout.fillWidth:       true
                        Layout.alignment:       Qt.AlignVCenter
                    }
                    FactTextField {
                        fact:                   _maxVelHz
                        Layout.minimumWidth:    _editFieldWidth
                        Layout.alignment:       Qt.AlignVCenter
                    }
                }

                Item { width: 1; height: _margins; Layout.columnSpan: 3 }

            }
        }
    }
}
