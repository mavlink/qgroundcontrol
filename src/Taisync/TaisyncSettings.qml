/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Dialogs  1.2

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

Column {
    id:                 _taisyncSettings
    spacing:            ScreenTools.defaultFontPixelHeight * 0.5
    anchors.margins:    ScreenTools.defaultFontPixelWidth
    function saveSettings() {
        if(subEditConfig) {
            subEditConfig.videoEnabled = videoEnableCheck.checked
        }
    }
    QGCCheckBox {
        id:             videoEnableCheck
        text:           qsTr("Enable Taisync video link")
        Component.onCompleted: {
            checked = subEditConfig && subEditConfig.linkType === LinkConfiguration.TypeTaisync ? subEditConfig.videoEnabled : false
        }
    }
}
