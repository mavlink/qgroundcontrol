import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.Controls

/// Square icon-only button sized for touch
AbstractButton {
    id: control

    property string iconSource
    property real   iconRotation:   0
    property bool   danger:         false
    property string accessibleName  ///< Translated action name read by assistive technology

    Accessible.name: accessibleName

    implicitWidth:  implicitHeight
    implicitHeight: ScreenTools.isMobile ? ScreenTools.minTouchPixels : ScreenTools.implicitButtonHeight
    padding:        ScreenTools.defaultFontPixelHeight * 0.35
    hoverEnabled:   !ScreenTools.isMobile
    focusPolicy:    Qt.ClickFocus

    QGCPalette { id: qgcPal; colorGroupEnabled: control.enabled }

    background: Rectangle {
        radius:         ScreenTools.defaultBorderRadius
        color:          control.pressed ? qgcPal.accentSubtle : qgcPal.windowCard
        border.color:   control.hovered || control.pressed ? qgcPal.accent : qgcPal.borderSubtle
    }

    contentItem: QGCColoredImage {
        source:             control.iconSource
        rotation:           control.iconRotation
        color:              control.danger ? qgcPal.colorRed : qgcPal.textPrimary
        fillMode:           Image.PreserveAspectFit
        sourceSize.height:  height
    }
}
