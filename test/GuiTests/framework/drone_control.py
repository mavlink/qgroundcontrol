import global_object_map as names
import squish
import test
import builtins
import drone_params
from utils import confirm_with_slider


def trigger_takeoff():
    test.log("[Drone Control] Trigger Takeoff")
    squish.mouseClick(squish.waitForObject(names.takeoff_QGCHoverButton))


def takeoff(altitude):
    test.startSection("[Drone Control] Takeoff")
    trigger_takeoff()
    set_altitude(altitude)
    confirm_with_slider()
    test.endSection()


def trigger_rtl():
    test.log("[Drone Control] Trigger RTL")
    squish.mouseClick(squish.waitForObject(names.rtl_QGCHoverButton))


def rtl():
    test.startSection("[Drone Control] RTL")
    trigger_rtl()
    confirm_with_slider()
    test.endSection()


def set_altitude(altitude, epsilon=0.3):
    test.log(
        f'[Drone Control] Set target altitude to "{altitude}m" with epsilon "{epsilon}"'
    )
    initial_altitude = float(str(squish.waitForObject(names.sliderAltField).text)[:-1])
    handler = squish.waitForObject(names.fakeHandle_Item)
    center_h = builtins.int(handler.height / 2)
    x = builtins.int(handler.width / 2)
    y = center_h - 1 if altitude > initial_altitude else center_h + 1
    current_alt_label = squish.waitForObject(names.sliderAltField)
    try:
        squish.mousePress(handler)
        while abs(float(str(current_alt_label.text)[:-1]) - altitude) > epsilon:
            squish.mouseMove(handler, x, y)
    except:
        raise
    finally:
        squish.mouseRelease()


def send_to_location(dx, dy):
    test.startSection("[Drone Control] Send drone to a provided location")
    test.log(
        f"Select location {dx}px horizontally and {dy}px vertically from the drone"
    )
    drone = squish.waitForObject(names.o_VehicleMapItem)
    x0, y0 = drone_params.get_position()
    # x0 = drone.x + builtins.int(drone.width / 2)
    # y0 = drone.y + builtins.int(drone.height / 2)
    squish.mouseClick(
        squish.waitForObject(names.o_fMap_FlightDisplayViewMap),
        x0 + dx,
        y0 + dy,
        squish.Qt.NoModifier,
        squish.Qt.LeftButton,
    )
    squish.mouseClick(squish.waitForObject(names.scrollView_Go_to_location_Text))
    confirm_with_slider()
    test.endSection()
