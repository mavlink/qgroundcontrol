import QtQuick 2.15
import QtQuick.Layouts 1.15
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

Item {
    id: root
    width: 50
    height: 40

    Row {
        anchors.verticalCenter: parent.verticalCenter
        spacing: 4

        // Replace with your own icon if desired
        Image {
            id: literIcon
            source: "Images/LiterIcon.svg" // Put your SVG inside Images/ and update path if necessary
            width: 24
            height: 24
            fillMode: Image.PreserveAspectFit
        }

        Text {
            id: literText
            text: "liter"
            color: QGCPalette.text
            font.pixelSize: ScreenTools.defaultFontPixelSize
            verticalAlignment: Text.AlignVCenter
        }
    }
}
