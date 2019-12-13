from qgroundcontrol import QGroundControlPO
import squish
import test


class MapPO(QGroundControlPO):
    o_VehicleMapItem = {
        "container": QGroundControlPO.application_window,
        "type": "VehicleMapItem",
        "unnamed": 1,
        "visible": True,
    }

    o_fMap_FlightDisplayViewMap = {
        "container": QGroundControlPO.application_window,
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
    zoomUp_Button = {
        "container": QGroundControlPO.application_window,
        "id": "zoomUpButton",
        "type": "QGCButton",
        "unnamed": 1,
        "visible": True,
    }
    zoomDown_Button = {
        "container": QGroundControlPO.application_window,
        "id": "zoomDownButton",
        "type": "QGCButton",
        "unnamed": 1,
        "visible": True,
    }
    zoomValue_Label = {
        "container": QGroundControlPO.application_window,
        "id": "scaleText",
        "type": "QGCMapLabel",
        "unnamed": 1,
        "visible": True,
    }


def zoom_in(times=1):
    for _ in range(times):
        test.log("[Map] Zoom In")
        squish.mouseClick(squish.waitForObject(MapPO.zoomUp_Button))


def zoom_out(times=1):
    for _ in range(times):
        test.log("[Map] Zoom Out")
        squish.mouseClick(squish.waitForObject(MapPO.zoomDown_Button))


def zoom_level():
    return str(squish.waitForObject(MapPO.zoomValue_Label).text)
