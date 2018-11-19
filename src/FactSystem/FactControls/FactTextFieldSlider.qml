import QtQuick              2.7
import QtQuick.Controls     1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Layouts          1.2

import QGroundControl.FactSystem    1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.FactControls  1.0


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
        slide.minimumValue = fact.min
        slide.maximumValue = fact.max
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
                font.family:            ScreenTools.demiboldFontFamily
                font.pointSize:         ScreenTools.defaultFontPointSize * 1.1
                anchors.verticalCenter: parent.verticalCenter
            }

            // Row container for Value: xx.xx +/- (different spacing than parent)
            Row {
                spacing:                ScreenTools.defaultFontPixelWidth
                anchors.verticalCenter: parent.verticalCenter

                QGCLabel {
                    text:                   "Value: "
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
                    onWheel: {
                        // do nothing
                        wheel.accepted = true;
                    }
                    onPressed: {
                        // propogate/accept
                        mouse.accepted = false;
                    }
                    onReleased: {
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
