import global_object_map as names
import squish
import builtins


def get_altitude():
    return float(str(squish.waitForObject(names.currentDroneAltitude).text)[:-1])


def get_distance():
    return builtins.int(str(squish.waitForObject(names.currentDroneDistance).text)[:-1])


def get_speed():
    return float(str(squish.waitForObject(names.currentDroneSpeed).text)[:-3])


def get_position():
    drone = squish.waitForObject(names.o_VehicleMapItem)
    return (builtins.int(drone.x), builtins.int(drone.y))


#     x = drone.x + builtins.int(drone.width / 2)
#     y = drone.y + builtins.int(drone.height / 2)
#     return (x, y)
