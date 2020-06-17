import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.4

import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

/// Provides a standard set of tools for dynamically create/adding/removing Qml objects
Item {
    visible: false

    property var    rgDynamicObjects:   [ ]
    property bool   empty:              rgDynamicObjects.length === 0

    Component.onDestruction: destroyObjects()

    function createObject(sourceComponent, parentObject, addMapItem) {
        var obj = sourceComponent.createObject(parentObject)
        if (obj.status === Component.Error) {
            console.log(obj.errorString())
        }
        rgDynamicObjects.push(obj)
        if (arguments.length < 3) {
            addMapItem = false
        }
        if (addMapItem) {
            parentObject.addMapItem(obj)
        }
        return obj
    }

    function createObjects(rgSourceComponents, parentObject, addMapItem) {
        if (arguments.length < 3) {
            addMapItem = false
        }
        for (var i=0; i<rgSourceComponents.length; i++) {
            createObject(rgSourceComponents[i], parentObject, addMapItem)
        }
    }

    /// Adds the object to the list. If mapControl is specified it will aso be added to the map.
    function addObject(object, mapControl) {
        rgDynamicObjects.push(object)
        if (arguments.length == 2) {
            mapControl.addMapItem(object)
        }
        return object
    }

    function destroyObjects() {
        for (var i=0; i<rgDynamicObjects.length; i++) {
            rgDynamicObjects[i].destroy()
        }
        rgDynamicObjects = [ ]
    }
}
