/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                  2.4
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0

Item {
    id:         _root
    z:          QGroundControl.zOrderWidgets
    visible:    false

    signal          clicked()
    property real   radius:             ScreenTools.isMobile ? ScreenTools.defaultFontPixelHeight * 1.75 : ScreenTools.defaultFontPixelHeight * 1.25
    property real   viewportMargins:    0
    property var    toolStrip


    width:  radius * 2
    height: radius * 2

    // Should be an enum but that get's into the whole problem of creating a singleton which isn't worth the effort
    readonly property int dropLeft:     1
    readonly property int dropRight:    2
    readonly property int dropUp:       3
    readonly property int dropDown:     4

    readonly property real _arrowBaseWidth:     radius             // Width of long side of arrow
    readonly property real _arrowPointHeight:   radius * 0.666     // Height is long side to point
    readonly property real _dropCornerRadius:   ScreenTools.defaultFontPixelWidth * 0.5
    readonly property real _dropCornerRadiusX2: _dropCornerRadius * 2
    readonly property real _dropMargin:         _dropCornerRadius
    readonly property real _dropMarginX2:       _dropMargin * 2

    property var    _dropEdgeTopPoint
    property real   _dropEdgeHeight
    property alias  _dropDownComponent: dropDownLoader.sourceComponent
    property real   _viewportMaxTop:    0
    property real   _viewportMaxBottom: parent.parent.height - parent.y

    function show(panelEdgeTopPoint, panelEdgeHeight, panelComponent) {
        _dropEdgeTopPoint = panelEdgeTopPoint
        _dropEdgeHeight = panelEdgeHeight
        _dropDownComponent = panelComponent
        _calcPositions()
        visible = true
    }

    function hide() {
        if (visible) {
            visible = false
            _dropDownComponent = undefined
            toolStrip.uncheckAll()
        }
    }

    function _calcPositions() {
        var dropComponentWidth = dropDownLoader.item.width
        var dropComponentHeight = dropDownLoader.item.height
        var dropRectWidth = dropComponentWidth + _dropMarginX2
        var dropRectHeight = dropComponentHeight + _dropMarginX2

        dropItemHolderRect.width = dropRectWidth
        dropItemHolderRect.height = dropRectHeight

        dropDownItem.width = dropComponentWidth + _dropMarginX2
        dropDownItem.height = dropComponentHeight + _dropMarginX2

        dropDownItem.width += _arrowPointHeight

        dropDownItem.y = _dropEdgeTopPoint.y -(dropDownItem.height / 2) + radius

        dropItemHolderRect.y = 0

        dropDownItem.x = _dropEdgeTopPoint.x + _dropMargin
        dropItemHolderRect.x = _arrowPointHeight

        // Validate that dropdown is within viewport
        dropDownItem.y = Math.min(dropDownItem.y + dropDownItem.height, _viewportMaxBottom) - dropDownItem.height
        dropDownItem.y = Math.max(dropDownItem.y, _viewportMaxTop)

        // Arrow points
        arrowCanvas.arrowPoint.y = (_dropEdgeTopPoint.y + radius) - dropDownItem.y
        arrowCanvas.arrowPoint.x = 0
        arrowCanvas.arrowBase1.x = _arrowPointHeight
        arrowCanvas.arrowBase1.y = arrowCanvas.arrowPoint.y - (_arrowBaseWidth / 2)
        arrowCanvas.arrowBase2.x = arrowCanvas.arrowBase1.x
        arrowCanvas.arrowBase2.y = arrowCanvas.arrowBase1.y + _arrowBaseWidth
        arrowCanvas.requestPaint()
    } // function - _calcPositions

    QGCPalette { id: qgcPal }

    Item {
        id: dropDownItem

        Canvas {
            id:             arrowCanvas
            anchors.fill:   parent

            property var arrowPoint: Qt.point(0, 0)
            property var arrowBase1: Qt.point(0, 0)
            property var arrowBase2: Qt.point(0, 0)

            onPaint: {
                var context = getContext("2d")
                context.reset()
                context.beginPath()

                context.moveTo(dropItemHolderRect.x, dropItemHolderRect.y)
                context.lineTo(dropItemHolderRect.x + dropItemHolderRect.width, dropItemHolderRect.y)
                context.lineTo(dropItemHolderRect.x + dropItemHolderRect.width, dropItemHolderRect.y + dropItemHolderRect.height)
                context.lineTo(dropItemHolderRect.x, dropItemHolderRect.y + dropItemHolderRect.height)
                context.lineTo(arrowBase2.x, arrowBase2.y)
                context.lineTo(arrowPoint.x, arrowPoint.y)
                context.lineTo(arrowBase1.x, arrowBase1.y)
                context.lineTo(dropItemHolderRect.x, dropItemHolderRect.y)
                context.closePath()
                context.fillStyle = qgcPal.windowShade
                context.fill()
            }
        } // Canvas - arrowCanvas

        Item {
            id:     dropItemHolderRect

            Loader {
                id: dropDownLoader
                x:  _dropMargin
                y:  _dropMargin

                property var dropPanel: _root
            }
        }
    } // Item - dropDownItem
}
