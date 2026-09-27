pragma Singleton

import QtQml

/// Display formatting shared by the GPS and correction status views.
QtObject {
    function dataRate(bytesPerSecond: real): string {
        //: Data rate in bytes per second
        if (bytesPerSecond < 1024) return qsTr("%1 B/s").arg(bytesPerSecond.toFixed(0))
        //: Data rate in kilobytes per second
        return qsTr("%1 KB/s").arg((bytesPerSecond / 1024).toFixed(1))
    }
}
