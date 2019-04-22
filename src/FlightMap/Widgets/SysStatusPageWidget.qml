import QtQuick          2.3
import QtQuick.Layouts  1.2

import QGroundControl                    1.0
import QGroundControl.Controls           1.0
import QGroundControl.ScreenTools        1.0

/// Health page for Instrument Panel PageWidget
Column {
    width: pageWidth
    height: 80

    property bool   showSettingsIcon: false

    property string boardTemp: QGroundControl.sysStatusManager.boardTemputure

    Item {
        anchors.fill:   parent
        visible:        true

        Column {
            spacing: 1
            width: pageWidth

            QGCLabel {
                width:                  parent.width
                horizontalAlignment:    Text.AlignHCenter
                text:                   qsTr("Board Temperature (C)")
            }

            QGCLabel {
                width:                  parent.width
                horizontalAlignment:    Text.AlignHCenter
                text:                   boardTemp
            }
        }

    }
}
