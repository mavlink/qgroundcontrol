import QtQuick 2.2
import QtQuick.Controls 1.2

import QGroundControl.ScreenTools 1.0
import QGroundControl.ScreenToolsController 1.0

Item {
    Component.onCompleted: ScreenToolsController.qmlDefaultFontPixelSize = ScreenTools.defaultFontPixelSize
}
