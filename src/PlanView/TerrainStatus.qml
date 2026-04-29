import QtQuick
import QtGraphs

import QGroundControl
import QGroundControl.Controls

Rectangle {
    id:         root
    radius:     ScreenTools.defaultFontPixelWidth * 0.5
    color:      qgcPal.window
    opacity:    0.80
    clip:       true

    property var missionController

    signal setCurrentSeqNum(int seqNum)

    property real _margins:                 ScreenTools.defaultFontPixelWidth / 2
    property var  _visualItems:             missionController.visualItems
    property real _altRange:                _maxAMSLAltitude - _minAMSLAltitude
    property real _indicatorSpacing:        5
    property real _minAMSLAltitude:         isNaN(terrainProfile.minAMSLAlt) ? 0 : terrainProfile.minAMSLAlt
    property real _maxAMSLAltitude:         isNaN(terrainProfile.maxAMSLAlt) ? 100 : terrainProfile.maxAMSLAlt
    property real _missionTotalDistance:    isNaN(missionController.missionTotalDistance) ? 100 : missionController.missionTotalDistance
    property var  _unitsConversion:         QGroundControl.unitsConversion

    QGCPalette { id: qgcPal }

    QGCLabel {
        id:                     titleLabel
        anchors.top:            parent.bottom
        width:                  parent.height
        font.pointSize:         ScreenTools.smallFontPointSize
        text:                   qsTr("Height AMSL (%1)").arg(_unitsConversion.appSettingsVerticalDistanceUnitsString)
        horizontalAlignment:    Text.AlignHCenter
        rotation:               -90
        transformOrigin:        Item.TopLeft
    }

    QGCFlickable {
        id:                 terrainProfileFlickable
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        anchors.leftMargin: titleLabel.contentHeight
        anchors.left:       parent.left
        anchors.right:      parent.right
        clip:               true

        Item {
            height: terrainProfileFlickable.height
            width:  terrainProfileFlickable.width

            GraphsView {
                id:                 chart
                anchors.fill:       parent
                marginTop:          ScreenTools.defaultFontPixelHeight / 2  // Fixes top clipping problem
                marginRight:        ScreenTools.defaultFontPixelWidth * 2   // Prevents clipping last tick mark
                marginBottom:       -ScreenTools.defaultFontPixelHeight / 2 // For some reason you can't get rid of bottom margin by setting to 0
                marginLeft:         0

                theme: GraphsTheme {
                    colorScheme:                qgcPal.globalTheme === QGCPalette.Light ? GraphsTheme.ColorScheme.Light : GraphsTheme.ColorScheme.Dark
                    backgroundColor:            "transparent"
                    backgroundVisible:          false
                    plotAreaBackgroundColor:     qgcPal.window
                    grid.mainColor:             applyOpacity(qgcPal.text, 0.5)
                    grid.subColor:              applyOpacity(qgcPal.text, 0.3)
                    grid.mainWidth:             1
                    labelBackgroundVisible:     false
                    labelTextColor:             qgcPal.text
                    axisXLabelFont.family:      ScreenTools.fixedFontFamily
                    axisXLabelFont.pointSize:   ScreenTools.smallFontPointSize
                    axisYLabelFont.family:      ScreenTools.fixedFontFamily
                    axisYLabelFont.pointSize:   ScreenTools.smallFontPointSize
                }

                axisX: ValueAxis {
                    id:                         axisX
                    min:                        0
                    max:                        _unitsConversion.metersToAppSettingsHorizontalDistanceUnits(_missionTotalDistance)
                    lineVisible:                true
                    tickInterval:               max > 0 ? max / 4 : 1
                    labelDecimals:              1
                }

                axisY: ValueAxis {
                    id:                         axisY
                    min:                        _unitsConversion.metersToAppSettingsVerticalDistanceUnits(_minAMSLAltitude)
                    max:                        _unitsConversion.metersToAppSettingsVerticalDistanceUnits(_maxAMSLAltitude)
                    lineVisible:                true
                    tickInterval:               (max - min) > 0 ? (max - min) / 3 : 1
                    labelDecimals:              1
                }

                LineSeries {
                    id:         terrainSeries
                    color:      "green"
                    width:      2
                }

                LineSeries {
                    id:         flightSeries
                    color:      "orange"
                    width:      2
                }

                LineSeries {
                    id:         missingSeries
                    color:      "yellow"
                    width:      2
                }

                LineSeries {
                    id:         collisionSeries
                    color:      "red"
                    width:      3
                }
            }

            TerrainProfile {
                id:                 terrainProfile
                x:                  chart.plotArea.x
                y:                  chart.plotArea.y
                height:             chart.plotArea.height
                visibleWidth:       chart.plotArea.width
                missionController:  root.missionController
                horizontalScale:    _unitsConversion.metersToAppSettingsHorizontalDistanceUnits(1)
                verticalScale:      _unitsConversion.metersToAppSettingsVerticalDistanceUnits(1)

                onProfileChanged:   terrainProfile.updateSeries(terrainSeries, flightSeries, missingSeries, collisionSeries)

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
                            color:      qgcPal.text
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
                            color:      qgcPal.text
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
                            color:      qgcPal.text
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

    function applyOpacity(colorIn, opacity){
        return Qt.rgba(colorIn.r, colorIn.g, colorIn.b, opacity)
    }
}
