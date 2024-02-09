import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl.Palette
import QGroundControl.ScreenTools
import QGroundControl.Controls

// 3D Viewer modules
import QGroundControl.Viewer3D
import Viewer3D.Models3D


Item{
    id: viewer3DBody
    property bool viewer3DOpen: false
    property bool settingMenuOpen: false

    Component{
        id: viewer3DManagerComponent

        Viewer3DManager{
            id: _viewer3DManager
        }
    }

    Loader{
        id: view3DManagerLoader

        onLoaded: {
            view3DLoader.source = "Models3D/Viewer3DModel.qml"
        }
    }

    Loader{
        id: view3DLoader
        anchors.fill: parent

        onLoaded: {
            item.viewer3DManager = view3DManagerLoader.item
        }
    }

    Binding{
        target: view3DLoader.item
        property: "viewer3DOpen"
        value: viewer3DOpen
        when: view3DLoader.status == Loader.Ready
    }

    onViewer3DOpenChanged: {
        view3DManagerLoader.sourceComponent = viewer3DManagerComponent
        if(viewer3DOpen){
            viewer3DBody.z = 1
        }else{
            viewer3DBody.z = 0
        }
    }

    onSettingMenuOpenChanged:{
        if(settingMenuOpen === true){
            settingMenuComponent.createObject(mainWindow).open()
        }
    }

    Component {
        id: settingMenuComponent

        QGCPopupDialog{
            id: settingMenuDialog
            title:      qsTr("3D view settings")
            buttons:    Dialog.Ok | Dialog.Cancel

            Viewer3DSettingMenu{
                id:                     viewer3DSettingMenu
                viewer3DManager:        view3DManagerLoader.item
                visible:                true
            }

            onRejected: {
                settingMenuOpen = false
                viewer3DSettingMenu.menuClosed(false)
                settingMenuDialog.close()
            }

            onAccepted: {
                settingMenuOpen = false
                viewer3DSettingMenu.menuClosed(true)
                settingMenuDialog.close()
            }
        }
    }
}
