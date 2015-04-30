import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtGraphicalEffects 1.0

import QGroundControl.FactSystem 1.0
import QGroundControl.Palette 1.0
import QGroundControl.Controls 1.0

Rectangle {
    id: topLevel

    QGCPalette { id: palette; colorGroupEnabled: true }
    color: palette.window

    ExclusiveGroup { id: setupButtonGroup }

    Column {
        anchors.fill: parent

        SubMenuButton {
            id: firmwareButton; objectName: "firmwareButton"
            width: parent.width
            text: "FIRMWARE"
            imageResource: "FirmwareUpgradeIcon.png"
            setupIndicator: false
            exclusiveGroup: setupButtonGroup
            onClicked: controller.firmwareButtonClicked()
        }

        Item { width: parent.width; height: 10 }    // spacer

        QGCLabel {
            width: parent.width
            text: "Full setup options are only available when connected to vehicle and full parameter list has completed downloading."
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
