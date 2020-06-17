import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Layouts 1.3
import org.freedesktop.gstreamer.GLVideoItem 1.0

Window {
    visible: true
    width: 640
    height: 480
    title: qsTr("VideoReceiverApp")

    RowLayout {
        anchors.fill: parent
        spacing: 0

        GstGLVideoItem {
            id: video
            objectName: "videoItem"
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
