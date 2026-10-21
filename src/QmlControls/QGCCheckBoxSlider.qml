import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

AbstractButton   {
    id:         control
    checkable:  true
    padding:    0

    property int  _sliderInset:     2
    property bool _showHighlight:   enabled && (pressed || checked)
    property real _labelSpacing:    ScreenTools.defaultFontPixelWidth

    QGCPalette { id: qgcPal; colorGroupEnabled: control.enabled }

    // The label wraps only when the control is narrower than its single-line width
    contentItem: Item {
        implicitWidth:  (label.visible ? label.implicitWidth + control._labelSpacing : 0) + indicator.width
        implicitHeight: label.contentHeight

        QGCLabel {
            id:             label
            objectName:     "checkBoxSlider_label"
            anchors.left:   parent.left
            width:          Math.max(0, parent.width - indicator.width - control._labelSpacing)
            text:           visible ? control.text : "X"
            wrapMode:       Text.WordWrap
            visible:        control.text !== ""
        }

        Rectangle {
            id:                     indicator
            objectName:             "checkBoxSlider_indicator"
            anchors.right:          parent.right
            anchors.verticalCenter: parent.verticalCenter
            height:                 ScreenTools.defaultFontPixelHeight
            width:                  height * 2
            radius:                 height / 2
            color:                  checked ? qgcPal.accent : qgcPal.windowInset
            border.width:           1
            border.color:           checked ? qgcPal.accent : qgcPal.borderSubtle

            Rectangle {
                anchors.fill:   parent
                color:          qgcPal.accent
                opacity:        _showHighlight ? 1 : control.enabled && control.hovered ? .2 : 0
                radius:         parent.radius
            }

            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                x:                      checked ? indicator.width - width - _sliderInset : _sliderInset
                height:                 parent.height - (_sliderInset * 2)
                width:                  height
                radius:                 height / 2
                color:                  qgcPal.textPrimary
            }
        }
    }
}
