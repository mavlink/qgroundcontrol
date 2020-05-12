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
import QGroundControl.Templates     1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0

ColumnLayout {
    property var    instrumentValueData:            null
    property bool   settingsUnlocked:               false

    property bool   _verticalOrientation:           instrumentValueData.factValueGrid.orientation === FactValueGrid.VerticalOrientation
    property var    _rgFontSizes:                   [ ScreenTools.defaultFontPointSize, ScreenTools.smallFontPointSize, ScreenTools.mediumFontPointSize, ScreenTools.largeFontPointSize ]
    property var    _rgFontSizeRatios:              [ 1, ScreenTools.smallFontPointRatio, ScreenTools.mediumFontPointRatio, ScreenTools.largeFontPointRatio ]
    property real   _doubleDescent:                 ScreenTools.defaultFontDescent * 2
    property real   _tightDefaultFontHeight:        ScreenTools.defaultFontPixelHeight - _doubleDescent
    property var    _rgFontSizeTightHeights:        [ _tightDefaultFontHeight * _rgFontSizeRatios[0] + 2, _tightDefaultFontHeight * _rgFontSizeRatios[1] + 2, _tightDefaultFontHeight * _rgFontSizeRatios[2] + 2, _tightDefaultFontHeight * _rgFontSizeRatios[3] + 2 ]
    property real   _tightHeight:                   _rgFontSizeTightHeights[instrumentValueData.factValueGrid.fontSize]
    property real   _fontSize:                      _rgFontSizes[instrumentValueData.factValueGrid.fontSize]
    property real   _horizontalLabelSpacing:        ScreenTools.defaultFontPixelWidth
    property bool   _valueVisible:                  instrumentValueData.fact
    property real   _width:                         0
    property real   _height:                        0

    QGCLabel {
        Layout.alignment:   _verticalOrientation ? Qt.AlignHCenter : Qt.AlignVCenter
        font.pointSize:     _fontSize
        text:               instrumentValueData.fact.enumOrValueString + (instrumentValueData.showUnits ? " " + instrumentValueData.fact.units : "")
        visible:            _valueVisible

        QGCPalette { id: qgcPal; colorGroupEnabled: enabled }
    }
}
