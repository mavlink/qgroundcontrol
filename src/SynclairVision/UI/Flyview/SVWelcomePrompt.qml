import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs
import QGroundControl.FirstRunPromptDialogs

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls


FirstRunPrompt {
    id: root
    title: qsTr("Welcome to Synclair: QGroundControl")
    promptId: QGroundControl.corePlugin.svInitialWelcomePromptId

    // Ensure it updates settings when closed so it won't open again
    onClosed: {
        var appSettings = QGroundControl.settingsManager.appSettings
        var shownIds = appSettings.firstRunPromptIdsShown.rawValue
        
        if (!shownIds.includes(promptId)) {
            shownIds.push(promptId)
            appSettings.firstRunPromptIdsShown.rawValue = shownIds
        }
    }
    Flickable {
        id: contentFlickable
        readonly property real scrollBarGutterWidth: Math.max(contentScrollBar.implicitWidth, ScreenTools.defaultFontPixelWidth)

        width: 700
        height: 600
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        contentWidth: width
        contentHeight: test.height


        ScrollBar.vertical: ScrollBar {
            id: contentScrollBar
        }

        Rectangle {
            id: test
            width: 700
            height: 1500
            color: "red"
            opacity: 0.1
        }
    }
}
