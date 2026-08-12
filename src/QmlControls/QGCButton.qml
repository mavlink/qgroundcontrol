import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl.Palette
import QGroundControl.ScreenTools

/// Industrial Aerospace Ground Control Push Button
Button {
    id: control
    hoverEnabled: !ScreenTools.isMobile
    implicitHeight: 42
    implicitWidth: Math.max(96, contentItem.implicitWidth + (leftPadding + rightPadding))
    topPadding: 6
    bottomPadding: 6
    leftPadding: 12
    rightPadding: 12
    focusPolicy: Qt.ClickFocus
    font.family: ScreenTools.normalFontFamily
    font.pixelSize: 13
    font.weight: Font.DemiBold
    text: ""

    property bool   primary:            false
    property bool   showBorder:         true
    property real   backRadius:         6
    property string iconSource:         ""
    property real   fontWeight:         Font.DemiBold
    property real   pointSize:          11
    property real   _horizontalPadding: -1

    on_HorizontalPaddingChanged: {
        if (_horizontalPadding >= 0) {
            leftPadding = _horizontalPadding
            rightPadding = _horizontalPadding
        }
    }

    property alias wrapMode:            text.wrapMode
    property alias horizontalAlignment: text.horizontalAlignment
    property alias backgroundColor:     backRect.color
    property alias textColor:           text.color
    property bool  _showHighlight:      control.pressed | control.checked

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    background: Rectangle {
        id: backRect
        radius: backRadius
        color: primary ? (control.pressed ? qgcPal.buttonHighlight : qgcPal.primaryButton) :
                         (control.pressed ? qgcPal.buttonHighlight : (control.hovered ? qgcPal.windowShadeLight : qgcPal.button))
        border.width: showBorder ? 1 : 0
        border.color: primary ? qgcPal.primaryButton :
                                (control.pressed ? qgcPal.buttonHighlight : (control.hovered ? qgcPal.buttonHighlight : qgcPal.buttonBorder))

        Behavior on color { ColorAnimation { duration: 80 } }
        Behavior on border.color { ColorAnimation { duration: 80 } }
    }

    contentItem: RowLayout {
        spacing: 8
        anchors.centerIn: parent

        QGCColoredImage {
            id: icon
            Layout.alignment: Qt.AlignVCenter
            source: control.iconSource
            height: 16
            width: 16
            color: text.color
            fillMode: Image.PreserveAspectFit
            visible: control.iconSource !== ""
        }

        QGCLabel {
            id: text
            Layout.alignment: Qt.AlignVCenter
            text: control.text
            font.pixelSize: control.font.pixelSize
            font.family: control.font.family
            font.weight: control.fontWeight
            color: primary ? qgcPal.primaryButtonText : (control.pressed ? qgcPal.buttonHighlightText : qgcPal.buttonText)
            visible: control.text !== ""
        }
    }
}
