import QtQuick
import QtQuick.Effects

import QGroundControl
import QGroundControl.Controls

Item {
    id: root

    width: overlayToggle.implicitWidth

    property bool showIndicator: true

    readonly property bool welcomePromptAcknowledged: {
        const shownIds = QGroundControl.settingsManager.appSettings.firstRunPromptIdsShown.rawValue
        return shownIds.includes(QGroundControl.corePlugin.svInitialWelcomePromptId)
    }

    MultiEffect {
        id: overlayGlow

        anchors.fill: overlayToggle
        source: overlayToggle
        visible: !SVState.synclairOverlay && !root.welcomePromptAcknowledged
        shadowEnabled: true
        shadowColor: QGroundControl.globalPalette.colorYellow
        shadowBlur: 1

        SequentialAnimation on shadowOpacity {
            running: overlayGlow.visible
            loops: Animation.Infinite
            NumberAnimation { from: 0; to: 1; duration: 1000; easing.type: Easing.InOutSine }
            NumberAnimation { from: 1; to: 0; duration: 1000; easing.type: Easing.InOutSine }
        }
    }

    QGCCheckBoxSlider {
        id: overlayToggle

        anchors.centerIn: parent
        text: ScreenTools.isTinyScreen ? qsTr("Synclair") : qsTr("Synclair Vision: QGroundControl")
        checked: SVState.synclairOverlay

        onToggled: SVState.synclairOverlay = checked
    }
}
