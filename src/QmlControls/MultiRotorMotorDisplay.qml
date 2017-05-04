import QtQuick 2.3

import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

Item {
    id: motorRoot

    property int    motorCount: 4       // Number of motors on vehicle
    property bool   xConfig:    true    // true: X configuration, false: Plus configuration
    property bool   coaxial:    true    // true: motors on top bottom of same arm, false: motors only on top of arm

    property bool   _unsupportedConfig: motorCount == 3 || (motorCount == 6 && coaxial) // Tricopters NYI
    property var    _qgcPal:            QGCPalette { colorGroupEnabled: enabled }
    property real   _rotorRadius:       motorRoot.height / 8
    property int    _motorsPerSide:     motorCount / (coaxial ? 2 : 1)

    readonly property string    _cwColor:               "#15ce15"   // Green
    readonly property string    _ccwColor:              "#1283e0"   // Blue
    readonly property var       _motorColors4Plus:      [ _ccwColor, _cwColor ]
    readonly property var       _motorColors4X:         [ _cwColor, _ccwColor ]
    readonly property var       _motorColors6:          [ _cwColor, _ccwColor ]
    readonly property var       _motorColors8:          [ _cwColor, _ccwColor ]
    readonly property var       _motorColors4Top:       [ _cwColor, _ccwColor ]
    readonly property var       _motorColors4Bottom:    [ _ccwColor, _cwColor ]

    readonly property var       _motorNumbers4Plus:     [ 1, 4, 2, 3 ]
    readonly property var       _motorNumbers4X:        [ 4, 2, 3, 1 ]
    readonly property var       _motorNumbers6Plus:     [ 6, 2, 3, 5, 1, 4 ]
    readonly property var       _motorNumbers6X:        [ 1, 4, 6, 2, 3, 5 ]
    readonly property var       _motorNumbers8:         [ 8, 4, 2, 6, 7, 5, 1, 3 ]
    readonly property var       _motorNumbers4Top:      [ 4, 3, 2, 1 ]
    readonly property var       _motorNumbers4Bottom:   [ 7, 8, 5, 6 ]

    Component.onCompleted: {
        if (coaxial) {
            topMotors.motorNumbers = _motorNumbers4Top
            bottomMotors.motorNumbers = _motorNumbers4Bottom
        } else {
            switch (motorCount) {
            case 4:
                topMotors.motorNumbers = xConfig ? _motorNumbers4X : _motorNumbers4Plus
                topMotors.motorColors = xConfig ? _motorColors4X : _motorColors4Plus
                break
            case 6:
                topMotors.motorNumbers = xConfig ? _motorNumbers6X : _motorNumbers6Plus
                topMotors.motorColors = _motorColors6
                break
            default:
            case 8:
                topMotors.motorNumbers = _motorNumbers8
                topMotors.motorColors = _motorColors8
                break
            }
            bottomMotors.motorNumbers = _motorNumbers8
        }
    }

    Component {
        id: motorDisplayComponent

        Repeater {
            id:     motorRepeater
            model: _motorsPerSide

            Item {
                x:      motorRepeater.width / 2 + _armXCenter - rotor.radius
                y:      motorRepeater.height / 2 + _armYCenter - rotor.radius
                width:  _rotorRadius * 2
                height: _rotorRadius * 2

                property real _armOffsetRadians:        ((2 * Math.PI) / _motorsPerSide)
                property real _armOffsetIndexRadians:   (_armOffsetRadians  * index) + ((xConfig && _motorsPerSide != 6) || (!xConfig && _motorsPerSide == 6) ? _armOffsetRadians / 2 : 0)
                property real _armLength:               (motorRepeater.height / 2) - (_rotorRadius * (xConfig && _motorsPerSide == 4 ? 0 : 1))
                property real _armXCenter:              Math.cos(_armOffsetIndexRadians) * _armLength // adjacent = cos * hypotenuse
                property real _armYCenter:              Math.sin(_armOffsetIndexRadians) * _armLength // opposite = sin * hypotenuse

                Rectangle {
                    id:             rotor
                    anchors.fill:   parent
                    radius:         _rotorRadius
                    color:          motorColors[index & 1]
                    opacity:        topCoaxial ? 0.65 : 1.0
                    border.color:   topCoaxial ? "black" : color
                    antialiasing:   true
                    readonly property bool topCoaxial: topMotors && coaxial
                    //-- Top Directional Arrow
                    QGCColoredImage {
                        color:                      _qgcPal.globalTheme === QGCPalette.Light ? "black" : "white"
                        height:                     parent.height * 0.2
                        width:                      height
                        sourceSize.height:          height
                        mipmap:                     true
                        fillMode:                   Image.PreserveAspectFit
                        source:                     (index & 1) ? "/qmlimages/ArrowCW.svg" : "/qmlimages/ArrowCCW.svg"
                        anchors.top:                parent.top
                        anchors.topMargin:          height * -0.5
                        anchors.horizontalCenter:   parent.horizontalCenter
                    }
                    //-- Bottom Directional Arrow
                    QGCColoredImage {
                        color:                      _qgcPal.globalTheme === QGCPalette.Light ? "black" : "white"
                        height:                     parent.height * 0.2
                        width:                      height
                        sourceSize.height:          height
                        mipmap:                     true
                        fillMode:                   Image.PreserveAspectFit
                        source:                     (index & 1) ? "/qmlimages/ArrowCCW.svg" : "/qmlimages/ArrowCW.svg"
                        anchors.bottom:             parent.bottom
                        anchors.bottomMargin:       height * -0.5
                        anchors.horizontalCenter:   parent.horizontalCenter
                    }
                    transform: [
                        Rotation {
                            origin.x:           rotor.width  / 2
                            origin.y:           rotor.height / 2
                            angle:              (index & 1) ? 45 : -45
                        }]
                }

                Rectangle {
                    anchors.horizontalCenter:   parent.horizontalCenter
                    anchors.verticalCenter:     parent.verticalCenter
                    width:                      radius * 2
                    height:                     radius * 2
                    radius:                     ScreenTools.defaultFontPixelHeight * 0.666
                    color:                      Qt.rgba(1,1,1,1)
                    border.color:               Qt.rgba(0,0,0,0.75)
                    antialiasing:               true

                    QGCLabel {
                        anchors.fill:               parent
                        verticalAlignment:          Text.AlignVCenter
                        horizontalAlignment:        Text.AlignHCenter
                        text:                       motorNumbers[index]
                        color:                      parent.border.color
                    }
                }
            } // Item
        } // Repeater
    } // Component - MotorDisplayComponent

    Item {
        anchors.fill:   parent
        visible:        !_unsupportedConfig

        Loader {
            id:                 bottomMotors
            anchors.topMargin:  ScreenTools.defaultFontPixelHeight
            anchors.fill:       parent
            sourceComponent:    motorDisplayComponent
            visible:            coaxial

            property bool   topMotors:      false
            property var    motorNumbers:   _motorNumbers8
            property var    motorColors:    _motorColors4Bottom
        }

        Loader {
            id:                     topMotors
            anchors.fill:           parent
            anchors.bottomMargin:   coaxial ? ScreenTools.defaultFontPixelHeight : 0
            sourceComponent:        motorDisplayComponent

            property bool   topMotors:      true
            property var    motorNumbers:   _motorNumbers8
            property var    motorColors:    _motorColors4Top
        }

        QGCColoredImage {
            color:                      _qgcPal.text
            height:                     parent.height * 0.5
            width:                      height * 0.55
            sourceSize.height:          height
            mipmap:                     true
            fillMode:                   Image.PreserveAspectFit
            source:                     "/qmlimages/ArrowDirection.svg"
            anchors.centerIn:           parent
        }

    } // Item
} // Item
