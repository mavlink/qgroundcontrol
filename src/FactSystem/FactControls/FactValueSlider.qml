import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Dialogs  1.2

import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactSystem    1.0

Rectangle {
    height: _itemHeight
    width:  _totalSlots * _itemWidth
    color:  qgcPal.textField

    property Fact   fact:               undefined
    property int    digitCount:         4           ///< The minimum number of digits to show for each value
    property int    incrementSlots:     1           ///< The number of visible slots to left/right of center value

    property int    _adjustedDigitCount:    Math.max(digitCount, _model.initialValueAtPrecision.toString().length)
    property int    _totalDigitCount:       _adjustedDigitCount + 1 + fact.units.length
    property real   _margins:               (ScreenTools.implicitTextFieldHeight - ScreenTools.defaultFontPixelHeight) / 2
    property real   _increment:             fact.increment
    property real   _value:                 fact.value
    property int    _decimalPlaces:         fact.decimalPlaces
    property string _units:                 fact.units
    property real   _prevValue:             _value - _increment
    property real   _nextValue:             _value + _increment
    property real   _itemWidth:             (_totalDigitCount * ScreenTools.defaultFontPixelWidth) + (_margins * 2)
    property real   _itemHeight:            ScreenTools.implicitTextFieldHeight
    property var    _valueModel
    property int    _totalSlots:            (incrementSlots * 2) + 1
    property int    _currentIndex:          _totalSlots / 2
    property int    _currentRelativeIndex:  _currentIndex
    property int    _prevIncrementSlots:    incrementSlots
    property int    _nextIncrementSlots:    incrementSlots
    property int    _selectionWidth:        3
    property var    _model:                 fact.valueSliderModel()
    property var    _fact:                  fact

    QGCPalette { id: qgcPal; colorGroupEnabled: parent.enabled }
    QGCPalette { id: qgcPalDisabled; colorGroupEnabled: false }

    function firstVisibleIndex() {
        return valueListView.contentX / _itemWidth
    }

    function recalcRelativeIndex() {
        _currentRelativeIndex = _currentIndex - firstVisibleIndex()
        _prevIncrementSlots = _currentRelativeIndex
        _nextIncrementSlots = _totalSlots - _currentRelativeIndex - 1
    }

    function reset() {
        valueListView.positionViewAtIndex(0, ListView.Beginning)
        _currentIndex = _model.resetInitialValue()
        valueListView.positionViewAtIndex(_currentIndex, ListView.Center)
        recalcRelativeIndex()
    }

    Component.onCompleted: {
        valueListView.maximumFlickVelocity = valueListView.maximumFlickVelocity / 2
        reset()
    }

    Connections {
        target:         _fact
        onValueChanged: reset()
    }

    Component {
        id: editDialogComponent

        ParameterEditorDialog {
            fact:       _fact
            setFocus:   ScreenTools.isMobile ? false : true // Works around strange android bug where wrong virtual keyboard is displayed
        }
    }

    QGCListView {
        id:             valueListView
        anchors.fill:   parent
        orientation:    ListView.Horizontal
        snapMode:       ListView.SnapToItem
        clip:           true
        model:          _model

        delegate: QGCLabel {
            width:                  _itemWidth
            height:                 _itemHeight
            verticalAlignment:      Text.AlignVCenter
            horizontalAlignment:    Text.AlignHCenter
            text:                   value + " " + _units
            color:                  qgcPal.textFieldText

            MouseArea {
                anchors.fill:   parent
                onClicked: {
                    valueListView.focus = true
                    if (_currentIndex === index) {
                        mainWindow.showComponentDialog(editDialogComponent, qsTr("Value Details"), mainWindow.showDialogDefaultWidth, StandardButton.Save | StandardButton.Cancel)
                    } else {
                        _currentIndex = index
                        valueListView.positionViewAtIndex(_currentIndex, ListView.Center)
                        recalcRelativeIndex()
                        fact.value = value
                    }
                }
            }
        }

        onMovementStarted: valueListView.focus = true

        onMovementEnded: {
            _currentIndex = firstVisibleIndex() + _currentRelativeIndex
            fact.value = _model.valueAtModelIndex(_currentIndex)
        }
    }

    Rectangle {
        id:         leftOverlay
        width:      _itemWidth * _prevIncrementSlots
        height:     _itemHeight
        color:      qgcPal.textField
        opacity:    0.5
    }

    Rectangle {
        width:          _itemWidth * _nextIncrementSlots
        height:         _itemHeight
        anchors.right:  parent.right
        color:          qgcPal.textField
        opacity:        0.5
    }

    Rectangle {
        x:              _currentRelativeIndex * _itemWidth - _borderWidth
        y:              -_borderWidth
        width:          _itemWidth + (_borderWidth * 2)
        height:         _itemHeight + (_borderWidth * 2)
        border.width:   _borderWidth
        border.color:   qgcPal.brandingBlue
        color:          "transparent"

        readonly property int _borderWidth: 3
    }
}
