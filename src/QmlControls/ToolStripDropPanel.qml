/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.ScreenTools
import QGroundControl.Palette

Item {
    id:         _root
    visible:    false

    signal          clicked()
    property real   radius:             ScreenTools.isMobile ? ScreenTools.defaultFontPixelHeight * 1.75 : ScreenTools.defaultFontPixelHeight * 1.25
    property real   viewportMargins:    0
    property var    toolStrip

    // Should be an enum but that get's into the whole problem of creating a singleton which isn't worth the effort
    readonly property int dropLeft:     1
    readonly property int dropRight:    2
    readonly property int dropUp:       3
    readonly property int dropDown:     4

    readonly property real _arrowBaseHeight:    radius             // Height of vertical side of arrow
    readonly property real _arrowPointWidth:    radius * 0.666     // Distance from vertical side to point
    readonly property real _dropMargin:         ScreenTools.defaultFontPixelWidth
    readonly property color _panelColor:        Qt.rgba(0.045, 0.048, 0.052, 0.72)
    readonly property color _panelBorderColor:  Qt.rgba(0.82, 0.88, 0.94, 0.075)

    property var    _dropEdgeTopPoint
    property alias  _dropDownComponent: panelLoader.sourceComponent
    property real   _viewportMaxTop:    0
    property real   _viewportMaxBottom: parent.parent.height - parent.y
    property real   _viewportMaxHeight: _viewportMaxBottom - _viewportMaxTop
    property var    _dropPanelCancel
    property var    _parentButton

    function show(panelEdgeTopPoint, panelComponent, parentButton) {
        _parentButton = parentButton
        _dropEdgeTopPoint = panelEdgeTopPoint
        _dropDownComponent = panelComponent
        _calcPositions()
        visible = true
        _dropPanelCancel = dropPanelCancelComponent.createObject(toolStrip.parent)
    }

    function hide() {
        if (_dropPanelCancel) {
            _dropPanelCancel.destroy()
            _parentButton.checked = false
            visible = false
            _dropDownComponent = undefined
        }
    }

    function _calcPositions() {
        var panelComponentWidth  = panelLoader.item.width
        var panelComponentHeight = panelLoader.item.height

        dropDownItem.width  = panelComponentWidth  + (_dropMargin * 2) + _arrowPointWidth
        dropDownItem.height = panelComponentHeight + (_dropMargin * 2)

        dropDownItem.x = _dropEdgeTopPoint.x + _dropMargin
        dropDownItem.y = _dropEdgeTopPoint.y -(dropDownItem.height / 2) + radius

        // Validate that dropdown is within viewport
        dropDownItem.y = Math.min(dropDownItem.y + dropDownItem.height, _viewportMaxBottom) - dropDownItem.height
        dropDownItem.y = Math.max(dropDownItem.y, _viewportMaxTop)

        // Adjust height to not exceed viewport bounds
        dropDownItem.height = Math.min(dropDownItem.height, _viewportMaxHeight - dropDownItem.y)

        // Arrow points
        arrowCanvas.arrowPoint.y = (_dropEdgeTopPoint.y + radius) - dropDownItem.y
        arrowCanvas.arrowPoint.x = 0
        arrowCanvas.arrowBase1.x = _arrowPointWidth
        arrowCanvas.arrowBase1.y = arrowCanvas.arrowPoint.y - (_arrowBaseHeight / 2)
        arrowCanvas.arrowBase2.x = arrowCanvas.arrowBase1.x
        arrowCanvas.arrowBase2.y = arrowCanvas.arrowBase1.y + _arrowBaseHeight
        arrowCanvas.requestPaint()
    } // function - _calcPositions

    QGCPalette { id: qgcPal }

    Component {
        // Overlay which is used to cancel the panel when the user clicks away
        id: dropPanelCancelComponent

        MouseArea {
            anchors.fill:   parent
            z:              toolStrip.z - 1
            onClicked:      dropPanel.hide()
        }
    }

    // This item is sized to hold the entirety of the drop panel including the arrow point
    Item {
        id: dropDownItem

        DeadMouseArea {
            anchors.fill: parent
        }

        Canvas {
            id:             arrowCanvas
            anchors.fill:   parent

            property point arrowPoint: Qt.point(0, 0)
            property point arrowBase1: Qt.point(0, 0)
            property point arrowBase2: Qt.point(0, 0)

            onPaint: {
                var panelX = _arrowPointWidth
                var panelY = 0
                var panelWidth = parent.width - _arrowPointWidth
                var panelHeight = parent.height
                var cornerRadius = Math.min(ScreenTools.defaultFontPixelWidth * 0.85, panelWidth / 2, panelHeight / 2)

                var context = getContext("2d")
                context.reset()
                context.beginPath()

                context.moveTo(panelX + cornerRadius, panelY)
                context.lineTo(panelX + panelWidth - cornerRadius, panelY)
                context.quadraticCurveTo(panelX + panelWidth, panelY, panelX + panelWidth, panelY + cornerRadius)
                context.lineTo(panelX + panelWidth, panelY + panelHeight - cornerRadius)
                context.quadraticCurveTo(panelX + panelWidth, panelY + panelHeight, panelX + panelWidth - cornerRadius, panelY + panelHeight)
                context.lineTo(panelX + cornerRadius, panelY + panelHeight)
                context.quadraticCurveTo(panelX, panelY + panelHeight, panelX, panelY + panelHeight - cornerRadius)
                context.lineTo(panelX, arrowBase2.y)
                context.lineTo(arrowPoint.x, arrowPoint.y)
                context.lineTo(arrowBase1.x, arrowBase1.y)
                context.lineTo(panelX, panelY + cornerRadius)
                context.quadraticCurveTo(panelX, panelY, panelX + cornerRadius, panelY)

                context.closePath()
                context.fillStyle = _panelColor
                context.fill()
                context.strokeStyle = _panelBorderColor
                context.lineWidth = 1
                context.stroke()
            }
        } // Canvas - arrowCanvas

        QGCFlickable {
            id:                 panelItemFlickable
            anchors.margins:    _dropMargin
            anchors.leftMargin: _dropMargin + _arrowPointWidth
            anchors.fill:       parent
            flickableDirection: Flickable.VerticalFlick
            contentWidth:       panelLoader.width
            contentHeight:      panelLoader.height

            Loader {
                id: panelLoader

                onHeightChanged:    _calcPositions()
                onWidthChanged:     _calcPositions()

                property var dropPanel: _root
            }
        }
    } // Item - dropDownItem
}
