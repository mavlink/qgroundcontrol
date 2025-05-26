import QtQuick

import QGroundControl.ScreenTools

Rectangle {
    id:         control
    z:          10
    color:      _flickable.indicatorColor
    opacity:    _opacity
    visible:    showIndicator
    state:      orientation == QGCFlickableScrollIndicator.Vertical ? "vertical" : "horizontal"

    property bool showIndicator: false
    property int  orientation:   QGCFlickableScrollIndicator.Vertical   

    enum Orientation {
        Vertical,
        Horizontal
    }

    property real _opacity:     0.5
    property var  _flickable:   parent

    states: [
            State {
                name: "vertical"
                AnchorChanges { 
                    target:         control
                    anchors.right: _flickable.right
                }

                PropertyChanges {
                    target:         control
                    y:              _flickable.height * (_flickable.contentY / _flickable.contentHeight)
                    width:          ScreenTools.defaultFontPixelWidth / 2
                    height:         _flickable.height * (_flickable.height / _flickable.contentHeight)
                    showIndicator:  (_flickable.flickableDirection === Flickable.AutoFlickDirection ||
                                        _flickable.flickableDirection === Flickable.VerticalFlick ||
                                        _flickable.flickableDirection === Flickable.HorizontalAndVerticalFlick) &&
                                        (_flickable.contentHeight > _flickable.height)
                }
            },

            State {
                name: "horizontal"
                AnchorChanges { 
                    target:         control
                    anchors.bottom: _flickable.bottom
                }

                PropertyChanges {
                    target:         control
                    x:              _flickable.width * (_flickable.contentX / _flickable.contentWidth)
                    height:         ScreenTools.defaultFontPixelWidth / 2
                    width:          _flickable.width * (_flickable.width / _flickable.contentWidth)
                    showIndicator:  (_flickable.flickableDirection === Flickable.AutoFlickDirection ||
                                        _flickable.flickableDirection === Flickable.HorizontalFlick ||
                                        _flickable.flickableDirection === Flickable.HorizontalAndVerticalFlick) &&
                                        (_flickable.contentWidth > _flickable.width)
                }
            }
        ]

    Component.onCompleted:  { if (animateOpacity) animateOpacity.restart() }
    onVisibleChanged:       { if (animateOpacity) animateOpacity.restart() }
    onHeightChanged:        { if (animateOpacity) animateOpacity.restart() }
    onWidthChanged:         { if (animateOpacity) animateOpacity.restart() }

    Connections {
        target: control._flickable
        function onMovementStarted()        { control.opacity = control._opacity }
        function onMovementEnded()          { animateOpacity.restart() }
        function onContentHeightChanged()   { animateOpacity.restart() }
    }

    NumberAnimation {
        id:            animateOpacity
        target:        control
        properties:    "opacity"
        from:          control._opacity
        to:            0.0
        duration:      2000
        easing.type:   Easing.InQuint
    }
}
