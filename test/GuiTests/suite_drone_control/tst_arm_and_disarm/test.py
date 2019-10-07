from utils import start_qgc
import drone_bar


def main():
    start_qgc()
    test.compare(drone_bar.is_drone_armed(), False, "Drone should be Disarmed")
    drone_bar.arm_drone()
    test.compare(drone_bar.is_drone_armed(), True, "Drone should be Armed")
    snooze(30)
    test.compare(drone_bar.is_drone_armed(), True, "Drone should be Armed after 30s")
    drone_bar.disarm_drone()
    test.compare(drone_bar.is_drone_armed(), False, "Drone should be Disarmed")
