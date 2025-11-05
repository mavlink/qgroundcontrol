import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls


DelayButton {
    id:             control
    hoverEnabled:   !ScreenTools.isMobile
    topPadding:     _verticalPadding
    bottomPadding:  _verticalPadding
    leftPadding:    _horizontalPadding
    rightPadding:   _horizontalPadding
    focusPolicy:    Qt.ClickFocus
    font.family:    ScreenTools.normalFontFamily
    delay:          defaultDelay

    property bool   showBorder:     qgcPal.globalTheme === QGCPalette.Light
    property real   backRadius:     ScreenTools.buttonBorderRadius
    property real   heightFactor:   0.5
    property real   fontWeight:     Font.Normal // default for qml Text
    property real   pointSize:      ScreenTools.defaultFontPointSize
    property int    defaultDelay:   500

    property alias wrapMode:            text.wrapMode
    property alias horizontalAlignment: text.horizontalAlignment
    property alias backgroundColor:     backRect.color
    property alias textColor:           text.color

    property bool   _showHighlight:     enabled && pressed
    property int    _horizontalPadding: ScreenTools.defaultFontPixelWidth * 2
    property int    _verticalPadding:   Math.round(ScreenTools.defaultFontPixelHeight * heightFactor)
    property bool   _showHelp:          false
    property bool   _activated:         false

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    Timer {
        id:         helpTimeout
        interval:   3000
        repeat:     false
        onTriggered: control._showHelp = false
    }

    onActivated: {
        _activated = true
        _showHelp = false
    }
    onPressed: {
        _activated = false
    }
    onReleased: {
        _showHelp = !_activated
        _activated = false
        if (_showHelp) {
            helpTimeout.start()
        } else {
            helpTimeout.stop()
        }
    }

    background: Rectangle {
        id:             backRect
        radius:         backRadius
        implicitWidth:  Math.max(control._showHelp ? helpText.contentWidth : 0, ScreenTools.implicitButtonWidth)
        implicitHeight: ScreenTools.implicitButtonHeight
        border.width:   showBorder ? 1 : 0
        border.color:   qgcPal.buttonBorder

        gradient: Gradient {
            orientation: Gradient.Horizontal

            GradientStop { position: 0.0; color: qgcPal.buttonHighlight }
            GradientStop { position: 0.15; color: qgcPal.button }
        }

        Rectangle {
            anchors.fill:           parent
            anchors.rightMargin:    control.pressed ? parent.width * (1.0 - control.progress) : 0
            color:                  qgcPal.buttonHighlight
            opacity:                _showHighlight ? 1 : control.enabled && control.hovered ? .2 : 0
            radius:                 parent.radius
        }

        QGCLabel {
            id:                         helpText
            text:                       qsTr("Hold to Confirm")
            anchors.bottom:             parent.bottom
            anchors.horizontalCenter:   parent.horizontalCenter
            font.pointSize:             ScreenTools.smallFontPointSize
            visible:                    control._showHelp
        }
    }

    contentItem: QGCLabel {
        id:                     text
        horizontalAlignment:    Text.AlignHCenter
        text:                   control.text
        font.pointSize:         control.pointSize
        font.family:            control.font.family
        font.weight:            control.fontWeight
        color:                  qgcPal.buttonText
    }
}
