/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick  2.12
import QtCharts 2.3

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0

Rectangle {
    id:         root
    radius:     ScreenTools.defaultFontPixelWidth * 0.5
    color:      qgcPal.window
    opacity:    0.80
    clip:       true

    property var missionController

    signal setCurrentSeqNum(int seqNum)

    property real _margins:             ScreenTools.defaultFontPixelWidth / 2
    property var  _visualItems:         missionController.visualItems
    property real _altRange:            _maxAMSLAltitude - _minAMSLAltitude
    property real _indicatorSpacing:    5
    property real _minAMSLAltitude:     isNaN(terrainProfile.minAMSLAlt) ? 0 : terrainProfile.minAMSLAlt
    property real _maxAMSLAltitude:     isNaN(terrainProfile.maxAMSLAlt) ? 100 : terrainProfile.maxAMSLAlt
    property real _missionDistance:     isNaN(missionController.missionDistance) ? 100 : missionController.missionDistance
    property var  _unitsConversion:     QGroundControl.unitsConversion

    QGCPalette { id: qgcPal }

    QGCLabel {
        id:                     titleLabel
        anchors.top:            parent.bottom
        width:                  parent.height
        font.pointSize:         ScreenTools.smallFontPointSize
        text:                   qsTr("Height AMSL (%1)").arg(_unitsConversion.appSettingsHorizontalDistanceUnitsString)
        horizontalAlignment:    Text.AlignHCenter
        rotation:               -90
        transformOrigin:        Item.TopLeft
    }

    QGCFlickable {
        id:                 terrainProfileFlickable
        //anchors.margins:    _margins
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        anchors.leftMargin: titleLabel.contentHeight
        anchors.left:       parent.left
        anchors.right:      parent.right
        clip:               true

        Item {
            height: terrainProfileFlickable.height
            width:  terrainProfileFlickable.width

            ChartView {
                id:                 chart
                anchors.fill:       parent
                margins.top:        0
                margins.right:      0
                margins.bottom:     0
                margins.left:       0
                backgroundColor:    "transparent"
                legend.visible:     false
                antialiasing:       true

                ValueAxis {
                    id:                         axisX
                    min:                        0
                    max:                        _unitsConversion.metersToAppSettingsHorizontalDistanceUnits(missionController.missionDistance)
                    lineVisible:                true
                    labelsFont.family:          "Fixed"
                    labelsFont.pointSize:       ScreenTools.smallFontPointSize
                    labelsColor:                "white"
                    tickCount:                  5
                    gridLineColor:              "#44FFFFFF"
                }

                ValueAxis {
                    id:                         axisY
                    min:                        _unitsConversion.metersToAppSettingsVerticalDistanceUnits(_minAMSLAltitude)
                    max:                        _unitsConversion.metersToAppSettingsVerticalDistanceUnits(_maxAMSLAltitude)
                    lineVisible:                true
                    labelsFont.family:          "Fixed"
                    labelsFont.pointSize:       ScreenTools.smallFontPointSize
                    labelsColor:                "white"
                    tickCount:                  4
                    gridLineColor:              "#44FFFFFF"
                }

                LineSeries {
                    id:         lineSeries
                    axisX:      axisX
                    axisY:      axisY
                    visible:    true

                    XYPoint { x: 0; y: _unitsConversion.metersToAppSettingsVerticalDistanceUnits(_minAMSLAltitude) }
                    XYPoint { x: _unitsConversion.metersToAppSettingsHorizontalDistanceUnits(_missionDistance); y: _unitsConversion.metersToAppSettingsVerticalDistanceUnits(_maxAMSLAltitude) }
                }
            }

            TerrainProfile {
                id:                 terrainProfile
                x:                  chart.plotArea.x
                y:                  chart.plotArea.y
                height:             chart.plotArea.height
                visibleWidth:       chart.plotArea.width
                missionController:  root.missionController

                Repeater {
                    model: missionController.visualItems

                    Item {
                        id:             topLevelItem
                        anchors.fill:   parent
                        visible:        object.specifiesCoordinate && !object.standaloneCoordinate

                        Rectangle {
                            id:         simpleItem
                            height:     terrainProfile.height
                            width:      1
                            color:      "white"
                            x:          (object.distanceFromStart * terrainProfile.pixelsPerMeter)
                            visible:    object.isSimpleItem || object.isSingleItem

                            MissionItemIndexLabel {
                                anchors.horizontalCenter:   parent.horizontalCenter
                                anchors.bottom:             parent.bottom
                                small:                      true
                                checked:                    object.isCurrentItem
                                label:                      object.abbreviation.charAt(0)
                                index:                      object.abbreviation.charAt(0) > 'A' && object.abbreviation.charAt(0) < 'z' ? -1 : object.sequenceNumber
                                onClicked:                  root.setCurrentSeqNum(object.sequenceNumber)
                            }
                        }

                        Rectangle {
                            id:         complexItemEntry
                            height:     terrainProfile.height
                            width:      1
                            color:      "white"
                            x:          (object.distanceFromStart * terrainProfile.pixelsPerMeter)
                            visible:    complexItem.visible

                            MissionItemIndexLabel {
                                anchors.horizontalCenter:   parent.horizontalCenter
                                anchors.bottom:             parent.bottom
                                small:                      true
                                checked:                    object.isCurrentItem
                                index:                      object.sequenceNumber
                                onClicked:                  root.setCurrentSeqNum(object.sequenceNumber)
                            }
                        }

                        Rectangle {
                            id:         complexItemExit
                            height:     terrainProfile.height
                            width:      1
                            color:      "white"
                            x:          ((object.distanceFromStart + object.complexDistance) * terrainProfile.pixelsPerMeter)
                            visible:    complexItem.visible

                            MissionItemIndexLabel {
                                anchors.horizontalCenter:   parent.horizontalCenter
                                anchors.bottom:             parent.bottom
                                small:                      true
                                checked:                    object.isCurrentItem
                                index:                      object.lastSequenceNumber
                                onClicked:                  root.setCurrentSeqNum(object.sequenceNumber)
                            }
                        }

                        Rectangle {
                            id:             complexItem
                            anchors.bottom: parent.bottom
                            x:              (object.distanceFromStart * terrainProfile.pixelsPerMeter)
                            width:          complexItem.visible ? object.complexDistance * terrainProfile.pixelsPerMeter : 0
                            height:         patternNameLabel.height
                            color:          "green"
                            opacity:        0.5
                            visible:        !object.isSimpleItem && !object.isSingleItem

                            QGCMouseArea {
                                anchors.fill:   parent
                                onClicked:      root.setCurrentSeqNum(object.sequenceNumber)
                            }

                            QGCLabel {
                                id:                         patternNameLabel
                                anchors.horizontalCenter:   parent.horizontalCenter
                                text:                       complexItem.visible ? object.patternName : ""
                            }
                        }
                    }
                }
            }
        }
    }
}
