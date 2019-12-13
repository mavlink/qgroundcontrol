import application
from qgroundcontrol import QGroundControl
import map


def main():
    application.start()
    QGroundControl.set_window_size(950, 550)
    map.zoom_in(times=8)

    test.imagePresent("map_zoom_100_miles")
    test.compare(map.zoom_level(), "100 miles", 'Zoom Level should be "100 miles"')
    map.zoom_out()
    map.zoom_out()

    test.imagePresent("map_zoom_250_miles")
    test.compare(map.zoom_level(), "250 miles", 'Zoom Level should be "250 miles"')
    map.zoom_out()
    map.zoom_out()

    test.imagePresent("map_zoom_500_miles")
    test.compare(map.zoom_level(), "500 miles", 'Zoom Level should be "500 miles"')
    map.zoom_out()
    map.zoom_out()

    test.imagePresent("map_zoom_1000_miles_2x")
    test.compare(map.zoom_level(), "1000 miles", 'Zoom Level should be "500 miles"')
    map.zoom_out()
    map.zoom_out()

    test.compare(map.zoom_level(), "1000 miles", 'Zoom Level should be "1000 miles"')
    test.imagePresent("map_zoom_1000_miles")
