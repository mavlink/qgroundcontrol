from utils import is_sufficiently_sized
from qgroundcontrol import QGroundControl
import application
import toolbar
import main_menu
import analyze
import drone_control


def main():
    application.start()
    QGroundControl.set_window_size(950, 550)
    toolbar.open_main_menu()
    main_menu.open_analyze()
    test.compare(
        analyze.LogDownload.get_number_of_log_files(),
        0,
        "The Log Files list should be empty",
    )
    toolbar.open_main_menu()
    main_menu.open_fly()
    drone_control.takeoff()
    drone_control.land()
    toolbar.open_main_menu()
    main_menu.open_analyze()
    analyze.LogDownload.refresh_logs()
    test.compare(
        analyze.LogDownload.get_number_of_log_files(),
        1,
        "One Log file should be listed",
    )
    cells = analyze.LogDownload.get_cells_from_row()
    for cell in cells:
        test.verify(
            is_sufficiently_sized(cell),
            f'Size of the displayed "{cell.text}" text should not be larger than the cell size',
        )
    analyze.LogDownload.erase_all_logs()
