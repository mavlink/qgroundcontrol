/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Dialogs  1.2
import QtQuick.Layouts  1.2

import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.Controllers   1.0
import QGroundControl.ScreenTools   1.0

SetupPage {
    id:             airframePage
    pageComponent:  pageComponent

    Component {
        id: pageComponent

        ColumnLayout {
            id:     mainColumn
            width:  availableWidth

            property real _minW:                ScreenTools.defaultFontPixelWidth * 20
            property real _boxWidth:            _minW
            property real _boxSpace:            ScreenTools.defaultFontPixelWidth
            property real _margins:             ScreenTools.defaultFontPixelWidth
            property Fact _frameClass:          controller.getParameterFact(-1, "FRAME_CLASS")
            property Fact _frameType:           controller.getParameterFact(-1, "FRAME_TYPE", false)    // FRAME_TYPE is not available on all Rover versions
            property bool _frameTypeAvailable:  controller.parameterExists(-1, "FRAME_TYPE")

            readonly property real spacerHeight: ScreenTools.defaultFontPixelHeight

            onWidthChanged:         computeDimensions()
            Component.onCompleted:  computeDimensions()

            function computeDimensions() {
                var sw  = 0
                var rw  = 0
                var idx = Math.floor(mainColumn.width / (_minW + ScreenTools.defaultFontPixelWidth))
                if(idx < 1) {
                    _boxWidth = mainColumn.width
                    _boxSpace = 0
                } else {
                    _boxSpace = 0
                    if(idx > 1) {
                        _boxSpace = ScreenTools.defaultFontPixelWidth
                        sw = _boxSpace * (idx - 1)
                    }
                    rw = mainColumn.width - sw
                    _boxWidth = rw / idx
                }
            }

            APMAirframeComponentController { id: controller; }

            QGCLabel {
                id:                 helpText
                Layout.fillWidth:   true
                text:               (_frameClass.rawValue === 0 ?
                                         qsTr("Airframe is currently not set.") :
                                         qsTr("Currently set to frame class '%1'").arg(_frameClass.enumStringValue) +
                                         (_frameTypeAvailable ?  qsTr(" and frame type '%2'").arg(_frameType.enumStringValue) : "") +
                                         qsTr(".", "period for end of sentence")) +
                                    qsTr(" To change this configuration, select the desired frame class below and frame type.")
                font.family:        ScreenTools.demiboldFontFamily
                wrapMode:           Text.WordWrap
            }

            Item {
                id:             lastSpacer
                height:         parent.spacerHeight
                width:          10
            }

            Flow {
                id:                 flowView
                Layout.fillWidth:   true
                spacing:            _boxSpace

                ExclusiveGroup {
                    id: airframeTypeExclusive
                }

                Repeater {
                    model: controller.frameClassModel

                    // Outer summary item rectangle
                    Rectangle {
                        id:     outerRect
                        width:  _boxWidth
                        height: ScreenTools.defaultFontPixelHeight * 14
                        color:  qgcPal.window

                        readonly property real titleHeight: ScreenTools.defaultFontPixelHeight * 1.75
                        readonly property real innerMargin: ScreenTools.defaultFontPixelWidth

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                if (!airframeCheckBox.checked || !combo.valid) {
                                    if (_frameTypeAvailable && object.defaultFrameType != -1) {
                                        _frameType.rawValue = object.defaultFrameType
                                    }
                                    airframeCheckBox.checked = true
                                }
                            }
                        }

                        QGCLabel {
                            id:     title
                            text:   object.name
                        }

                        Rectangle {
                            id:                 imageComboRect
                            anchors.topMargin:  ScreenTools.defaultFontPixelHeight / 2
                            anchors.top:        title.bottom
                            anchors.bottom:     parent.bottom
                            anchors.left:       parent.left
                            anchors.right:      parent.right
                            color:              airframeCheckBox.checked ? qgcPal.buttonHighlight : qgcPal.windowShade
                            opacity:            combo.valid ? 1.0 : 0.5

                            ColumnLayout {
                                anchors.margins:    innerMargin
                                anchors.fill:       parent
                                spacing:            innerMargin

                                Image {
                                    id:                 image
                                    Layout.fillWidth:   true
                                    Layout.fillHeight:  true
                                    fillMode:           Image.PreserveAspectFit
                                    smooth:             true
                                    antialiasing:       true
                                    sourceSize.width:   width
                                    source:             airframeCheckBox.checked ? object.imageResource : object.imageResourceDefault
                                }

                                QGCCheckBox {
                                    // Although this item is invisible we still use it to manage state
                                    id:             airframeCheckBox
                                    checked:        object.frameClass === _frameClass.rawValue
                                    exclusiveGroup: airframeTypeExclusive
                                    visible:        false

                                    onCheckedChanged: {
                                        if (checked) {
                                            _frameClass.rawValue = object.frameClass
                                        }
                                    }
                                }

                                QGCLabel {
                                    text:           qsTr("Frame Type")
                                    font.pointSize: ScreenTools.smallFontPointSize
                                    color:          qgcPal.buttonHighlightText
                                    visible:        airframeCheckBox.checked && object.frameTypeSupported
                                }

                                QGCComboBox {
                                    id:                 combo
                                    Layout.fillWidth:   true
                                    model:              object.frameTypeEnumStrings
                                    visible:            airframeCheckBox.checked && object.frameTypeSupported
                                    onActivated:        _frameType.rawValue = object.frameTypeEnumValues[index]

                                    property bool valid: true

                                    function checkFrameType(value) {
                                        return value == _frameType.rawValue
                                    }

                                    function selectFrameType() {
                                        var index = object.frameTypeEnumValues.findIndex(checkFrameType)
                                        if (index == -1 && combo.visible) {
                                            // Frame Class/Type is set to an invalid combination
                                            combo.valid = false
                                        } else {
                                            combo.currentIndex = index
                                            combo.valid = true
                                        }
                                    }

                                    Component.onCompleted: selectFrameType()

                                    Connections {
                                        target:                 _frameTypeAvailable ? _frameType : null
                                        ignoreUnknownSignals:   true
                                        onRawValueChanged:      combo.selectFrameType()
                                    }
                                }
                            }
                        }

                        QGCLabel {
                            anchors.fill:   imageComboRect
                            text:           qsTr("Invalid setting for FRAME_TYPE. Click to Reset.")
                            wrapMode:       Text.WordWrap
                            visible:        !combo.valid
                        }
                    }
                } // Repeater - summary boxes
            } // Flow - summary boxes
        } // Column
    } // Component
} // SetupPage
