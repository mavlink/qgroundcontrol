import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QGroundControl.FactControls 1.0


Rectangle {
    QGCPalette { id: palette; colorGroup: enabled ? QGCPalette.Active : QGCPalette.Disabled }

    width: 100
    height: 100
    color: "#e43f3f"
    // palette.windowText
}
