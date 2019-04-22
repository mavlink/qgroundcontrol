import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Layouts          1.2

import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.FlightMap     1.0

Item {
    id:     _root
    clip:   true
    height: column.height

    property color  textColor
    property color  backgroundColor
    property var    maxHeight       ///< Maximum height that should be taken, smaller than this is ok

    property real   _margins:       ScreenTools.defaultFontPixelWidth / 2
    property real   _pageWidth:     _root.width
    property int    _currentPage:   0
    property int    _maxPage:       3

    onWidthChanged: showPage(_currentPage)

    function showPicker() {
        valuesPage.showPicker()
    }

    function showPage(pageIndex) {
        pageRow.x = -(pageIndex * _pageWidth)
        _currentPage = pageIndex
    }

    function showNextPage() {
        if (_currentPage == _maxPage) {
            _currentPage = 0
        } else {
            _currentPage++
        }
        showPage(_currentPage)
    }

    function currentPage() {
        return _currentPage
    }

    MouseArea {
        anchors.fill:   parent
        onClicked:      showNextPage()
    }

    Column {
        id:             column
        anchors.left:   parent.left
        anchors.right:  parent.right

        Row {
            id: pageRow
            ValuesWidget {
                id:         valuesPage
                width:      _pageWidth
                textColor:  _root.textColor
                maxHeight:  _root.maxHeight
            }
            CameraWidget {
                width:      _pageWidth
                textColor:  _root.textColor
                maxHeight:  _root.maxHeight
            }
            VehicleHealthWidget {
                width:      _pageWidth
                textColor:  _root.textColor
                maxHeight:  _root.maxHeight
            }
            VibrationWidget {
                width:              _pageWidth
                textColor:          _root.textColor
                backgroundColor:    _root.backgroundColor
                maxHeight:          _root.maxHeight
            }
        }

        Row {
            anchors.horizontalCenter:   parent.horizontalCenter
            spacing:                    _margins

            Repeater {
                model: _maxPage + 1

                Rectangle {
                    height:         radius * 2
                    width:          radius * 2
                    radius:         ScreenTools.defaultFontPixelWidth / 3
                    border.color:   textColor
                    border.width:   1
                    color:          _currentPage == index ? textColor : "transparent"
                }
            }
        }
    }
}
