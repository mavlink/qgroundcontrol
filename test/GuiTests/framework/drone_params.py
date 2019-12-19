from qgroundcontrol import QGroundControlPO
from map import MapPO
import squish
import builtins


class DroneParamsPO(QGroundControlPO):
    vehicleStatusGrid_GridLayout = {
        "container": QGroundControlPO.application_window,
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


def get_altitude():
    return float(
        str(squish.waitForObject(DroneParamsPO.currentDroneAltitude).text).split()[0]
    )


def get_distance():
    return builtins.int(
        str(squish.waitForObject(DroneParamsPO.currentDroneDistance).text).split()[0]
    )


def get_speed():
    return float(
        str(squish.waitForObject(DroneParamsPO.currentDroneSpeed).text).split()[0]
    )


def get_position():
    drone = squish.waitForObject(MapPO.o_VehicleMapItem)
    return (builtins.int(drone.x), builtins.int(drone.y))
