import QtQuick                      2.3
import QtQuick.Controls             1.2
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QGroundControl.ScreenTools   1.0

Image {
    id: root
    property var checkState: false
    property var imageOn
    property var imageOff
    signal clicked()

    source: checkState ? imageOff : imageOn
    fillMode: Image.PreserveAspectFit
    height: ScreenTools.defaultFontPixelHeight * 1.5
    width: ScreenTools.defaultFontPixelHeight * 1.5
    sourceSize.height:  height
    MouseArea {
        anchors.fill: parent
        onClicked: root.clicked()
    }
}
