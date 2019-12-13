import application
from qgroundcontrol import QGroundControl


def main():
    application.start()
    QGroundControl.set_window_size(950, 550)
    test.vp("initial_map")
