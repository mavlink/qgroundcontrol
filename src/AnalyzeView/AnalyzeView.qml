import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.Controls

Rectangle {
    id:     _root
    color:  qgcPal.window
    z:      QGroundControl.zOrderTopMost

    readonly property real  _defaultTextHeight:     ScreenTools.defaultFontPixelHeight
    readonly property real  _defaultTextWidth:      ScreenTools.defaultFontPixelWidth
    readonly property real  _horizontalMargin:      _defaultTextWidth / 2
    readonly property real  _verticalMargin:        _defaultTextHeight / 2

    property var  _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property var  _currentPage:   null
    property var  _currentItem:   null
    // Destroy the in-panel page item while panelContainer is still alive, before the
    // loader clears the source and tears down this component.
    Component.onDestruction: mainWindow.destroyInPanelAnalyzePage()

    function _loadPage(source) {
        // Clear reference before calling mainWindow.createAnalyzePage (which destroys the old item).
        _currentItem = null
        if (source !== "") {
            // mainWindow creates and owns the item (QObject parent = mainWindow) so it
            // survives AnalyzeView being unloaded in the popped-out case.
            // anchors.fill: parent in AnalyzePage.qml automatically fills panelContainer.
            _currentItem = mainWindow.createAnalyzePage(source)
            if (_currentItem) {
                _currentItem.parent = panelContainer
            }
        } else {
            mainWindow.destroyInPanelAnalyzePage()
        }
    }

    function _updatePanelSource() {
        if (_currentPage) {
            if (_currentPage.requiresVehicle && !_activeVehicle) {
                _loadPage("")
            } else {
                _loadPage(_currentPage.url)
            }
        }
    }

    on_ActiveVehicleChanged: {
        if (_currentPage && _currentPage.requiresVehicle) {
            _loadPage("")
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
                        panelContainer.title = _currentPage.title
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
                        panelContainer.title = modelData.title
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

    Item {
        id:                     panelContainer
        anchors.topMargin:      _verticalMargin
        anchors.bottomMargin:   _verticalMargin
        anchors.leftMargin:     _horizontalMargin
        anchors.rightMargin:    _horizontalMargin
        anchors.left:           divider.right
        anchors.right:          parent.right
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom

        property string title

        Connections {
            target: _currentItem

            function onPopout() {
                var existingItem = _currentItem
                var pageTitle = panelContainer.title
                var pageSource = _currentPage.url
                var requiresVehicle = _currentPage ? _currentPage.requiresVehicle : false
                // Clear references before handing item to the popup window.
                _currentItem = null
                // Tell mainWindow this item has moved to a popup so it is not destroyed
                // when AnalyzeView is torn down.
                mainWindow.analyzePageMovedToPopup()
                existingItem.visible = false
                // Hand the existing item to the popout window.
                mainWindow.createWindowedAnalyzePage(pageTitle, pageSource, requiresVehicle, existingItem)
                // Create a fresh in-panel instance.
                _loadPage(pageSource)
            }
        }
    }

    QGCLabel {
        anchors.centerIn:   panelContainer
        text:               qsTr("Requires a connected vehicle")
        visible:            _currentPage && _currentPage.requiresVehicle && !_activeVehicle
    }
}
