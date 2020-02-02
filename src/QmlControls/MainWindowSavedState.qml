/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.11
import QtQuick.Window   2.11
import QtQuick.Controls 2.4
import Qt.labs.settings 1.0

import QGroundControl.ScreenTools 1.0

Item {
    property Window window

    Settings {
        id:         s
        category:   "MainWindowState"

        property int x
        property int y
        property int width
        property int height
        property int visibility
    }

    Component.onCompleted: {
        if (!ScreenTools.isMobile && s.width && s.height) {
            window.x = s.x;
            window.y = s.y;
            window.width = s.width;
            window.height = s.height;
            window.visibility = s.visibility;
        }
    }

    Connections {
        target:                 ScreenTools.isMobile ? null : window
        onXChanged:             saveSettingsTimer.restart()
        onYChanged:             saveSettingsTimer.restart()
        onWidthChanged:         saveSettingsTimer.restart()
        onHeightChanged:        saveSettingsTimer.restart()
        onVisibilityChanged:    saveSettingsTimer.restart()
    }

    Timer {
        id:             saveSettingsTimer
        interval:       1000
        repeat:         false
        onTriggered:    saveSettings()
    }

    function saveSettings() {
        switch(window.visibility) {
        case ApplicationWindow.Windowed:
            s.x = window.x;
            s.y = window.y;
            s.width = window.width;
            s.height = window.height;
            s.visibility = window.visibility;
            break;
        case ApplicationWindow.FullScreen:
            s.visibility = window.visibility;
            break;
        case ApplicationWindow.Maximized:
            s.visibility = window.visibility;
            break;
        }
    }
}
