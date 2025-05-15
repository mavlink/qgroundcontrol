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
import QtQuick.Layouts

import QGroundControl
import QGroundControl.ScreenTools
import QGroundControl.Palette

/// Drop panel that displays positioned next to the specified click position.
/// By default the panel drops to the right of the click position. If there isn't
/// enough room to the right then the panel will drop to the left.
Popup {
    id:             _root
    padding:        _innerMargin
    leftPadding:    _dropRight ? _innerMargin + _arrowPointWidth : _innerMargin
    rightPadding:   _dropRight ? _innerMargin : _innerMargin + _arrowPointWidth
    modal:          true
    focus:          true
    closePolicy:    Popup.CloseOnEscape | Popup.CloseOnPressOutside
    clip:           false
    dim:            false

    property var  sourceComponent                                               // Component to display within the popup
    property var  clickRect:        Qt.rect(0, 0, 0, 0)                         // Rectangle of clicked item - used to position drop down
    property var  dropViewPort:     Qt.rect(0, 0, parent.width, parent.height)  // Available viewport for dropdown

    property var  _qgcPal:              QGroundControl.globalPalette
    property real _innerMargin:         ScreenTools.defaultFontPixelWidth * 0.5 // Margin between content and rectanglular portion of background
    property real _arrowPointWidth:     ScreenTools.defaultFontPixelWidth * 2   // Distance from vertical side to point
    property real _arrowPointPositionY: height / 2
    property bool _dropRight:           true

    onAboutToShow: {
        // Panel defaults to dropping to the right of click position
        let xPos = clickRect.x + clickRect.width

        // If there isn't room to the right then we switch to drop to the left
        if (xPos + _root.width > dropViewPort.x + dropViewPort.width) {
            _dropRight = false
            xPos = clickRect.x - _root.width
        }

        // Default position of panel is vertically centered on click position
        let yPos = clickRect.y + (clickRect.height / 2)
        yPos -= _root.height / 2

        // Make sure panel is within viewport
        let originalYPos = yPos
        yPos = Math.max(yPos, dropViewPort.y)
        yPos = Math.min(yPos, dropViewPort.y + dropViewPort.height - _root.height)

        _root.x = xPos
        _root.y = yPos

        // Adjust arrow position back to point at click position
        _arrowPointPositionY += originalYPos - yPos
    }

    background: Item {
        implicitWidth:  contentItem.implicitWidth + _innerMargin * 2 + _arrowPointWidth
        implicitHeight: contentItem.implicitHeight + _innerMargin * 2

        Rectangle {
            x:      _dropRight ? _arrowPointWidth : 0
            radius: ScreenTools.defaultFontPixelHeight / 2
            width:  parent.implicitWidth - _arrowPointWidth
            height: parent.implicitHeight
            color:  _qgcPal.window
        }

        // Arrowhead
        Canvas {
            x:      _dropRight ? 0 : parent.width - _arrowPointWidth
            y:      _arrowPointPositionY - _arrowPointWidth
            width:  _arrowPointWidth
            height: _arrowPointWidth * 2
            
            onPaint: {
                var context = getContext("2d")
                context.reset()
                context.beginPath()
                context.moveTo(_dropRight ? 0 : _arrowPointWidth, _arrowPointWidth)
                context.lineTo(_dropRight ? _arrowPointWidth : 0, 0)
                context.lineTo(_dropRight ? _arrowPointWidth : 0, _arrowPointWidth * 2)
                context.closePath()
                context.fillStyle = _qgcPal.window
                context.fill()
            }
        }
    }

    contentItem: SettingsGroupLayout {
        Loader {
            sourceComponent: _root.sourceComponent
        }
    }
}
