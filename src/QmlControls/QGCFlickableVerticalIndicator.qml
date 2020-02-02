import QtQuick 2.3

Rectangle {
    id:                    verticalIndicator
    anchors.rightMargin:   2
    anchors.right:         parent.right
    y:                     parent.height * (parent.contentY / parent.contentHeight)
    z:                     10
    width:                 2
    height:                parent.height * (parent.height / parent.contentHeight)
    color:                 parent.indicatorColor
    visible:               showIndicator

    property bool showIndicator: (parent.flickableDirection === Flickable.AutoFlickDirection ||
                                  parent.flickableDirection === Flickable.VerticalFlick ||
                                  parent.flickableDirection === Flickable.HorizontalAndVerticalFlick) &&
                                 (parent.contentHeight > parent.height)

    Component.onCompleted:  { if(animateOpacity) animateOpacity.restart() }
    onVisibleChanged:       { if(animateOpacity) animateOpacity.restart() }
    onHeightChanged:        { if(animateOpacity) animateOpacity.restart() }

    Connections {
        target:                    verticalIndicator.parent
        onMovementStarted:         verticalIndicator.opacity = 1.0
        onMovementEnded:           animateOpacity.restart()
        onContentHeightChanged:    animateOpacity.restart()
    }

    NumberAnimation {
        id:            animateOpacity
        target:        verticalIndicator
        properties:    "opacity"
        from:          1.0
        to:            0.0
        duration:      2000
        easing.type:   Easing.InQuint
    }
}
