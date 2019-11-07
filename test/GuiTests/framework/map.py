from qgroundcontrol import QGroundControlPO


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
