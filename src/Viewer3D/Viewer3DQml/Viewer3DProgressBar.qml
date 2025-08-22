/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightDisplay
import QGroundControl.FlightMap
import QGroundControl.Viewer3D

Rectangle{
    id: progressBody

    property real progressValue: 100.0
    property string progressText: qsTr("Progress")

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    width:          ScreenTools.screenWidth * 0.2
    height: _progressCol.height + 2 * ScreenTools.defaultFontPixelWidth

    radius: ScreenTools.defaultFontPixelWidth * 2
    color: qgcPal.windowShadeDark

    visible: progressValue < 100.0
    opacity:  (progressValue < 100)?(1.0):(0.0)

    Behavior on opacity {
        NumberAnimation { duration: 300 }
    }

    Column{
        id: _progressCol
        anchors{
            verticalCenter: parent.verticalCenter
            right: parent.right
            left: parent.left
        }
        ProgressBar {
            id: _progressBar
            anchors{
                right: parent.right
                left: parent.left
                margins: ScreenTools.defaultFontPixelWidth
            }
            from:           0
            to:             100
            value:          progressBody.progressValue
        }

        QGCLabel {
            anchors.horizontalCenter: parent.horizontalCenter
            text:                progressText + Number(Math.floor(progressBody.progressValue)) + " %"
            color:              qgcPal.text
            font.bold:          true
            font.pointSize:     ScreenTools.mediumFontPointSize
            horizontalAlignment:Text.AlignHCenter
        }
    }
}
