from utils import start_qgc
import drone_bar


def main():
    start_qgc()
    test.verify(
        waitFor(lambda: drone_bar.is_drone_disarmed(), 10000),
        "Drone should be Disarmed",
    )
    drone_bar.arm_drone()
    test.verify(
        waitFor(lambda: drone_bar.is_drone_armed(), 10000), "Drone should be Armed"
    )
    drone_bar.disarm_drone()
    test.verify(
        waitFor(lambda: drone_bar.is_drone_disarmed(), 10000),
        "Drone should be Disarmed",
    )
