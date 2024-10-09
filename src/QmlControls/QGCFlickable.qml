import QtQuick

import QGroundControl.Palette
import QGroundControl.ScreenTools

/// QGC version of Flickable control that shows horizontal/vertial scroll indicators
Flickable {
    id:                     root
    boundsBehavior:         Flickable.StopAtBounds
    clip:                   true
    maximumFlickVelocity:   (ScreenTools.realPixelDensity * 25.4) * 8   // About two inches per second

    property color indicatorColor: qgcPal.text

    Component.onCompleted: {
        var indicatorComponent = Qt.createComponent("QGCFlickableVerticalIndicator.qml")
        indicatorComponent.createObject(root)
        indicatorComponent = Qt.createComponent("QGCFlickableHorizontalIndicator.qml")
        indicatorComponent.createObject(root)
    }

    // Make so the flickable repositions the content if the user is navigating using the TAB key 
    // so that the active item is centered in the screen. This is especially helpful when 
    // the item being tabbed to is offscreen.
    property var activeFocusItemCopy: activeFocusItem
    onActiveFocusItemCopyChanged: centerActiveFocusItemInFlickable()
    function centerActiveFocusItemInFlickable(){
        if(activeFocusItem !== 'undefined' && isDescendant(activeFocusItem, root)){
            let posThatWouldCenterItemInFlickable = activeFocusItem.mapToItem(root.contentItem, 0, 0).y
                - root.height/2 + activeFocusItem.height/2
            if(posThatWouldCenterItemInFlickable < 0 || root.contentHeight < root.height){
                root.contentY = 0
            }
            else if(posThatWouldCenterItemInFlickable > root.contentHeight - root.height){ 
                root.contentY = root.contentHeight - root.height
            }
            else {
                root.contentY = posThatWouldCenterItemInFlickable
            }
        }
    }
    function isDescendant(item, potentialAncestor) {
        var currentItem = item
        while (currentItem) {
            if (currentItem === potentialAncestor) {
                return true
            }
            currentItem = currentItem.parent
        }
        return false
    }
}
