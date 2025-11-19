import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls


/// Standard push button control:
///     If there is both an icon and text the icon will be to the left of the text
///     If icon only, icon will be centered
Button {
    id:             control
    hoverEnabled:   !ScreenTools.isMobile
    topPadding:     _verticalPadding
    bottomPadding:  _verticalPadding
    leftPadding:    _horizontalPadding
    rightPadding:   _horizontalPadding
    focusPolicy:    Qt.ClickFocus
    font.family:    ScreenTools.normalFontFamily
    text:           ""

    property bool   primary:        false                               ///< primary button for a group of buttons
    property bool   showBorder:     qgcPal.globalTheme === QGCPalette.Light
    property real   backRadius:     ScreenTools.buttonBorderRadius
    property real   heightFactor:   0.5
    property string iconSource:     ""
    property real   fontWeight:     Font.Normal // default for qml Text
    property real   pointSize:      ScreenTools.defaultFontPointSize

    // Neon style options (optional)
    property bool   neon:           false
    property color  neonColor:      qgcPal.brandingBlue
    property bool   pill:           false
    property real   neonBorderWidth: 1

    property alias wrapMode:            text.wrapMode
    property alias horizontalAlignment: text.horizontalAlignment
    property alias backgroundColor:     backRect.color
    property alias textColor:           text.color

    property bool   _showHighlight:     enabled && (pressed | checked)

    property int _horizontalPadding:    neon ? Math.round(ScreenTools.defaultFontPixelWidth * 1.25) : ScreenTools.defaultFontPixelWidth * 2
    property int _verticalPadding:      Math.round(ScreenTools.defaultFontPixelHeight * heightFactor)

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    background: Rectangle {
        id:             backRect
        radius:         pill ? height/2 : backRadius
        implicitWidth:  ScreenTools.implicitButtonWidth
        implicitHeight: ScreenTools.implicitButtonHeight
        border.width:   neon ? neonBorderWidth : 1
        border.color:   neon ? neonColor : qgcPal.groupBorder
        color:          qgcPal.toolbarBackground
        antialiasing:   true

        Rectangle {
            anchors.fill:   parent
            anchors.margins: neon ? 1 : 0
            color:          neon ? neonColor : qgcPal.toolStripHoverColor
            opacity:        neon ? (_showHighlight ? 0.16 : control.enabled && control.hovered ? 0.08 : 0)
                                : (_showHighlight ? 0.18 : control.enabled && control.hovered ? 0.10 : 0)
            radius:         parent.radius
            antialiasing:   true
        }
    }

    contentItem: RowLayout {
            spacing: ScreenTools.defaultFontPixelWidth

            QGCColoredImage {
                id:                     icon
                Layout.alignment:       Qt.AlignHCenter
                source:                 control.iconSource
                height:                 text.height
                width:                  height
                color:                  text.color
                fillMode:               Image.PreserveAspectFit
                sourceSize.height:      height
                visible:                control.iconSource !== ""
            }

            QGCLabel {
                id:                     text
                Layout.alignment:       Qt.AlignHCenter
                text:                   control.text
                font.pointSize:         control.pointSize
                font.family:            control.font.family
                font.weight:            fontWeight
                color:                  neon ? neonColor : qgcPal.buttonText
                visible:                control.text !== "" 
            }
    }
}
