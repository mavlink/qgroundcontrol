import QtQuick                  2.5
import QtQuick.Controls         1.4

import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.FlightMap     1.0

Item {
    id: _root
    clip: true
    height: valuesPage.height + pageIndicatorRow.anchors.topMargin + pageIndicatorRow.height

    property var    qgcView     ///< QGCView to use for showing dialogs
    property color  textColor
    property color  backgroundColor
    property var    maxHeight   ///< Maximum height that should be taken, smaller than this is ok

    property real   _margins:           ScreenTools.defaultFontPixelWidth / 2
    property real   _pageWidth:         _root.width
    property int    _currentPage:       0
    property int    _maxPage:           2

    function showPicker() {
        valuesPage.showPicker()
    }

    function showPage(pageIndex) {
        _root.height = Qt.binding(function() { return _root.children[pageIndex].height + pageIndicatorRow.anchors.topMargin + pageIndicatorRow.height } )
        _root.children[0].x = -(pageIndex * _pageWidth)
    }

    ValuesWidget {
        id:         valuesPage
        width:      _pageWidth
        qgcView:    _root.qgcView
        textColor:  _root.textColor
        maxHeight:  _root.maxHeight
    }

    VehicleHealthWidget {
        id:             healthPage
        anchors.left:   valuesPage.right
        width:          _pageWidth
        qgcView:        _root.qgcView
        textColor:      _root.textColor
        maxHeight:      _root.maxHeight
    }

    VibrationWidget {
        id:                 vibrationPage
        anchors.left:       healthPage.right
        width:              _pageWidth
        textColor:          _root.textColor
        backgroundColor:    _root.backgroundColor
        maxHeight:          _root.maxHeight
    }

    Row {
        id:                         pageIndicatorRow
        anchors.bottom:             parent.bottom
        anchors.horizontalCenter:   parent.horizontalCenter
        spacing:                    _margins

        Repeater {
            model: _maxPage + 1

            Rectangle {
                height:         radius * 2
                width:          radius * 2
                radius:         2.5
                border.color:   textColor
                border.width:   1
                color:          _currentPage == index ? textColor : "transparent"
            }
        }
    }

    MouseArea {
        anchors.fill: parent

        property real xDragStart
        property real xFirstPageSave

        onPressed: {
            if (mouse.button == Qt.LeftButton) {
                mouse.accepted = true
                xDragStart = mouse.x
                xFirstPageSave = _root.children[0].x
            }
        }

        onPositionChanged: {
            _root.children[0].x = xFirstPageSave + mouse.x - xDragStart
        }

        onReleased: {
            if (mouse.x < xDragStart) {
                // Swipe left
                _currentPage = Math.min(_currentPage + 1, _maxPage)
            } else {
                // Swipe right
                _currentPage = Math.max(_currentPage - 1, 0)
            }
            showPage(_currentPage)
        }
    }
}
