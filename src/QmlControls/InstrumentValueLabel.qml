/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.12
import QtQuick.Layouts  1.2
import QtQuick.Controls 2.5

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.Templates     1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0

ColumnLayout {
    property var    instrumentValueData:            null

    property bool   _verticalOrientation:       instrumentValueData.factValueGrid.orientation === FactValueGrid.VerticalOrientation
    property var    _rgFontSizes:               [ ScreenTools.defaultFontPointSize, ScreenTools.smallFontPointSize, ScreenTools.mediumFontPointSize, ScreenTools.largeFontPointSize ]
    property var    _rgFontSizeRatios:          [ 1, ScreenTools.smallFontPointRatio, ScreenTools.mediumFontPointRatio, ScreenTools.largeFontPointRatio ]
    property real   _doubleDescent:             ScreenTools.defaultFontDescent * 2
    property real   _tightDefaultFontHeight:    ScreenTools.defaultFontPixelHeight - _doubleDescent
    property var    _rgFontSizeTightHeights:    [ _tightDefaultFontHeight * _rgFontSizeRatios[0] + 2, _tightDefaultFontHeight * _rgFontSizeRatios[1] + 2, _tightDefaultFontHeight * _rgFontSizeRatios[2] + 2, _tightDefaultFontHeight * _rgFontSizeRatios[3] + 2 ]
    property real   _tightHeight:               _rgFontSizeTightHeights[instrumentValueData.factValueGrid.fontSize]
    property bool   _iconVisible:               instrumentValueData.rangeType === InstrumentValueData.IconSelectRange || instrumentValueData.icon

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    QGCColoredImage {
        id:                         valueIcon
        Layout.alignment:           _verticalOrientation ? Qt.AlignHCenter : Qt.AlignVCenter
        height:                     _tightHeight * 0.75
        width:                      height
        sourceSize.height:          height
        fillMode:                   Image.PreserveAspectFit
        mipmap:                     true
        smooth:                     true
        color:                      instrumentValueData.isValidColor(instrumentValueData.currentColor) ? instrumentValueData.currentColor : qgcPal.text
        opacity:                    instrumentValueData.currentOpacity
        visible:                    _iconVisible


        readonly property string iconPrefix: "/InstrumentValueIcons/"

        function updateIcon() {
            if (instrumentValueData.rangeType === InstrumentValueData.IconSelectRange) {
                valueIcon.source = iconPrefix + instrumentValueData.currentIcon
            } else if (instrumentValueData.icon) {
                valueIcon.source = iconPrefix + instrumentValueData.icon
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

        Rectangle {
            anchors.fill:   valueIcon
            color:          qgcPal.text
            visible:        valueIcon.status === Image.Error
        }
    }

    QGCLabel {
        Layout.alignment:   _verticalOrientation ? Qt.AlignHCenter : Qt.AlignVCenter
        height:             _tightHeight
        font.pointSize:     ScreenTools.smallFontPointSize
        text:               instrumentValueData.text
        visible:            !_iconVisible
    }
}
