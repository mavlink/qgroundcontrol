import QGroundControl
import QGroundControl.Controls

ToolStripAction {
    id: root

    property bool _is3DViewOpen: QGCViewer3DManager.displayMode === QGCViewer3DManager.View3D
    property bool _viewer3DEnabled: QGroundControl.settingsManager.viewer3DSettings.enabled.rawValue

    iconSource: _is3DViewOpen ? "/qmlimages/PaperPlane.svg" : "/qml/QGroundControl/Viewer3D/City3DMapIcon.svg"
    text: _is3DViewOpen ? qsTr("Fly") : qsTr("3D View")
    visible: _viewer3DEnabled

    onTriggered: {
        if (_is3DViewOpen) {
            QGCViewer3DManager.setDisplayMode(QGCViewer3DManager.Map);
        } else {
            QGCViewer3DManager.setDisplayMode(QGCViewer3DManager.View3D);
        }
    }
}
