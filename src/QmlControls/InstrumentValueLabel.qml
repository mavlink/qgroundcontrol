/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import QGroundControl
import QGroundControl.Controls
import QGroundControl.ScreenTools
import QGroundControl.Palette

ColumnLayout {
    property var    instrumentValueData:        null

    property var    _rgFontSizes:               [ ScreenTools.defaultFontPointSize, ScreenTools.smallFontPointSize, ScreenTools.mediumFontPointSize, ScreenTools.largeFontPointSize ]
    property var    _rgFontSizeRatios:          [ 1, ScreenTools.smallFontPointRatio, ScreenTools.mediumFontPointRatio, ScreenTools.largeFontPointRatio ]
    property real   _doubleDescent:             ScreenTools.defaultFontDescent * 2
    property real   _tightDefaultFontHeight:    ScreenTools.defaultFontPixelHeight - _doubleDescent
    property var    _rgFontSizeTightHeights:    [ _tightDefaultFontHeight * _rgFontSizeRatios[0] + 2, _tightDefaultFontHeight * _rgFontSizeRatios[1] + 2, _tightDefaultFontHeight * _rgFontSizeRatios[2] + 2, _tightDefaultFontHeight * _rgFontSizeRatios[3] + 2 ]
    property real   _tightHeight:               _rgFontSizeTightHeights[instrumentValueData.factValueGrid.fontSize]
    property bool   _iconVisible:               instrumentValueData.rangeType === InstrumentValueData.IconSelectRange || instrumentValueData.icon
    property var    _color:                     instrumentValueData.isValidColor(instrumentValueData.currentColor) ? instrumentValueData.currentColor : qgcPal.text

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    QGCColoredImage {
        id:                         valueIcon
        Layout.alignment:           Qt.AlignVCenter
        height:                     _tightHeight * 0.75
        width:                      _tightHeight * 0.85
        sourceSize.height:          height
        fillMode:                   Image.PreserveAspectFit
        mipmap:                     true
        smooth:                     true
        color:                      _color
        opacity:                    instrumentValueData.currentOpacity
        visible:                    _iconVisible

        readonly property string iconPrefix: "/InstrumentValueIcons/"

        function updateIcon() {
            if (instrumentValueData.rangeType === InstrumentValueData.IconSelectRange) {
                valueIcon.source = instrumentValueData.currentIcon != "" ? iconPrefix + instrumentValueData.currentIcon : "";
            } else if (instrumentValueData.icon) {
                valueIcon.source = instrumentValueData.icon != "" ? iconPrefix + instrumentValueData.icon : "";
            } else {
                valueIcon.source = ""
            }
        }

        Connections {
            target:                 instrumentValueData
            function onRangeTypeChanged() { valueIcon.updateIcon() }
            function onCurrentIconChanged() { valueIcon.updateIcon() }
            function onIconChanged() { valueIcon.updateIcon() }
        }
        Component.onCompleted:      updateIcon();
    }

    QGCLabel {
        Layout.alignment:   Qt.AlignVCenter
        height:             _tightHeight
        font.pointSize:     ScreenTools.smallFontPointSize
        text:               instrumentValueData.text
        color:              _color
        opacity:            instrumentValueData.currentOpacity
        visible:            !_iconVisible
    }
}
