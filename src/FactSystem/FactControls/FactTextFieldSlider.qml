import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl.FactSystem
import QGroundControl.Controls
import QGroundControl.ScreenTools
import QGroundControl.FactControls


Row {
    id: sliderRoot
    width: parent.width

    property Fact   fact:           null
    property var    _factValue:     fact ? fact.value : null
    property bool   _loadComplete:  false

    property real   _range:         Math.abs(fact.max - fact.min)
    property real   _minIncrement:  _range/50
    property int    precision:      2

    on_FactValueChanged: {
        slide.value = fact.value
    }

    Component.onCompleted: {
        slide.from = fact.min
        slide.to = fact.max
        slide.value = fact.value
        _loadComplete = true
    }

    // Used to find width of value string
    QGCLabel {
        id:      textMeasure
        visible: false
        text:    fact.value.toFixed(precision)
    }

    // Param name, value, description and slider adjustment
    Column {
        id:       sliderColumn
        width:    parent.width
        spacing:  _margins/2

        // Param name and value
        Row {
            spacing: _margins

            QGCLabel {
                text:                   fact.name
                font.bold:              true
                font.pointSize:         ScreenTools.defaultFontPointSize * 1.1
                anchors.verticalCenter: parent.verticalCenter
            }

            // Row container for Value: xx.xx +/- (different spacing than parent)
            Row {
                spacing:                ScreenTools.defaultFontPixelWidth
                anchors.verticalCenter: parent.verticalCenter

                QGCLabel {
                    text:                   qsTr("Value: ")
                    anchors.verticalCenter: parent.verticalCenter
                }

                FactTextField {
                    anchors.verticalCenter: parent.verticalCenter
                    fact:                   sliderRoot.fact
                    showUnits:              false
                    showHelp:               false
                    text:                   fact.value.toFixed(precision)
                    width:                  textMeasure.width + ScreenTools.defaultFontPixelWidth*2 // Fudged, nothing else seems to work
                }

                QGCLabel {
                    text:                   fact.units
                    anchors.verticalCenter: parent.verticalCenter
                }

                QGCButton {
                    height:                 parent.height
                    width:                  height
                    text:                   "-"
                    anchors.verticalCenter: parent.verticalCenter

                    onClicked: fact.value = Math.max(Math.min(fact.value - _minIncrement, fact.max), fact.min)
                }

                QGCButton {
                    height:                 parent.height
                    width:                  height
                    text:                   "+"
                    anchors.verticalCenter: parent.verticalCenter

                    onClicked: fact.value = Math.max(Math.min(fact.value + _minIncrement, fact.max), fact.min)
                }
            } // Row - container for Value: xx.xx +/- (different spacing than parent)
        } // Row - Param name and value

        QGCLabel {
            text: fact.shortDescription
        }

        // Slider, with minimum and maximum values labeled
        Row {
            width:      parent.width
            spacing:    _margins

            QGCLabel {
                id:                  minLabel
                width:               ScreenTools.defaultFontPixelWidth * 10
                text:                fact.min.toFixed(precision)
                horizontalAlignment: Text.AlignRight
            }

            QGCSlider {
                id:                 slide
                width:              parent.width - minLabel.width - maxLabel.width - _margins * 2
                stepSize:           fact.increment ? Math.max(fact.increment, _minIncrement) : _minIncrement
                tickmarksEnabled:   true

                onValueChanged: {
                    if (_loadComplete) {
                        if (Math.abs(fact.value - value) >= _minIncrement) { // prevent binding loop
                            fact.value = value
                        }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onWheel: (wheel) => {
                        // do nothing
                        wheel.accepted = true;
                    }
                    onPressed: (mouse) => {
                        // propogate/accept
                        mouse.accepted = false;
                    }
                    onReleased: (mouse) => {
                        // propogate/accept
                        mouse.accepted = false;
                    }
                }
            } // Slider

            QGCLabel {
                id:     maxLabel
                width:  ScreenTools.defaultFontPixelWidth * 10
                text:   fact.max.toFixed(precision)
            }
        } // Row - Slider with minimum and maximum values labeled
    } // Column - Param name, value, description and slider adjustment
} // Row
