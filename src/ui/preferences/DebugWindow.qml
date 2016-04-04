/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.2
import QtQuick.Window 2.2

import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.Controllers 1.0
import QGroundControl.ScreenTools 1.0

QGCView {
    id:         qgcView
    viewPanel:  panel

    QGCPalette { id: qgcPal; colorGroupEnabled: panel.enabled }

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent

        Text {
            id:     _textMeasure
            text:   "X"
            color:  qgcPal.window
        }

        GridLayout {
            anchors.margins: 20
            anchors.top:     parent.top
            anchors.left:    parent.left
            columns: 2
            Text {
                text:   qsTr("Qt Platform:")
                color:  qgcPal.text
            }
            Text {
                text:   Qt.platform.os
                color:  qgcPal.text
            }
            Text {
                text:   qsTr("Default font width:")
                color:  qgcPal.text
            }
            Text {
                text:   _textMeasure.contentWidth
                color:  qgcPal.text
            }
            Text {
                text:   qsTr("Default font height:")
                color:  qgcPal.text
            }
            Text {
                text:   _textMeasure.contentHeight
                color:  qgcPal.text
            }
            Text {
                text:   qsTr("Default font pixel size:")
                color:  qgcPal.text
            }
            Text {
                text:   _textMeasure.font.pixelSize
                color:  qgcPal.text
            }
            Text {
                text:   qsTr("Default font point size:")
                color:  qgcPal.text
            }
            Text {
                text:   _textMeasure.font.pointSize
                color:  qgcPal.text
            }
            Text {
                text:   qsTr("QML Screen Desktop:")
                color:  qgcPal.text
            }
            Text {
                text:   Screen.desktopAvailableWidth + " x " + Screen.desktopAvailableHeight
                color:  qgcPal.text
            }
            Text {
                text:   qsTr("QML Screen Size:")
                color:  qgcPal.text
            }
            Text {
                text:   Screen.width + " x " + Screen.height
                color:  qgcPal.text
            }
            Text {
                text:   qsTr("QML Pixel Density:")
                color:  qgcPal.text
            }
            Text {
                text:   Screen.pixelDensity
                color:  qgcPal.text
            }
            Text {
                text:   qsTr("QML Pixel Ratio:")
                color:  qgcPal.text
            }
            Text {
                text:   Screen.devicePixelRatio
                color:  qgcPal.text
            }
        }

        Rectangle {
            width:              100
            height:             100
            color:              qgcPal.text
            anchors.right:      parent.right
            anchors.bottom:     parent.bottom
            anchors.margins:    10
            Text {
                text: "100x100"
                anchors.centerIn: parent
                color:  qgcPal.window
            }
        }
    }
}
