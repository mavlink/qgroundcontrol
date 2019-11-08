import application
import toolbar


def main():
    application.start()
    test.verify(
        waitFor(
            lambda: toolbar.get_drone_status() == toolbar.DroneStatus.DISARMED, 10000
        ),
        f"Drone should be Disarmed ({toolbar.DroneStatus.DISARMED})",
    )
    toolbar.arm_drone()
    test.verify(
        waitFor(lambda: toolbar.get_drone_status() == toolbar.DroneStatus.ARMED, 10000),
        f"Drone should be Armed ({toolbar.DroneStatus.ARMED})",
    )
    toolbar.disarm_drone()
    test.verify(
        waitFor(
            lambda: toolbar.get_drone_status() == toolbar.DroneStatus.DISARMED, 10000
        ),
        f"Drone should be Disarmed ({toolbar.DroneStatus.DISARMED})",
    )
