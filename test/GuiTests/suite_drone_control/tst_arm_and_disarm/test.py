from utils import start_qgc
import toolbar


def main():
    start_qgc()
    test.verify(
        waitFor(lambda: toolbar.is_drone_disarmed(), 10000), "Drone should be Disarmed"
    )
    toolbar.arm_drone()
    test.verify(
        waitFor(lambda: toolbar.is_drone_armed(), 10000), "Drone should be Armed"
    )
    toolbar.disarm_drone()
    test.verify(
        waitFor(lambda: toolbar.is_drone_disarmed(), 10000), "Drone should be Disarmed"
    )
