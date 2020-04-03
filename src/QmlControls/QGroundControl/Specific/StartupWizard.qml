import QtQuick 2.11
import QtQuick.Layouts 1.11

import QGroundControl.ScreenTools 1.0
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl 1.0
import QGroundControl.Specific 1.0


Item {
    id: _root

    implicitWidth: contentColumn.implicitWidth + contentColumn.anchors.margins * 2.5
    implicitHeight: contentColumn.implicitHeight + contentColumn.anchors.margins * 2 + contentColumn.spacing * 3

    property bool forceKeepingOpen: _pageReady && pageLoader.item.forceConfirmation && !_armed

    signal closeView()

    property bool _pageReady: pageLoader.status === Loader.Ready
    property int _currentIndex: 0
    property int _pagesCount: QGroundControl.corePlugin.startupPages.length
    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property bool _armed: _activeVehicle && _activeVehicle.armed

    function doneOrJumpToNext() {
        if(_currentIndex < _pagesCount - 1)
            _currentIndex += 1
        else {
            _root.closeView()
            QGroundControl.settingsManager.appSettings.firstTimeStart.value = false
        }
    }

    Connections {
        target: _pageReady ? pageLoader.item : null
        onCloseView: doneOrJumpToNext()
    }

    ColumnLayout {
        id: contentColumn
        anchors.fill: parent
        anchors.margins: ScreenTools.defaultFontPixelHeight
        spacing: ScreenTools.defaultFontPixelHeight * 0.5

        QGCLabel {
            id: titleLabel
            text: qsTr("Welcome to " + QGroundControl.appName)
            color: qgcPal.text
            font.family: ScreenTools.demiboldFontFamily
            font.pointSize: ScreenTools.mediumFontPointSize
        }

        Rectangle {
            id: separatorRect
            height: 1
            color: qgcPal.windowShade
            Layout.fillWidth: true
        }

        Flickable {
            id: flickablePage
            clip: true

            contentWidth: pageLoader.item ? pageLoader.item.width : 0
            contentHeight: pageLoader.item ? pageLoader.item.height : 0

            Layout.fillHeight: true
            Layout.preferredWidth: contentWidth
            Layout.preferredHeight: contentHeight

            Loader {
                id: pageLoader
                source: QGroundControl.corePlugin.startupPages[_currentIndex]
            }
        }

        QGCButton {
            id: confirmButton
            property string _acknowledgeText: _pagesCount <= 1 ? qsTr("Next") : qsTr("Done")

            text: (_pageReady && pageLoader.item && pageLoader.item.doneText) ? pageLoader.item.doneText : _acknowledgeText
            onClicked: doneOrJumpToNext()
        }
    }
}
