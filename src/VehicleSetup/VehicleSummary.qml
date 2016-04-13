/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

import QtQuick                  2.2
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2

import QGroundControl                       1.0
import QGroundControl.FactSystem            1.0
import QGroundControl.Controls              1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.Palette               1.0

Rectangle {
    id:             _summaryRoot
    anchors.fill:   parent
    color:          qgcPal.window

    property real _minSummaryW:     ScreenTools.defaultFontPixelWidth * 30
    property real _summaryBoxWidth: _minSummaryW
    property real _summaryBoxSpace: ScreenTools.defaultFontPixelWidth

    function computeSummaryBoxSize() {
        var sw  = 0
        var rw  = 0
        var idx = Math.floor(_summaryRoot.width / (_minSummaryW + ScreenTools.defaultFontPixelWidth))
        if(idx < 1) {
            _summaryBoxWidth = _summaryRoot.width
            _summaryBoxSpace = 0
        } else {
            _summaryBoxSpace = 0
            if(idx > 1) {
                _summaryBoxSpace = ScreenTools.defaultFontPixelWidth
                sw = _summaryBoxSpace * (idx - 1)
            }
            rw = _summaryRoot.width - sw
            _summaryBoxWidth = rw / idx
        }
    }

    function capitalizeWords(sentence) {
        return sentence.replace(/(?:^|\s)\S/g, function(a) { return a.toUpperCase(); });
    }

    QGCPalette {
        id:                 qgcPal
        colorGroupEnabled:  enabled
    }

    Component.onCompleted: {
        computeSummaryBoxSize()
    }

    onWidthChanged: {
        computeSummaryBoxSize()
    }

    QGCFlickable {
        clip:               true
        anchors.fill:       parent
        contentHeight:      summaryColumn.height
        contentWidth:       _summaryRoot.width
        flickableDirection: Flickable.VerticalFlick

        Column {
            id:             summaryColumn
            width:          _summaryRoot.width
            spacing:        ScreenTools.defaultFontPixelHeight

            QGCLabel {
                width:			parent.width
                wrapMode:		Text.WordWrap
                color:			setupComplete ? qgcPal.text : qgcPal.warningText
                font.weight:    Font.DemiBold
                text:           setupComplete ?
                    qsTr("Below you will find a summary of the settings for your vehicle. To the left are the setup menus for each component.") :
                    qsTr("WARNING: Your vehicle requires setup prior to flight. Please resolve the items marked in red using the menu on the left.")
                property bool setupComplete: QGroundControl.multiVehicleManager.activeVehicle ? QGroundControl.multiVehicleManager.activeVehicle.autopilot.setupComplete : false
            }

            Flow {
                id:         _flowCtl
                width:      _summaryRoot.width
                spacing:    _summaryBoxSpace

                Repeater {
                    model: QGroundControl.multiVehicleManager.activeVehicle ? QGroundControl.multiVehicleManager.activeVehicle.autopilot.vehicleComponents : undefined

                    // Outer summary item rectangle
                    Rectangle {
                        width:      _summaryBoxWidth
                        height:     ScreenTools.defaultFontPixelHeight * 13
                        color:      qgcPal.window
                        visible:    modelData.summaryQmlSource.toString() != ""

                        readonly property real titleHeight: ScreenTools.defaultFontPixelHeight * 2

                        // Title bar
                        Rectangle {
                            id:     titleBar
                            width:  parent.width
                            height: titleHeight
                            color:  qgcPal.windowShade

                            // Title text
                            QGCLabel {
                                anchors.fill:           parent
                                verticalAlignment:      TextEdit.AlignVCenter
                                horizontalAlignment:    TextEdit.AlignHCenter
                                text:                   capitalizeWords(modelData.name)
                            }

                            // Setup indicator
                            Rectangle {
                                anchors.rightMargin:    ScreenTools.defaultFontPixelWidth / 3
                                anchors.right:          parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                width:                  ScreenTools.defaultFontPixelWidth
                                height:                 width
                                radius:                 width / 2
                                color:                  modelData.setupComplete ? "#00d932" : "red"
                                visible:                modelData.requiresSetup
                            }
                        }

                        // Summary Qml
                        Rectangle {
                            anchors.top:    titleBar.bottom
                            width:          parent.width
                            Loader {
                                anchors.fill:   parent
                                source:         modelData.summaryQmlSource
                            }
                        }
                    }
                }
            }
        }
    }
}
