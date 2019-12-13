import application
from qgroundcontrol import QGroundControl
import map


def main():
    application.start()
    QGroundControl.set_window_size(950, 550)

    test.imagePresent("map_zoom_1000_miles")
    test.compare(map.zoom_level(), "1000 miles", 'Zoom Level should be "1000 miles"')
    map.zoom_in()
    map.zoom_in()

    test.imagePresent("map_zoom_1000_miles_2x")
    test.compare(map.zoom_level(), "1000 miles", 'Zoom Level should be "1000 miles"')
    map.zoom_in()
    map.zoom_in()

    test.imagePresent("map_zoom_500_miles")
    test.compare(map.zoom_level(), "500 miles", 'Zoom Level should be "500 miles"')
    map.zoom_in()
    map.zoom_in()

    test.imagePresent("map_zoom_250_miles")
    test.compare(map.zoom_level(), "250 miles", 'Zoom Level should be "250 miles"')
    map.zoom_in()
    map.zoom_in()

    test.imagePresent("map_zoom_100_miles")
    test.compare(map.zoom_level(), "100 miles", 'Zoom Level should be "100 miles"')
