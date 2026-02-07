import QtQuick
import QGroundControl
import QGroundControl.Controls

Item {
    id: root
    height: parent.height

    QGCPalette { id: qgcPal }

    readonly property string _logoSource: (qgcPal.globalTheme === QGCPalette.Light
        ? QGroundControl.corePlugin.brandImageOutdoor
        : QGroundControl.corePlugin.brandImageIndoor)

    // FlyViewToolBarIndicators expects this property
    property bool showIndicator: _logoSource.length > 0

    Image {
        id: logo
        anchors.verticalCenter: parent.verticalCenter
        height: ScreenTools.defaultFontPixelHeight * 2
        width: (implicitWidth > 0 && implicitHeight > 0)
            ? (height * implicitWidth / implicitHeight)
            : height * 3
        fillMode: Image.PreserveAspectFit
        source: _logoSource
        sourceSize.height: height
    }

    width: logo.width
}