import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0

Item {
    id: _root
    z:  QGroundControl.zOrderWidgets

    signal          clicked()
    property alias  buttonImage:        roundButton.buttonImage
    property alias  rotateImage:        roundButton.rotateImage
    property real   radius:             ScreenTools.isMobile ? ScreenTools.defaultFontPixelHeight * 1.75 : ScreenTools.defaultFontPixelHeight * 1.25
    property int    dropDirection:      dropDown
    property alias  dropDownComponent:  dropDownLoader.sourceComponent
    property real   viewportMargins:    0
    property real   topMargin:          parent.height - mainWindow.height
    property alias  lightBorders:       roundButton.lightBorders

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

    property real   _viewportMaxLeft:   -x + viewportMargins
    property real   _viewportMaxRight:  parent.width  - (viewportMargins * 2) - x
    property real   _viewportMaxTop:    -y + viewportMargins + topMargin
    property real   _viewportMaxBottom: parent.height - (viewportMargins * 2) - y

    // Set up ExclusiveGroup support. We use the checked property to drive visibility of drop down.

    property alias checked: roundButton.checked
    property ExclusiveGroup exclusiveGroup: null

    onExclusiveGroupChanged: {
        if (exclusiveGroup) {
            exclusiveGroup.bindCheckable(_root)
        }
    }

    function hideDropDown() {
        checked = false
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

        if (dropDirection == dropUp || dropDirection == dropDown) {
            dropDownItem.height += _arrowPointHeight

            dropDownItem.x = -(dropDownItem.width / 2) + radius

            dropItemHolderRect.x = 0

            if (dropDirection == dropUp) {
                dropDownItem.y = -(dropDownItem.height + _dropMargin)

                dropItemHolderRect.y = 0
            } else {
                dropDownItem.y = roundButton.height + _dropMargin

                dropItemHolderRect.y = _arrowPointHeight
            }

            // Validate that dropdown is within viewport
            dropDownItem.x = Math.max(dropDownItem.x, _viewportMaxLeft)
            dropDownItem.x = Math.min(dropDownItem.x + dropDownItem.width, _viewportMaxRight) - dropDownItem.width

            // Arrow points
            arrowCanvas.arrowPoint.x = (roundButton.x + radius) - dropDownItem.x
            if (dropDirection == dropUp) {
                arrowCanvas.arrowPoint.y = dropDownItem.height
                arrowCanvas.arrowBase1.x = arrowCanvas.arrowPoint.x - (_arrowBaseWidth / 2)
                arrowCanvas.arrowBase1.y = arrowCanvas.arrowPoint.y - _arrowPointHeight
                arrowCanvas.arrowBase2.x = arrowCanvas.arrowBase1.x + _arrowBaseWidth
                arrowCanvas.arrowBase2.y = arrowCanvas.arrowBase1.y
            } else {
                arrowCanvas.arrowPoint.y = 0
                arrowCanvas.arrowBase1.x = arrowCanvas.arrowPoint.x + (_arrowBaseWidth / 2)
                arrowCanvas.arrowBase1.y = _arrowPointHeight
                arrowCanvas.arrowBase2.x = arrowCanvas.arrowBase1.x - _arrowBaseWidth
                arrowCanvas.arrowBase2.y = arrowCanvas.arrowBase1.y
            }
        } else {
            dropDownItem.width += _arrowPointHeight

            dropDownItem.y = -(dropDownItem.height / 2) + radius

            dropItemHolderRect.y = 0

            if (dropDirection == dropLeft) {
                dropDownItem.x = -(dropDownItem.width + _dropMargin)

                dropItemHolderRect.x = 0
            } else {
                dropDownItem.x = roundButton.width + _dropMargin

                dropItemHolderRect.x = _arrowPointHeight
            }

            // Validate that dropdown is within viewport
            dropDownItem.y = Math.max(dropDownItem.y, _viewportMaxTop)
            dropDownItem.y = Math.min(dropDownItem.y + dropDownItem.height, _viewportMaxBottom) - dropDownItem.height

            // Arrow points
            arrowCanvas.arrowPoint.y = (roundButton.y + radius) - dropDownItem.y
            if (dropDirection == dropLeft) {
                arrowCanvas.arrowPoint.x = dropDownItem.width
                arrowCanvas.arrowBase1.x = arrowCanvas.arrowPoint.x - _arrowPointHeight
                arrowCanvas.arrowBase1.y = arrowCanvas.arrowPoint.y - (_arrowBaseWidth / 2)
                arrowCanvas.arrowBase2.x = arrowCanvas.arrowBase1.x
                arrowCanvas.arrowBase2.y = arrowCanvas.arrowBase1.y + _arrowBaseWidth
            } else {
                arrowCanvas.arrowPoint.x = 0
                arrowCanvas.arrowBase1.x = _arrowPointHeight
                arrowCanvas.arrowBase1.y = arrowCanvas.arrowPoint.y - (_arrowBaseWidth / 2)
                arrowCanvas.arrowBase2.x = arrowCanvas.arrowBase1.x
                arrowCanvas.arrowBase2.y = arrowCanvas.arrowBase1.y + _arrowBaseWidth
            }
        }
        arrowCanvas.requestPaint()
    } // function - _calcPositions

    QGCPalette { id: qgcPal }

    MouseArea {
        x:          _viewportMaxLeft
        y:          _viewportMaxTop
        width:      _viewportMaxRight -_viewportMaxLeft
        height:     _viewportMaxBottom - _viewportMaxTop
        visible:    checked
        onClicked:  {
            checked = false
            _root.clicked()
        }
    }

    RoundButton {
        id:             roundButton
        radius:         parent.width / 2
        onClicked:  {
            _calcPositions()
            _root.clicked()
        }
    }

    Item {
        id:         dropDownItem
        visible:    checked

        Canvas {
            id:             arrowCanvas
            anchors.fill:   parent

            property point arrowPoint: Qt.point(0, 0)
            property point arrowBase1: Qt.point(0, 0)
            property point arrowBase2: Qt.point(0, 0)

            onPaint: {
                var context = getContext("2d")
                context.reset()
                context.beginPath()

                context.moveTo(dropItemHolderRect.x, dropItemHolderRect.y)
                if (dropDirection == dropDown) {
                    context.lineTo(arrowBase2.x, arrowBase2.y)
                    context.lineTo(arrowPoint.x, arrowPoint.y)
                    context.lineTo(arrowBase1.x, arrowBase1.y)
                }
                context.lineTo(dropItemHolderRect.x + dropItemHolderRect.width, dropItemHolderRect.y)
                if (dropDirection == dropLeft) {
                    context.lineTo(arrowBase2.x, arrowBase2.y)
                    context.lineTo(arrowPoint.x, arrowPoint.y)
                    context.lineTo(arrowBase1.x, arrowBase1.y)
                }
                context.lineTo(dropItemHolderRect.x + dropItemHolderRect.width, dropItemHolderRect.y + dropItemHolderRect.height)
                if (dropDirection == dropUp) {
                    context.lineTo(arrowBase2.x, arrowBase2.y)
                    context.lineTo(arrowPoint.x, arrowPoint.y)
                    context.lineTo(arrowBase1.x, arrowBase1.y)
                }
                context.lineTo(dropItemHolderRect.x, dropItemHolderRect.y + dropItemHolderRect.height)
                if (dropDirection == dropRight) {
                    context.lineTo(arrowBase2.x, arrowBase2.y)
                    context.lineTo(arrowPoint.x, arrowPoint.y)
                    context.lineTo(arrowBase1.x, arrowBase1.y)
                }
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
            }
        }
    } // Item - dropDownItem
}
