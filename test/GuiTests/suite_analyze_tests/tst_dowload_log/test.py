from utils import delete_logs_from_dir, file_exists
import application
import toolbar
import main_menu
import analyze
import drone_control


def main():
    delete_logs_from_dir("$HOME/Documents/CustomQGC/Logs/")
    application.start()
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
    analyze.LogDownload.select_log()
    analyze.LogDownload.download_log()
    test.verify(
        waitFor(lambda: analyze.LogDownload.get_log_status() == "Downloaded", 60000),
        'The log status should be changed to "Success" within 60s',
    )
    test.verify(
        file_exists("$HOME/Documents/CustomQGC/Logs/log_0_*.ulg"),
        "There should be $HOME/Documents/CustomQGC/Logs/log_0_*.ulg log file",
    )
    analyze.LogDownload.erase_all_logs()
