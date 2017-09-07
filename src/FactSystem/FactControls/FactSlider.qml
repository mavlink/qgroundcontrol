import QtQuick              2.7
import QtQuick.Controls     1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Layouts          1.2

import QGroundControl.FactSystem    1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.FactControls  1.0


Row {
    width: parent.width

    property Fact _fact: null
    property var _factValue: _fact ? _fact.value : null
    property bool _loadComplete: false

    property real _range: Math.abs(_fact.max - _fact.min)
    property real _minIncrement: _range/50

    on_FactValueChanged: {
        slide.value = _fact.value
    }

    Component.onCompleted: {
        slide.minimumValue= _fact.min
        slide.maximumValue= _fact.max
        slide.value = _fact.value

        _loadComplete = true
    }

    // Used to find width of value string
    QGCLabel {
        id: textMeasure
        visible: false
        text: _fact.value.toFixed(2)
    }

    // Param name, value, description and slider adjustment
    Column {
        id: sliderColumn
        width: parent.width
        spacing: _margins/2

        // Param name and value
        Row {
            spacing: _margins

            QGCLabel {
                text: _fact.name
                font.family: ScreenTools.demiboldFontFamily
                font.pointSize: ScreenTools.defaultFontPointSize * 1.1
                anchors.verticalCenter: parent.verticalCenter
            }

            // Row container for Value: xx.xx +/- (different spacing than parent)
            Row {
                spacing: ScreenTools.defaultFontPixelWidth
                anchors.verticalCenter: parent.verticalCenter

                QGCLabel {
                    text: "Value: "
                    anchors.verticalCenter: parent.verticalCenter
                }

//                TextMetrics {
//                    id: textMet
//                    text: _param.value.toFixed(2)
//                    font.pointSize: ScreenTools.defaultFontPointSize
//                }

                FactTextField {
                    anchors.verticalCenter: parent.verticalCenter
                    fact: _fact
                    text: _fact.value.toFixed(2)
                    textColor: qgcPal.text
                    width: textMeasure.width  + ScreenTools.defaultFontPixelWidth*2 // Fudged, nothing else seems to work
                    style: TextFieldStyle {
                        font.pointSize: ScreenTools.defaultFontPointSize
                        font.family: ScreenTools.normalFontFamily
                        background: Item {} // Nothing!
                    }
                }

                QGCLabel {
                    text: _fact.units
                    anchors.verticalCenter: parent.verticalCenter
                }

                QGCButton {
                    height: parent.height/2
                    width: height
                    text: "+"
                    anchors.verticalCenter: parent.verticalCenter
                    onClicked: {
                        _fact.value += _minIncrement
                    }
                }

                QGCButton {
                    height: parent.height/2
                    width: height
                    text: "-"
                    anchors.verticalCenter: parent.verticalCenter
                    onClicked: {
                        _fact.value -= _minIncrement
                    }
                }
            } // Row - container for Value: xx.xx +/- (different spacing than parent)
        } // Row - Param name and value

        QGCLabel {
            text: _fact.shortDescription
        }

        // Slider, with minimum and maximum values labeled
        Row {
            width: parent.width
            spacing: _margins

            QGCLabel {
                id: minLabel
                width: ScreenTools.defaultFontPixelWidth * 10
                text: _fact.min.toFixed(2)
                horizontalAlignment: Text.AlignRight
            }

            QGCSlider {
                id: slide
                width: parent.width - minLabel.width - maxLabel.width - _margins*2

                stepSize: _fact.increment ? Math.max(_fact.increment, _minIncrement) : _minIncrement
                tickmarksEnabled: true

                // Override style to make handles smaller than default
                style: SliderStyle {
                    handle: Rectangle {
                        anchors.centerIn: parent
                        color: qgcPal.button
                        border.color: qgcPal.buttonText
                        border.width:   1
                        implicitWidth: _radius * 2
                        implicitHeight: _radius * 2
                        radius: _radius

                        property real _radius: Math.round(ScreenTools.defaultFontPixelHeight * 0.35)
                    }
                }

                onValueChanged: {
                    if (_loadComplete) {
                        if (!(_fact.value < value + 0.0001 && _fact.value > value - 0.0001)) { // prevent binding loop
                            _fact.value = value
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
                id: maxLabel
                width: ScreenTools.defaultFontPixelWidth * 10
                text: _fact.max.toFixed(2)
            }
        } // Row - Slider with minimum and maximum values labeled
    } // Column - Param name, value, description and slider adjustment
} // Row
