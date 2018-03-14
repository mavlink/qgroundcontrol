import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts  1.2

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0

QGCFlickable {
    id:             root
    width:          availableWidth
    height:         availableHeight
    contentHeight:  editorColumn.height
    clip:           true

    property real   availableWidth
    property real   availableHeight
    property var    myGeoFenceController
    property var    flightMap

    readonly property real  _editFieldWidth:    Math.min(width - _margin * 2, ScreenTools.defaultFontPixelWidth * 15)
    readonly property real  _margin:            ScreenTools.defaultFontPixelWidth / 2
    readonly property real  _radius:            ScreenTools.defaultFontPixelWidth / 2

    Column {
        id:             editorColumn
        anchors.left:   parent.left
        anchors.right:  parent.right

        Rectangle {
            id:     geoFenceEditorRect
            width:  parent.width
            height: geoFenceItems.y + geoFenceItems.height + (_margin * 2)
            radius: _radius
            color:  qgcPal.missionItemEditor

            QGCLabel {
                id:                 geoFenceLabel
                anchors.margins:    _margin
                anchors.left:       parent.left
                anchors.top:        parent.top
                text:               qsTr("GeoFence")
                color:              "black"
            }

            Rectangle {
                id:                 geoFenceItems
                anchors.margins:    _margin
                anchors.left:       parent.left
                anchors.right:      parent.right
                anchors.top:        geoFenceLabel.bottom
                height:             fenceColumn.y + fenceColumn.height + (_margin * 2)
                color:              qgcPal.windowShadeDark
                radius:             _radius

                Column {
                    id:                 fenceColumn
                    anchors.margins:    _margin
                    anchors.top:        parent.top
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    spacing:            ScreenTools.defaultFontPixelHeight / 2

                    QGCLabel {
                        anchors.left:       parent.left
                        anchors.right:      parent.right
                        wrapMode:           Text.WordWrap
                        font.pointSize:     myGeoFenceController.supported ? ScreenTools.smallFontPointSize : ScreenTools.defaultFontPointSize
                        text:               myGeoFenceController.supported ?
                                                qsTr("GeoFencing allows you to set a virtual ‘fence’ around the area you want to fly in.") :
                                                qsTr("This vehicle does not support GeoFence.")
                    }

                    Column {
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        spacing:        ScreenTools.defaultFontPixelHeight / 2
                        visible:        myGeoFenceController.supported

                        Repeater {
                            model: myGeoFenceController.params

                            Item {
                                width:  fenceColumn.width
                                height: textField.height

                                property bool showCombo: modelData.enumStrings.length > 0

                                QGCLabel {
                                    id:                 textFieldLabel
                                    anchors.baseline:   textField.baseline
                                    text:               myGeoFenceController.paramLabels[index]
                                }

                                FactTextField {
                                    id:             textField
                                    anchors.right:  parent.right
                                    width:          _editFieldWidth
                                    showUnits:      true
                                    fact:           modelData
                                    visible:        !parent.showCombo
                                }

                                FactComboBox {
                                    id:             comboField
                                    anchors.right:  parent.right
                                    width:          _editFieldWidth
                                    indexModel:     false
                                    fact:           showCombo ? modelData : _nullFact
                                    visible:        parent.showCombo

                                    property var _nullFact: Fact { }
                                }
                            }
                        }

                        SectionHeader {
                            id:     insertSection
                            text:   qsTr("Insert GeoFence")
                        }

                        QGCButton {
                            anchors.left:   parent.left
                            anchors.right:  parent.right
                            text:           qsTr("Polygon Fence")

                            onClicked: {
                                var rect = Qt.rect(flightMap.centerViewport.x, flightMap.centerViewport.y, flightMap.centerViewport.width, flightMap.centerViewport.height)
                                var topLeftCoord = flightMap.toCoordinate(Qt.point(rect.x, rect.y), false /* clipToViewPort */)
                                var bottomRightCoord = flightMap.toCoordinate(Qt.point(rect.x + rect.width, rect.y + rect.height), false /* clipToViewPort */)
                                myGeoFenceController.addInclusionPolygon(topLeftCoord, bottomRightCoord)
                            }
                        }

                        QGCButton {
                            anchors.left:   parent.left
                            anchors.right:  parent.right
                            text:           qsTr("Circular Fence")

                            onClicked: {
                                var rect = Qt.rect(flightMap.centerViewport.x, flightMap.centerViewport.y, flightMap.centerViewport.width, flightMap.centerViewport.height)
                                var topLeftCoord = flightMap.toCoordinate(Qt.point(rect.x, rect.y), false /* clipToViewPort */)
                                var bottomRightCoord = flightMap.toCoordinate(Qt.point(rect.x + rect.width, rect.y + rect.height), false /* clipToViewPort */)
                                myGeoFenceController.addInclusionCircle(topLeftCoord, bottomRightCoord)
                            }
                        }

                        SectionHeader {
                            id:     polygonSection
                            text:   qsTr("Polygon Fences")
                        }

                        QGCLabel {
                            text:       qsTr("None")
                            visible:    polygonSection.checked && myGeoFenceController.polygons.count === 0
                        }

                        GridLayout {
                            anchors.left:   parent.left
                            anchors.right:  parent.right
                            columns:        3
                            flow:           GridLayout.TopToBottom
                            visible:        polygonSection.checked && myGeoFenceController.polygons.count > 0

                            QGCLabel {
                                text:               qsTr("Inclusion")
                                Layout.column:      0
                                Layout.alignment:   Qt.AlignHCenter
                            }

                            Repeater {
                                model: myGeoFenceController.polygons

                                QGCCheckBox {
                                    checked:            object.inclusion
                                    onClicked:          object.inclusion = checked
                                    Layout.alignment:   Qt.AlignHCenter
                                }
                            }

                            QGCLabel {
                                text:               qsTr("Edit")
                                Layout.column:      1
                                Layout.alignment:   Qt.AlignHCenter
                            }

                            Repeater {
                                model: myGeoFenceController.polygons

                                QGCRadioButton {
                                    checked:            _interactive
                                    Layout.alignment:   Qt.AlignHCenter

                                    property bool _interactive: object.interactive

                                    on_InteractiveChanged: checked = _interactive

                                    onClicked: {
                                        myGeoFenceController.clearAllInteractive()
                                        object.interactive = checked
                                    }
                                }
                            }

                            QGCLabel {
                                text:               qsTr("Delete")
                                Layout.column:      2
                                Layout.alignment:   Qt.AlignHCenter
                            }

                            Repeater {
                                model: myGeoFenceController.polygons

                                QGCColoredImage {
                                    width:              ScreenTools.defaultFontPixelHeight
                                    height:             width
                                    sourceSize.height:  width
                                    source:             "/res/XDelete.svg"
                                    fillMode:           Image.PreserveAspectFit
                                    color:              qgcPal.text
                                    Layout.alignment:   Qt.AlignHCenter

                                    property int _polygonIndex: index

                                    QGCMouseArea {
                                        fillItem:   parent
                                        onClicked:  myGeoFenceController.deletePolygon(parent._polygonIndex)
                                    }
                                }
                            }
                        } // GridLayout

                        SectionHeader {
                            id:     circleSection
                            text:   qsTr("Circular Fences")
                        }

                        QGCLabel {
                            text:       qsTr("None")
                            visible:    circleSection.checked && myGeoFenceController.circles.count === 0
                        }

                        GridLayout {
                            anchors.left:   parent.left
                            anchors.right:  parent.right
                            columns:        4
                            flow:           GridLayout.TopToBottom
                            visible:        polygonSection.checked && myGeoFenceController.circles.count > 0

                            QGCLabel {
                                text:               qsTr("Inclusion")
                                Layout.column:      0
                                Layout.alignment:   Qt.AlignHCenter
                            }

                            Repeater {
                                model: myGeoFenceController.circles

                                QGCCheckBox {
                                    checked:            object.inclusion
                                    onClicked:          object.inclusion = checked
                                    Layout.alignment:   Qt.AlignHCenter
                                }
                            }

                            QGCLabel {
                                text:               qsTr("Edit")
                                Layout.column:      1
                                Layout.alignment:   Qt.AlignHCenter
                            }

                            Repeater {
                                model: myGeoFenceController.circles

                                QGCRadioButton {
                                    checked:            _interactive
                                    Layout.alignment:   Qt.AlignHCenter

                                    property bool _interactive: object.interactive

                                    on_InteractiveChanged: checked = _interactive

                                    onClicked: {
                                        myGeoFenceController.clearAllInteractive()
                                        object.interactive = checked
                                    }
                                }
                            }

                            QGCLabel {
                                text:               qsTr("Radius")
                                Layout.column:      2
                                Layout.alignment:   Qt.AlignHCenter
                            }

                            Repeater {
                                model: myGeoFenceController.circles

                                FactTextField {
                                    fact:               object.radius
                                    Layout.fillWidth:   true
                                    Layout.alignment:   Qt.AlignHCenter
                                }
                            }

                            QGCLabel {
                                text:               qsTr("Delete")
                                Layout.column:      3
                                Layout.alignment:   Qt.AlignHCenter
                            }

                            Repeater {
                                model: myGeoFenceController.circles

                                QGCColoredImage {
                                    width:              ScreenTools.defaultFontPixelHeight
                                    height:             width
                                    sourceSize.height:  width
                                    source:             "/res/XDelete.svg"
                                    fillMode:           Image.PreserveAspectFit
                                    color:              qgcPal.text
                                    Layout.alignment:   Qt.AlignHCenter

                                    property int _circleIndex: index

                                    QGCMouseArea {
                                        fillItem:   parent
                                        onClicked:  myGeoFenceController.deleteCircle(parent._polygonIndex)
                                    }
                                }
                            }
                        } // GridLayout
                    }
                }
            }
        } // Rectangle
    }
}
