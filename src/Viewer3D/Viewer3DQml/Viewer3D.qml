import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.Controllers
import QGroundControl.FactSystem
import QGroundControl.FlightDisplay
import QGroundControl.FlightMap
import QGroundControl.Palette
import QGroundControl.ScreenTools
import QGroundControl.Vehicle

import QGroundControl.Viewer3D
import Viewer3D.Models3D


///     @author Omid Esrafilian <esrafilian.omid@gmail.com>

Item{
    id: viewer3DBody

    property bool isOpen: false
    property bool   _viewer3DEnabled:        QGroundControl.settingsManager.viewer3DSettings.enabled.rawValue
    property bool _loaderSourceSet: false

    function open(){
        if(_viewer3DEnabled === true){
            view3DManagerLoader.active = true;
            isOpen = true;
        }
    }

    function close(){
        isOpen = false;
    }

    visible: isOpen
    enabled: isOpen

    on_Viewer3DEnabledChanged: {
        if(_viewer3DEnabled === false){
            viewer3DBody.close();
            view3DManagerLoader.active = false;
        }
    }

    Component{
        id: viewer3DManagerComponent

        Viewer3DManager{
            id: _viewer3DManager
        }
    }

    Loader{
        id: view3DManagerLoader
        active: false
        sourceComponent: viewer3DManagerComponent

        onLoaded: {
            if(view3DLoader.active === true){
                view3DLoader.item.viewer3DManager = view3DManagerLoader.item
            }else{
                view3DLoader.active = true
            }
        }
    }

    Loader{
        id: view3DLoader
        anchors.fill: parent
        active: false
        source: "Models3D/Viewer3DModel.qml"

        onLoaded: {
            item.viewer3DManager = view3DManagerLoader.item
        }
    }

    Binding{
        target: view3DLoader.item
        property: "isViewer3DOpen"
        value: isOpen
        when: view3DLoader.status == Loader.Ready
    }
}
