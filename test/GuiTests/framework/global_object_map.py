from objectmaphelper import *

o_QQuickApplicationWindow = {
    "type": "QQuickApplicationWindow",
    "unnamed": 1,
    "visible": True,
}
sliderDragArea_QGCMouseArea = {
    "container": o_QQuickApplicationWindow,
    "id": "sliderDragArea",
    "type": "QGCMouseArea",
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
vehicle_1_Text = {
    "container": o_ToolBar,
    "text": "Vehicle 1",
    "type": "Text",
    "unnamed": 1,
    "visible": True,
}

o_CustomArmedIndicator = {
    "container": o_ToolBar,
    "type": "CustomArmedIndicator",
    "unnamed": 1,
    "visible": True,
}
armedIndicatorRow = {
    "container": o_CustomArmedIndicator,
    "id": "labelRow",
    "type": "Row",
    "unnamed": 1,
    "visible": True,
}
armedIndicatorLabel = {
    "container": armedIndicatorRow,
    "type": "Text",
    "unnamed": 1,
    "visible": True,
}
armedIndicatorLed = {
    "container": armedIndicatorRow,
    "type": "Rectangle",
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
altitudeSlider_GuidedAltitudeSlider = {
    "container": o_QQuickApplicationWindow,
    "id": "altitudeSlider",
    "type": "GuidedAltitudeSlider",
    "unnamed": 1,
    "visible": True,
}
header_GuidedAltitudeSlider_Column = {
    "container": altitudeSlider_GuidedAltitudeSlider,
    "id": "headerColumn",
    "type": "Column",
    "unnamed": 1,
    "visible": True,
}
sliderAltField = {
    "container": header_GuidedAltitudeSlider_Column,
    "id": "altField",
    "type": "Text",
    "unnamed": 1,
    "visible": True,
}
altSlider_QGCSlider = {
    "container": altitudeSlider_GuidedAltitudeSlider,
    "id": "altSlider",
    "type": "QGCSlider",
    "unnamed": 1,
    "visible": True,
}
fakeHandle_Item = {
    "container": altSlider_QGCSlider,
    "id": "fakeHandle",
    "type": "Item",
    "unnamed": 1,
    "visible": True,
}
takeoff_QGCHoverButton = {
    "checkable": False,
    "container": o_QQuickApplicationWindow,
    "id": "buttonTemplate",
    "text": "Takeoff",
    "type": "QGCHoverButton",
    "unnamed": 1,
    "visible": True,
}
rtl_QGCHoverButton = {
    "checkable": False,
    "container": o_QQuickApplicationWindow,
    "id": "buttonTemplate",
    "text": "RTL",
    "type": "QGCHoverButton",
    "unnamed": 1,
    "visible": True,
}
vehicleStatusGrid_GridLayout = {
    "container": o_QQuickApplicationWindow,
    "id": "vehicleStatusGrid",
    "type": "GridLayout",
    "unnamed": 1,
    "visible": True,
}
currentDroneAltitude = {
    "container": vehicleStatusGrid_GridLayout,
    "occurrence": 7,
    "type": "Text",
    "unnamed": 1,
    "visible": True,
}
currentDroneDistance = {
    "container": vehicleStatusGrid_GridLayout,
    "occurrence": 6,
    "type": "Text",
    "unnamed": 1,
    "visible": True,
}
currentDroneSpeed = {
    "container": vehicleStatusGrid_GridLayout,
    "occurrence": 2,
    "type": "Text",
    "unnamed": 1,
    "visible": True,
}
o_fMap_FlightDisplayViewMap = {
    "container": o_QQuickApplicationWindow,
    "id": "_fMap",
    "type": "FlightDisplayViewMap",
    "unnamed": 1,
    "visible": True,
}
o_QQuickMenuPopupWindow1 = {
    "type": "QQuickMenuPopupWindow1",
    "unnamed": 1,
    "visible": True,
}
scrollView_ScrollView = {
    "container": o_QQuickMenuPopupWindow1,
    "id": "scrollView",
    "type": "ScrollView",
    "unnamed": 1,
    "visible": True,
}
scrollView_Go_to_location_Text = {
    "container": scrollView_ScrollView,
    "text": "Go to location",
    "type": "Text",
    "unnamed": 1,
    "visible": True,
}
o_VehicleMapItem = {
    "container": o_QQuickApplicationWindow,
    "type": "VehicleMapItem",
    "unnamed": 1,
    "visible": True,
}
