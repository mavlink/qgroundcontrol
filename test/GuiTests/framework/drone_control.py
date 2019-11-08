from qgroundcontrol import QGroundControlPO
import squish
import test
import builtins
from complex_components import Slider
from map import MapPO
import drone_params


class DroneControlPO(QGroundControlPO):
    Takeoff_button = {
        "checkable": False,
        "container": QGroundControlPO.application_window,
        "id": "buttonTemplate",
        "text": "Takeoff",
        "type": "QGCHoverButton",
        "unnamed": 1,
        "visible": True,
    }
    Land_button = {
        "checkable": False,
        "container": QGroundControlPO.application_window,
        "id": "buttonTemplate",
        "text": "Land",
        "type": "QGCHoverButton",
        "unnamed": 1,
        "visible": True,
    }
    RTL_Button = {
        "checkable": False,
        "container": QGroundControlPO.application_window,
        "id": "buttonTemplate",
        "text": "RTL",
        "type": "QGCHoverButton",
        "unnamed": 1,
        "visible": True,
    }


class AltitudeSliderPO(QGroundControlPO):
    altitudeSlider_GuidedAltitudeSlider = {
        "container": QGroundControlPO.application_window,
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


def trigger_takeoff():
    test.log("[Drone Control] Trigger Takeoff")
    squish.mouseClick(squish.waitForObject(DroneControlPO.Takeoff_button))


def trigger_landing():
    test.log("[Drone Control] Trigger landing")
    squish.mouseClick(squish.waitForObject(DroneControlPO.Land_button, 30000))


def takeoff(altitude=None):
    test.startSection("[Drone Control] Takeoff")
    trigger_takeoff()
    if altitude:
        set_altitude(altitude)
    Slider.confirm()
    test.endSection()


def land():
    test.startSection("[Drone Control] Land")
    trigger_landing()
    Slider.confirm()
    test.endSection()


def trigger_rtl():
    test.log("[Drone Control] Trigger RTL")
    squish.mouseClick(squish.waitForObject(DroneControlPO.RTL_Button))


def rtl():
    test.startSection("[Drone Control] RTL")
    trigger_rtl()
    Slider.confirm()
    test.endSection()


def set_altitude(altitude, epsilon=0.3):
    test.log(
        f'[Drone Control] Set target altitude to "{altitude}m" with epsilon "{epsilon}"'
    )
    initial_altitude = float(
        str(squish.waitForObject(AltitudeSliderPO.sliderAltField).text)[:-1]
    )
    handler = squish.waitForObject(AltitudeSliderPO.fakeHandle_Item)
    center_h = builtins.int(handler.height / 2)
    x = builtins.int(handler.width / 2)
    y = center_h - 1 if altitude > initial_altitude else center_h + 1
    current_alt_label = squish.waitForObject(AltitudeSliderPO.sliderAltField)
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
    squish.waitForObject(MapPO.o_VehicleMapItem)
    x0, y0 = drone_params.get_position()
    squish.mouseClick(
        squish.waitForObject(MapPO.o_fMap_FlightDisplayViewMap),
        x0 + dx,
        y0 + dy,
        squish.Qt.NoModifier,
        squish.Qt.LeftButton,
    )
    squish.mouseClick(squish.waitForObject(MapPO.scrollView_Go_to_location_Text))
    Slider.confirm()
    test.endSection()
