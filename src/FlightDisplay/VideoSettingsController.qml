/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                  2.3
import QtQuick.Controls         2.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Dialogs          1.2
import QtQuick.Layouts          1.2

import QGroundControl                       1.0
import QGroundControl.FactSystem            1.0
import QGroundControl.FactControls          1.0
import QGroundControl.Controls              1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.Palette               1.0
import QGroundControl.Controllers           1.0
import QGroundControl.SettingsManager       1.0
import QGroundControl.QgcQtGStreamer        1.0

Rectangle {
    id: root
    // color: qgcPal.windowShade
    focus: true
    width: childrenRect.width + 10
    height: childrenRect.height + 10
    property alias videoReceiver : internalSettings.videoReceiver

    VideoSettings2 {
        id: internalSettings
    }

   ColumnLayout {
        spacing: 6
        Layout.margins: 6
        x: 10

        Label {
            id:         videoSectionLabel
            text:       qsTr("Video")
            Layout.alignment: Qt.AlignHCenter
        }

        GridLayout {
            id:                         videoGrid
            Layout.fillWidth:           false
            Layout.fillHeight:          false
            columns:                    2
            visible:                videoSectionLabel.visible

            Label {
                text:                   qsTr("Video Source")
            }
            ComboBox {
                id:                     videoSource
            }

            Label {
                text:                   qsTr("UDP Port")
            }
            TextField {
            }

            Label {
                text:                   qsTr("URL")
            }
            TextField {
            }
        }

        Item { width: 1; height: _margins }

        Label {
            id:                             videoRecSectionLabel
            text:                           qsTr("Video Recording")
            Layout.alignment: Qt.AlignHCenter
        }

        GridLayout {
            id:                         videoRecCol
            Layout.fillWidth:           false
            columns:                    2
            visible:                    videoRecSectionLabel.visible

            Label {
                text:                   qsTr("Auto-Delete Files")
            }
            CheckBox {
                text:                   ""
            }

            Label {
                text:                   qsTr("Max Storage Usage")
            }
            TextField {
            }

            Label {
            }
            ComboBox {
            }
        }

        Button {
            text: "Close"
            onClicked: root.visible = false
        }
    }
}
