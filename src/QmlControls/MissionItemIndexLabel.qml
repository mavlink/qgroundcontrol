import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4

import QGroundControl.ScreenTools 1.0
import QGroundControl.Palette     1.0

Canvas {
    id:     root
    width:  _width + (_singleChar ? 0 : _label.width)
    height: specifiesCoordinate ? (_width * 1.5) : _width

    signal clicked

    property alias  label:                  _label.text
    property bool   checked:                false
    property bool   small:                  false
    property var    color:                  checked ? "green" : qgcPal.mapButtonHighlight
    property real   anchorPointX:           _width / 2
    property real   anchorPointY:           _width * 1.5
    property bool   specifiesCoordinate:    true

    property real _width:       small ? ScreenTools.defaultFontPixelHeight * ScreenTools.smallFontPointRatio * 1.25 : ScreenTools.defaultFontPixelHeight * 1.25
    property bool _singleChar:  _label.text.length <= 1


    onColorChanged: requestPaint()

    QGCPalette { id: qgcPal }

    function paintSingleCoordinate(context) {
        context.arc(_width / 2, _width / 2, _width / 2, (Math.PI / 8) * 7, Math.PI / 8);
        context.lineTo(_width / 2, _width * 1.5)
    }

    function paintSingleNoCoordinate(context) {
        context.arc(_width / 2, _width / 2, _width / 2, Math.PI * 2, 0);
    }

    function paintMultipleCoordinate(context) {
        context.arc(_width / 2, _width / 2, _width / 2, (Math.PI / 8) * 7, (Math.PI / 2) * 3);
        context.lineTo(_label.width, 0)
        context.arc(_label.width, _width / 2, _width / 2, (Math.PI / 2) * 3, Math.PI / 2);
        context.lineTo((_width / 4) * 3, _width)
        context.lineTo(_width / 2, _width * 1.5)
    }

    onPaint: {
        var context = getContext("2d")
        context.reset()
        context.beginPath()
        if (_singleChar) {
            if (specifiesCoordinate) {
                paintSingleCoordinate(context)
            } else {
                paintSingleNoCoordinate(context)
            }
        } else {
            paintMultipleCoordinate(context)
        }
        context.closePath()
        context.fillStyle = color
        context.fill()
    }

    QGCLabel {
        id:             _label
        x:              Math.round((_width / 2) - (_singleChar ? (width / 2) : (ScreenTools.defaultFontPixelWidth / 2)))
        y:              Math.round((_width / 2) - (height / 2))
        color:          "white"
        font.pointSize: small ? ScreenTools.smallFontPointSize : ScreenTools.defaultFontPointSize

        onWidthChanged: requestPaint()
    }

    QGCMouseArea {
        fillItem:   parent
        onClicked:  parent.clicked()
    }
}
