import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4

import QGroundControl.ScreenTools 1.0
import QGroundControl.Palette     1.0

Canvas {
    id:     root
    width:  _width
    height: width

    signal clicked

    property string label
    property bool   checked:                false
    property bool   small:                  false
    property var    color:                  checked ? "green" : qgcPal.mapButtonHighlight
    property real   anchorPointX:           _width / 2
    property real   anchorPointY:           _width / 2
    property bool   specifiesCoordinate:    true
    property real   gimbalYaw
    property real   vehicleYaw
    property bool   showGimbalYaw:          false

    property real _width:           showGimbalYaw ? _gimbalYawRadius * 2 : _indicatorRadius * 2
    property bool _singleChar:      _label.text.length <= 1
    property real _gimbalYawRadius: ScreenTools.defaultFontPixelHeight
    property real _indicatorRadius: small ? (ScreenTools.defaultFontPixelHeight * ScreenTools.smallFontPointRatio * 1.25 / 2) : (ScreenTools.defaultFontPixelHeight * 0.66)
    property real _gimbalRadians:   degreesToRadians(vehicleYaw + gimbalYaw - 90)

    onColorChanged:         requestPaint()
    onShowGimbalYawChanged: requestPaint()
    onGimbalYawChanged:     requestPaint()
    onVehicleYawChanged:    requestPaint()

    QGCPalette { id: qgcPal }

    function degreesToRadians(degrees) {
        return (Math.PI/180)*degrees
    }

    function paintGimbalYaw(context) {
        if (showGimbalYaw) {
            context.save()
            context.globalAlpha = 0.75
            context.beginPath()
            context.moveTo(anchorPointX, anchorPointY)
            context.arc(anchorPointX, anchorPointY, _gimbalYawRadius,  _gimbalRadians + degreesToRadians(45), _gimbalRadians + degreesToRadians(-45), true /* clockwise */)
            context.closePath()
            context.fillStyle = "white"
            context.fill()
            context.restore()
        }
    }

    function paintSingleCoordinate(context) {
        context.beginPath()
        context.arc(_width / 2, _width / 2, _width / 2, (Math.PI / 8) * 7, Math.PI / 8);
        context.lineTo(_width / 2, _width * 1.5)
        context.closePath()
        context.fillStyle = color
        context.fill()
    }

    function paintSingleNoCoordinate(context) {
        context.save()
        context.beginPath()
        context.translate(anchorPointX, anchorPointY)
        context.arc(0, 0, _indicatorRadius, Math.PI * 2, 0);
        context.closePath()
        context.fillStyle = color
        context.fill()
        context.restore()
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
        context.clearRect(0, 0, width, height)
        paintGimbalYaw(context)
        /*
        if (_singleChar) {
            if (specifiesCoordinate) {
                paintSingleCoordinate(context)
            } else {
                paintSingleNoCoordinate(context)
            }
        } else {
            paintMultipleCoordinate(context)
        }
        */
        paintSingleNoCoordinate(context)
    }

    QGCLabel {
        id:                     _label
        anchors.centerIn:       parent
        width:                  _indicatorRadius * 2
        height:                 width
        horizontalAlignment:    Text.AlignHCenter
        verticalAlignment:      Text.AlignVCenter
        color:                  "white"
        font.pointSize:         ScreenTools.defaultFontPointSize
        fontSizeMode:           Text.HorizontalFit
        text:                   label
    }

    QGCMouseArea {
        fillItem:   parent
        onClicked:  parent.clicked()
    }
}
