import QtQuick

import QGroundControl
import QGroundControl.ScreenTools
import QGroundControl.Controls

QGCTabBar {
    id: tabBar

    Component.onCompleted: currentIndex = QGroundControl.settingsManager.planViewSettings.displayPresetsTabFirst.rawValue ? 2 : 0

    QGCTabButton { icon.source: "/qmlimages/PatternGrid.png"; icon.height: ScreenTools.defaultFontPixelHeight }
    QGCTabButton { icon.source: "/qmlimages/PatternCamera.png"; icon.height: ScreenTools.defaultFontPixelHeight }
    QGCTabButton { icon.source: "/qmlimages/PatternTerrain.png"; icon.height: ScreenTools.defaultFontPixelHeight }
    QGCTabButton { icon.source: "/qmlimages/PatternPresets.png"; icon.height: ScreenTools.defaultFontPixelHeight }
}
