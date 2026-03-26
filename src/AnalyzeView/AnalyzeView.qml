import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.Controls

Rectangle {
    id:     _root
    color:  qgcPal.window
    z:      QGroundControl.zOrderTopMost

    signal popout()

    readonly property real  _defaultTextHeight:     ScreenTools.defaultFontPixelHeight
    readonly property real  _defaultTextWidth:      ScreenTools.defaultFontPixelWidth
    readonly property real  _horizontalMargin:      _defaultTextWidth / 2
    readonly property real  _verticalMargin:        _defaultTextHeight / 2

    property var  _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property var  _currentPage:   null

    function _updatePanelSource() {
        if (_currentPage) {
            if (_currentPage.requiresVehicle && !_activeVehicle) {
                panelLoader.source = ""
            } else {
                panelLoader.source = _currentPage.url
            }
        }
    }

    on_ActiveVehicleChanged: {
        if (_currentPage && _currentPage.requiresVehicle) {
            panelLoader.source = ""
            if (_activeVehicle) {
                Qt.callLater(_updatePanelSource)
            }
        }
    }

    // This need to block click event leakage to underlying map.
    DeadMouseArea {
        anchors.fill: parent
    }

    QGCFlickable {
        id:                 buttonScroll
        width:              buttonColumn.width
        anchors.topMargin:  _defaultTextHeight / 2
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        anchors.leftMargin: _horizontalMargin
        anchors.left:       parent.left
        contentHeight:      buttonColumn.height
        flickableDirection: Flickable.VerticalFlick
        clip:               true

        Column {
            id:         buttonColumn
            width:      _maxButtonWidth
            spacing:    _defaultTextHeight / 2

            property real _maxButtonWidth: {
                var maxW = 0
                for (var i = 0; i < buttonRepeater.count; i++) {
                    var item = buttonRepeater.itemAt(i)
                    if (item) maxW = Math.max(maxW, item.implicitWidth)
                }
                return maxW
            }

            Repeater {
                id:     buttonRepeater
                model:  QGroundControl.corePlugin ? QGroundControl.corePlugin.analyzePages : []

                Component.onCompleted: {
                    if (count > 0) {
                        itemAt(0).checked = true
                        _currentPage = QGroundControl.corePlugin.analyzePages[0]
                        panelLoader.title = _currentPage.title
                        _updatePanelSource()
                    }
                }

                SubMenuButton {
                    imageResource:      modelData.icon
                    autoExclusive:      true
                    text:               modelData.title
                    width:              buttonColumn._maxButtonWidth

                    onClicked: {
                        _currentPage        = modelData
                        panelLoader.title   = modelData.title
                        checked             = true
                        _updatePanelSource()
                    }
                }
            }
        }
    }

    Rectangle {
        id:                     divider
        anchors.topMargin:      _verticalMargin
        anchors.bottomMargin:   _verticalMargin
        anchors.leftMargin:     _horizontalMargin
        anchors.left:           buttonScroll.right
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
        width:                  1
        color:                  qgcPal.windowShade
    }

    Loader {
        id:                     panelLoader
        anchors.topMargin:      _verticalMargin
        anchors.bottomMargin:   _verticalMargin
        anchors.leftMargin:     _horizontalMargin
        anchors.rightMargin:    _horizontalMargin
        anchors.left:           divider.right
        anchors.right:          parent.right
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
        source:                 ""

        property string title

        Connections {
            target:     panelLoader.item
            function onPopout() { mainWindow.createWindowedAnalyzePage(panelLoader.title, panelLoader.source, _currentPage ? _currentPage.requiresVehicle : false) }
        }
    }

    QGCLabel {
        anchors.centerIn:   panelLoader
        text:               qsTr("Requires a connected vehicle")
        visible:            _currentPage && _currentPage.requiresVehicle && !_activeVehicle
    }
}
