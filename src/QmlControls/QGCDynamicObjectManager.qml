import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.4

import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

/// Provides a standard set of tools for dynamically create/adding/removing Qml objects
Item {
    visible: false

    property var rgDynamicObjects: [ ]

    function createObject(sourceComponent, parentObject, parentObjectIsMap) {
        var obj = sourceComponent.createObject(parentObject)
        if (obj.status === Component.Error) {
            console.log(obj.errorString())
        }
        rgDynamicObjects.push(obj)
        if (arguments.length < 3) {
            parentObjectIsMap = false
        }
        if (parentObjectIsMap) {
            map.addMapItem(obj)
        }
        return obj
    }

    function createObjects(rgSourceComponents, parentObject, parentObjectIsMap) {
        if (arguments.length < 3) {
            parentObjectIsMap = false
        }
        for (var i=0; i<rgSourceComponents.length; i++) {
            createObject(rgSourceComponents[i], parentObject, parentObjectIsMap)
        }
    }

    function destroyObjects() {
        for (var i=0; i<rgDynamicObjects.length; i++) {
            rgDynamicObjects[i].destroy()
        }
        rgDynamicObjects = [ ]
    }
}
