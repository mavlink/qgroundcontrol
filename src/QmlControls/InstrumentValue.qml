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
import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0

Item {
    id:     root
    height: value.y + value.height

    property var    instrumentValueData:            null
    property bool   recalcOk:                       false

    property var    _rgFontSizes:                   [ ScreenTools.defaultFontPointSize, ScreenTools.smallFontPointSize, ScreenTools.mediumFontPointSize, ScreenTools.largeFontPointSize ]
    property var    _rgFontSizeRatios:              [ 1, ScreenTools.smallFontPointRatio, ScreenTools.mediumFontPointRatio, ScreenTools.largeFontPointRatio ]
    property real   _doubleDescent:                 ScreenTools.defaultFontDescent * 2
    property real   _tightDefaultFontHeight:        ScreenTools.defaultFontPixelHeight - _doubleDescent
    property var    _rgFontSizeTightHeights:        [ _tightDefaultFontHeight * _rgFontSizeRatios[0] + 2, _tightDefaultFontHeight * _rgFontSizeRatios[1] + 2, _tightDefaultFontHeight * _rgFontSizeRatios[2] + 2, _tightDefaultFontHeight * _rgFontSizeRatios[3] + 2 ]
    property real   _tightHeight:                   _rgFontSizeTightHeights[instrumentValueData.instrumentValueArea.fontSize]
    property real   _fontSize:                      _rgFontSizes[instrumentValueData.instrumentValueArea.fontSize]
    property real   _horizontalLabelSpacing:        ScreenTools.defaultFontPixelWidth
    property real   _blankEntryHeight:              ScreenTools.defaultFontPixelHeight * 2
    property bool   _showIcon:                      instrumentValueData.icon || instrumentValueData.rangeType === InstrumentValueData.IconSelectRange

    // After fighting with using layout and/or anchors I gave up and just do a manual recalc to position items which ends up being much simpler
    function recalcPositions() {
        if (!recalcOk) {
            return
        }
        var smallVerticalSpacing = 2
        var halfWidth = width / 2
        var halfHorizontalSpacing = _horizontalLabelSpacing / 2
        if (_showIcon) {
            if (instrumentValueData.instrumentValueArea.orientation === InstrumentValueArea.VerticalOrientation) {
                valueIcon.x = (width - valueIcon.width) / 2
                valueIcon.y = 0
                value.x = (width - value.width) / 2
                value.y = valueIcon.height + smallVerticalSpacing
            } else {
                value.y = 0 // value assumed to be taller
                valueIcon.y = (value.height - valueIcon.height) / 2
                value.x = halfWidth + halfHorizontalSpacing
                valueIcon.x = halfWidth - halfHorizontalSpacing - valueIcon.width
            }
            label.x = label.y = 0
        } else {
            if (instrumentValueData.text) {
                if (instrumentValueData.instrumentValueArea.orientation === InstrumentValueArea.VerticalOrientation) {
                    label.x = (width - label.width) / 2
                    label.y = 0
                    value.x = (width - value.width) / 2
                    value.y = label.height + smallVerticalSpacing
                } else {
                    value.y = 0 // value assumed to be taller
                    label.y = (value.height - label.height) / 2
                    value.x = halfWidth + halfHorizontalSpacing
                    label.x = halfWidth - halfHorizontalSpacing - label.width
                }
            } else {
                value.x = (width - value.width) / 2
                value.y = (height - value.height) / 2
            }
            valueIcon.x = valueIcon.y = 0
        }
    }

    onRecalcOkChanged:    recalcPositions()
    onWidthChanged:       recalcPositions()

    Connections {
        target:         instrumentValueData
        onIconChanged:  recalcPositions()
    }

    QGCColoredImage {
        id:                         valueIcon
        height:                     _tightHeight
        width:                      height
        source:                     icon
        sourceSize.height:          height
        fillMode:                   Image.PreserveAspectFit
        mipmap:                     true
        smooth:                     true
        color:                      instrumentValueData.isValidColor(instrumentValueData.currentColor) ? instrumentValueData.currentColor : qgcPal.text
        opacity:                    instrumentValueData.currentOpacity
        visible:                    _showIcon
        onWidthChanged:             root.recalcPositions()
        onHeightChanged:            root.recalcPositions()

        property string icon
        readonly property string iconPrefix: "/InstrumentValueIcons/"

        function updateIcon() {
            if (instrumentValueData.rangeType === InstrumentValueData.IconSelectRange) {
                icon = iconPrefix + instrumentValueData.currentIcon
            } else if (instrumentValueData.icon) {
                icon = iconPrefix + instrumentValueData.icon
            } else {
                icon = ""
            }
        }

        Connections {
            target:                 instrumentValueData
            onRangeTypeChanged:     valueIcon.updateIcon()
            onCurrentIconChanged:   valueIcon.updateIcon()
            onIconChanged:          valueIcon.updateIcon()
        }
        Component.onCompleted:      updateIcon();
    }

    QGCLabel {
        id:                         blank
        anchors.horizontalCenter:   parent.horizontalCenter
        height:                     _columnButtonsTotalHeight
        font.pointSize:             ScreenTools.smallFontPointSize
        text:                       _settingsUnlocked ? qsTr("BLANK") : ""
        horizontalAlignment:        Text.AlignHCenter
        verticalAlignment:          Text.AlignVCenter
        visible:                    !instrumentValueData.fact
        onWidthChanged:             root.recalcPositions()
        onHeightChanged:            root.recalcPositions()
    }

    QGCLabel {
        id:                         label
        height:                     _tightHeight
        text:                       instrumentValueData.text
        verticalAlignment:          Text.AlignVCenter
        visible:                    !_showIcon
        onWidthChanged:             root.recalcPositions()
        onHeightChanged:            root.recalcPositions()
    }

    QGCLabel {
        id:                         value
        font.pointSize:             _fontSize
        text:                       visible ? (instrumentValueData.fact.enumOrValueString + (instrumentValueData.showUnits ? instrumentValueData.fact.units : "")) : ""
        verticalAlignment:          Text.AlignVCenter
        visible:                    instrumentValueData.fact
        onWidthChanged:             root.recalcPositions()
        onHeightChanged:            root.recalcPositions()
    }
}
