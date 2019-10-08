from utils import start_qgc
import drone_control
import drone_params
import drone_bar
import utils


def main():
    epsilon = 0.3
    altitude = 10.0
    climb_timeout = 25
    rtl_altitude = 30.0
    rtl_timeout = 60
    rtl_epsilon = 1
    return_timeout = 90

    start_qgc()
    initial_position = drone_params.get_position()
    drone_control.takeoff(altitude)

    test.verify(
        waitFor(
            lambda: abs(drone_params.get_altitude() - altitude) <= epsilon,
            climb_timeout * 1000,
        ),
        f'Within {climb_timeout}s Drone should reach "{altitude}"m. Epsilon "{epsilon}"',
    )
    test.verify(
        drone_params.get_distance() <= 1, "Initial Drone distance should be <= 1"
    )
    test.verify(drone_params.get_speed() <= 1.0, "Initial Drone speed should be <= 0.1")
    drone_control.send_to_location(250, 50)
    test.verify(
        waitFor(lambda: drone_params.get_distance() > 10, 10000),
        f"Within 10s Drone distance should exceed 10m",
    )
    test.verify(
        waitFor(lambda: drone_params.get_speed() > 2.0, 10000),
        f"Within 10s Drone speed should exceed 2m/s",
    )
    drone_control.rtl()
    test.verify(
        waitFor(
            lambda: abs(drone_params.get_altitude() - rtl_altitude) <= rtl_epsilon,
            rtl_timeout * 1000,
        ),
        f'Within {rtl_timeout}s Drone should reach "{rtl_altitude}"m. Epsilon "{epsilon}"',
    )
    test.verify(
        waitFor(lambda: drone_bar.is_drone_armed() == False, return_timeout * 1000),
        f'After RTL Drone should land within {return_timeout}s. Epsilon "{epsilon}"',
    )
    test.verify(
        drone_params.get_altitude() <= epsilon, f"Drone altitude should be <= {epsilon}"
    )
    final_position = drone_params.get_position()
    test.verify(
        utils.compare_position(initial_position, final_position),
        f'"The initial({initial_position}) and final({final_position}) Drone position should be the same. Epsilon "{initial_position}"',
    )
