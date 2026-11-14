import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl.Palette
import QGroundControl.ScreenTools

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
    // Application Settings page only: pill-shaped buttons
    readonly property bool _settingsStyle: (typeof mainWindow !== "undefined") && mainWindow.settingsPageOpen

    property bool   showBorder:     _settingsStyle || qgcPal.globalTheme === QGCPalette.Light
    property real   backRadius:     _settingsStyle ? height / 2 : ScreenTools.buttonBorderRadius
    property real   heightFactor:   0.5
    property string iconSource:     ""
    property real   fontWeight:     Font.Normal // default for qml Text
    property real   pointSize:      ScreenTools.defaultFontPointSize

    property alias wrapMode:            text.wrapMode
    property alias horizontalAlignment: text.horizontalAlignment
    property alias backgroundColor:     backRect.color
    property alias textColor:           text.color

    property bool   _showHighlight:     enabled && (pressed | checked)

    property int _horizontalPadding:    ScreenTools.defaultFontPixelWidth * 2
    property int _verticalPadding:      Math.round(ScreenTools.defaultFontPixelHeight * heightFactor)

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    background: Rectangle {
        id:             backRect
        radius:         backRadius
        implicitWidth:  ScreenTools.implicitButtonWidth
        implicitHeight: ScreenTools.implicitButtonHeight
        border.width:   showBorder ? 1 : 0
        border.color:   (_settingsStyle && qgcPal.globalTheme === QGCPalette.Dark) ? "#606060" : qgcPal.buttonBorder
        color:          (_settingsStyle && qgcPal.globalTheme === QGCPalette.Dark) ? "#404040" : (primary ? qgcPal.primaryButton : qgcPal.button)

        Rectangle {
            anchors.fill:   parent
            color:          qgcPal.buttonHighlight
            opacity:        _showHighlight ? 1 : control.enabled && control.hovered ? .2 : 0
            radius:         parent.radius
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
                color:                  _showHighlight ? qgcPal.buttonHighlightText : (primary ? qgcPal.primaryButtonText : qgcPal.buttonText)
                visible:                control.text !== "" 
            }
    }
}
