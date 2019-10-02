from objectmaphelper import *

o_QQuickApplicationWindow = {
    "type": "QQuickApplicationWindow",
    "unnamed": 1,
    "visible": True,
}
o_ToolBar = {
    "container": o_QQuickApplicationWindow,
    "type": "ToolBar",
    "unnamed": 1,
    "visible": True,
}
o_icon_Image = {
    "container": o_ToolBar,
    "id": "_icon",
    "source": "qrc:/res/QGCLogoWhite",
    "type": "Image",
    "unnamed": 1,
    "visible": True,
}
o_Overlay = {
    "container": o_QQuickApplicationWindow,
    "type": "Overlay",
    "unnamed": 1,
    "visible": True,
}
settings_Button = {
    "checkable": False,
    "container": o_Overlay,
    "id": "settingsButton",
    "text": "Settings",
    "type": "Button",
    "unnamed": 1,
    "visible": True,
}
o_FactComboBox = {
    "container": o_QQuickApplicationWindow,
    "type": "FactComboBox",
    "unnamed": 1,
    "visible": True,
}
feet_Text = {
    "container": o_QQuickApplicationWindow,
    "text": "Feet",
    "type": "Text",
    "unnamed": 1,
    "visible": True,
}
fly_Button = {
    "checkable": True,
    "container": o_Overlay,
    "id": "flyButton",
    "text": "Fly",
    "type": "Button",
    "unnamed": 1,
    "visible": True,
}
application_Settings_Text = {
    "container": o_QQuickApplicationWindow,
    "text": "Application Settings",
    "type": "Text",
    "unnamed": 1,
    "visible": True,
}
systemMessageText = {
    "container": o_Overlay,
    "id": "systemMessageText",
    "type": "TextEdit",
    "unnamed": 1,
    "visible": True,
}
offline_Maps_QGCButton = {
    "container": o_QQuickApplicationWindow,
    "text": "Offline Maps",
    "type": "QGCButton",
    "unnamed": 1,
    "visible": True,
}
export_QGCButton = {
    "container": o_QQuickApplicationWindow,
    "text": "Export",
    "type": "QGCButton",
    "unnamed": 1,
    "visible": True,
}
o_exporTiles_QGCFlickable = {
    "container": o_QQuickApplicationWindow,
    "id": "_exporTiles",
    "type": "QGCFlickable",
    "unnamed": 1,
    "visible": True,
}
o_exporTiles_Default_Tile_Set_QGCCheckBox = {
    "container": o_exporTiles_QGCFlickable,
    "text": "Default Tile Set",
    "type": "QGCCheckBox",
    "unnamed": 1,
    "visible": True,
}
close_QGCButton = {
    "container": o_Overlay,
    "id": "exportCloseButton",
    "text": "Close",
    "type": "QGCButton",
    "unnamed": 1,
    "visible": True,
}
