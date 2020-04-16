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

import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0

Item {
    id:     root
    height: value.y + value.height

    property var    instrumentValue:                null
    property bool   recalcOk:                       false

    property var    _rgFontSizes:                   [ ScreenTools.defaultFontPointSize, ScreenTools.smallFontPointSize, ScreenTools.mediumFontPointSize, ScreenTools.largeFontPointSize ]
    property var    _rgFontSizeRatios:              [ 1, ScreenTools.smallFontPointRatio, ScreenTools.mediumFontPointRatio, ScreenTools.largeFontPointRatio ]
    property real   _doubleDescent:                 ScreenTools.defaultFontDescent * 2
    property real   _tightDefaultFontHeight:        ScreenTools.defaultFontPixelHeight - _doubleDescent
    property var    _rgFontSizeTightHeights:        [ _tightDefaultFontHeight * _rgFontSizeRatios[0] + 2, _tightDefaultFontHeight * _rgFontSizeRatios[1] + 2, _tightDefaultFontHeight * _rgFontSizeRatios[2] + 2, _tightDefaultFontHeight * _rgFontSizeRatios[3] + 2 ]
    property real   _blankEntryHeight:              ScreenTools.defaultFontPixelHeight * 2

    // After fighting with using layout and/or anchors I gave up and just do a manual recalc to position items which ends up being much simpler
    function recalcPositions() {
        if (!recalcOk) {
            return
        }
        var smallSpacing = 2
        if (instrumentValue.icon) {
            if (instrumentValue.labelPosition === InstrumentValue.LabelAbove) {
                valueIcon.x = (width - valueIcon.width) / 2
                valueIcon.y = 0
                value.x = (width - value.width) / 2
                value.y = valueIcon.height + smallSpacing
            } else {
                var iconPlusValueWidth = valueIcon.width + value.width + ScreenTools.defaultFontPixelWidth
                valueIcon.x = (width - iconPlusValueWidth) / 2
                valueIcon.y = (value.height - valueIcon.height) / 2
                value.x = valueIcon.x + valueIcon.width + (ScreenTools.defaultFontPixelWidth / 2)
                value.y = 0
            }
            label.x = label.y = 0
        } else {
            // label above value
            if (instrumentValue.text) {
                label.x = (width - label.width) / 2
                label.y = 0
                value.y = label.height + smallSpacing
            } else {
                value.y = 0
            }
            value.x = (width - value.width) / 2
            valueIcon.x = valueIcon.y = 0
        }
    }

    onRecalcOkChanged:    recalcPositions()
    onWidthChanged:                         recalcPositions()

    Connections {
        target:                 instrumentValue
        onIconChanged:          recalcPositions()
        onLabelPositionChanged: recalcPositions()
    }

    QGCColoredImage {
        id:                         valueIcon
        height:                     _rgFontSizeTightHeights[instrumentValue.fontSize]
        width:                      height
        source:                     icon
        sourceSize.height:          height
        fillMode:                   Image.PreserveAspectFit
        mipmap:                     true
        smooth:                     true
        color:                      instrumentValue.isValidColor(instrumentValue.currentColor) ? instrumentValue.currentColor : qgcPal.text
        opacity:                    instrumentValue.currentOpacity
        visible:                    instrumentValue.icon
        onWidthChanged:             root.recalcPositions()
        onHeightChanged:            root.recalcPositions()

        property string icon
        readonly property string iconPrefix: "/InstrumentValueIcons/"

        function updateIcon() {
            if (instrumentValue.rangeType == InstrumentValue.IconSelectRange) {
                icon = iconPrefix + instrumentValue.currentIcon
            } else if (instrumentValue.icon) {
                icon = iconPrefix + instrumentValue.icon
            } else {
                icon = ""
            }
        }

        Connections {
            target:                 instrumentValue
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
        visible:                    !instrumentValue.fact
        onWidthChanged:             root.recalcPositions()
        onHeightChanged:            root.recalcPositions()
    }

    QGCLabel {
        id:                         label
        height:                     _rgFontSizeTightHeights[InstrumentValue.SmallFontSize]
        font.pointSize:             ScreenTools.smallFontPointSize
        text:                       instrumentValue.text.toUpperCase()
        verticalAlignment:          Text.AlignVCenter
        visible:                    instrumentValue.fact && instrumentValue.text && !instrumentValue.icon
        onWidthChanged:             root.recalcPositions()
        onHeightChanged:            root.recalcPositions()
    }

    QGCLabel {
        id:                         value
        font.pointSize:             _rgFontSizes[instrumentValue.fontSize]
        text:                       visible ? (instrumentValue.fact.enumOrValueString + (instrumentValue.showUnits ? instrumentValue.fact.units : "")) : ""
        verticalAlignment:          Text.AlignVCenter
        visible:                    instrumentValue.fact
        onWidthChanged:             root.recalcPositions()
        onHeightChanged:            root.recalcPositions()
    }
}
