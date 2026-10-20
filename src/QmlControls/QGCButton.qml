import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// Standard push button control:
///     If there is both an icon and text the icon will be to the left of the text
///     If icon only, icon will be centered
Button {
    property bool primary: false
    property bool showBorder: true
    property real backRadius: ScreenTools.defaultBorderRadius
    property real heightFactor: small ? 0.2 : 0.3
    property string iconSource: ""
    property real fontWeight: Font.Normal // default for qml Text
    property real pointSize: small ? ScreenTools.smallFontPointSize : ScreenTools.defaultFontPointSize

    /// Compact variant for secondary actions in dense areas (indicator drop-downs, list headers).
    /// Only changes the defaults of heightFactor, pointSize and horizontal padding and drops the minimum button height
    /// (the minimum width still applies); each default can still be overridden explicitly.
    property bool small: false

    property alias wrapMode: text.wrapMode
    property alias horizontalAlignment: text.horizontalAlignment
    property color backgroundColor: primary ? qgcPal.accent : qgcPal.windowCard
    property alias textColor: text.color

    id: control
    hoverEnabled: !ScreenTools.isMobile
    topPadding: _verticalPadding
    bottomPadding: _verticalPadding
    leftPadding: _horizontalPadding
    rightPadding: _horizontalPadding
    focusPolicy: Qt.ClickFocus
    font.family: ScreenTools.normalFontFamily
    text: ""

    property bool _showHighlight: enabled && (pressed || checked)
    property int _horizontalPadding: ScreenTools.defaultFontPixelWidth * (small ? 1 : 2)
    property int _textVerticalPadding: Math.round(ScreenTools.defaultFontPixelHeight * heightFactor)
    property real _textOnlyHeight: Math.max(small ? 0 : ScreenTools.implicitButtonHeight, text.implicitHeight + _textVerticalPadding * 2)
    property real _iconHeight: Math.round(_textOnlyHeight * 0.75)
    property int _verticalPadding: iconSource === "" ? _textVerticalPadding : Math.max(0, Math.round((_textOnlyHeight - _iconHeight) / 2))
    // Solid fills (primary or a caller supplied backgroundColor) keep their color and show state with an overlay instead
    property bool _solidFill: primary || !Qt.colorEqual(backgroundColor, qgcPal.windowCard)

    QGCPalette { id: qgcPal; colorGroupEnabled: control.enabled }

    background: Rectangle {
        id: backRect
        radius: backRadius
        implicitWidth: ScreenTools.implicitButtonWidth
        implicitHeight: control.small ? control.implicitContentHeight + control.topPadding + control.bottomPadding : ScreenTools.implicitButtonHeight
        border.width: showBorder ? 1 : 0
        border.color: primary || _showHighlight || (control.enabled && control.hovered) ? qgcPal.accent : qgcPal.borderSubtle
        color: !control._solidFill && control._showHighlight ? qgcPal.accentSubtle : control.backgroundColor

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: qgcPal.accentText
            visible: control._solidFill
            opacity: control._showHighlight ? 0.25 : (control.enabled && control.hovered ? 0.12 : 0)
        }
    }

    contentItem: RowLayout {
        spacing: ScreenTools.defaultFontPixelWidth

        QGCColoredImage {
            id: icon
            Layout.alignment: Qt.AlignHCenter
            source: control.iconSource
            height: _iconHeight
            width: height
            color: text.color
            fillMode: Image.PreserveAspectFit
            sourceSize.height: height
            visible: control.iconSource !== ""
        }

        QGCLabel {
            id: text
            Layout.alignment: Qt.AlignHCenter
            text: control.text
            font.pointSize: control.pointSize
            font.family: control.font.family
            font.weight: fontWeight
            color: primary ? qgcPal.accentText : qgcPal.textPrimary
            visible: control.text !== ""
        }
    }
}
