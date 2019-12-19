from qgroundcontrol import QGroundControlPO
from complex_components import Table
import squish
import test
import os


class AnalyzeMenuPO(QGroundControlPO):
    setupView_AnalyzeView = {
        "container": QGroundControlPO.application_window,
        "id": "setupView",
        "type": "AnalyzeView",
        "unnamed": 1,
        "visible": True,
    }
    buttonScroll_Analyze_QGCFlickable = {
        "container": setupView_AnalyzeView,
        "id": "buttonScroll",
        "type": "QGCFlickable",
        "unnamed": 1,
        "visible": True,
    }
    Log_Download_button = {
        "container": buttonScroll_Analyze_QGCFlickable,
        "text": "Log Download",
        "type": "SubMenuButton",
        "unnamed": 1,
        "visible": True,
    }
    MAVLink_Inspector_button = {
        "container": buttonScroll_Analyze_QGCFlickable,
        "text": "MAVLink Inspector",
        "type": "SubMenuButton",
        "unnamed": 1,
        "visible": True,
    }


class AnalyzeMenu:
    @staticmethod
    def open_log_download():
        test.log('[Analyze] Open "Log Download"')
        squish.mouseClick(AnalyzeMenuPO.Log_Download_button)

    @staticmethod
    def open_mavlink_inspector():
        test.log('[Analyze] Open "MAVLink Inspector"')
        squish.mouseClick(AnalyzeMenuPO.MAVLink_Inspector_button)


class LogDownloadPO(QGroundControlPO):
    log_download_page = {
        "container": QGroundControlPO.application_window,
        "id": "logDownloadPage",
        "type": "LogDownloadPage",
        "unnamed": 1,
        "visible": True,
    }
    Refresh_button = {
        "container": log_download_page,
        "text": "Refresh",
        "type": "QGCButton",
        "unnamed": 1,
        "visible": True,
    }
    Erease_All_button = {
        "container": log_download_page,
        "text": "Erase All",
        "type": "QGCButton",
        "unnamed": 1,
        "visible": True,
    }
    Download_button = {
        "container": log_download_page,
        "text": "Download",
        "type": "QGCButton",
        "unnamed": 1,
        "visible": True,
    }
    delete_all_log_files_loader = {
        "container": QGroundControlPO.o_Overlay,
        "id": "dlgLoader",
        "type": "Loader",
        "unnamed": 1,
        "visible": True,
    }
    delete_files_confirmation_label = {
        "container": delete_all_log_files_loader,
        "id": "titleLabel",
        "text": "Delete All Log Files",
        "type": "Text",
        "unnamed": 1,
        "visible": True,
    }
    Yes_button = {
        "container": delete_all_log_files_loader,
        "id": "_acceptButton",
        "text": "Yes",
        "type": "QGCButton",
        "unnamed": 1,
        "visible": True,
    }
    log_files_table = {
        "container": log_download_page,
        "id": "tableView",
        "type": "TableView",
        "unnamed": 1,
        "visible": True,
    }


class LogDownload:
    @staticmethod
    def refresh_logs():
        test.log("[Analyze] Refresh Vehicle log list")
        squish.mouseClick(squish.waitForObject(LogDownloadPO.Refresh_button))

    @staticmethod
    def erase_all_logs():
        test.log("[Analyze] Erase All log files")
        squish.mouseClick(squish.waitForObject(LogDownloadPO.Erease_All_button))
        squish.waitForObject(LogDownloadPO.delete_files_confirmation_label)
        squish.mouseClick(squish.waitForObject(LogDownloadPO.Yes_button))

    @staticmethod
    def download_log():
        test.log("[Analyze] Download log file")
        squish.mouseClick(squish.waitForObject(LogDownloadPO.Download_button))
        squish.mouseClick(
            squish.waitForImage(
                squish.findFile("scripts", os.path.join("images", "openButton.png"))
            )
        )

    @staticmethod
    def get_number_of_log_files():
        return Table(LogDownloadPO.log_files_table).get_row_count()

    @staticmethod
    def get_cells_from_row(position=0):
        return Table(LogDownloadPO.log_files_table).get_cells_from_row(position)

    @staticmethod
    def select_log(position=0):
        test.log(f'[Analyze] Select log on position "{position}"')
        Table(LogDownloadPO.log_files_table).select_row(position)

    @staticmethod
    def get_log_status(position=0):
        return Table(LogDownloadPO.log_files_table).get_value("Status", position)


def mav_link_button(text):
    return {
        "container": QGroundControlPO.application_window,
        "text": text,
        "type": "MAVLinkMessageButton",
        "unnamed": 1,
        "visible": True,
    }


class MavLinkInspectorPO(QGroundControlPO):
    mavlink_inscpector_page = {
        "container": QGroundControlPO.application_window,
        "type": "MAVLinkInspectorPage",
        "unnamed": 1,
        "visible": True,
    }
    HEARTBEAT_button = mav_link_button("HEARTBEAT")
    SYS_STATUS_button = mav_link_button("SYS_STATUS")
    PING_button = mav_link_button("PING")
    GPS_RAW_INT_button = mav_link_button("GPS_RAW_INT")
    ATTITUDE_button = mav_link_button("ATTITUDE")
    ATTITUDE_QUATERNION_button = mav_link_button("ATTITUDE_QUATERNION")
    LOCAL_POSITION_NED_button = mav_link_button("LOCAL_POSITION_NED")
    GLOBAL_POSITION_INT_button = mav_link_button("GLOBAL_POSITION_INT")
    SERVO_OUTPUT_RAW_button = mav_link_button("SERVO_OUTPUT_RAW")
    VFR_HUD_button = mav_link_button("VFR_HUD")
    POSITION_TARGET_GLOBAL_INT = mav_link_button("POSITION_TARGET_GLOBAL_INT")
    HIGHRES_IMU_button = mav_link_button("HIGHRES_IMU")
    ALTITUDE_button = mav_link_button("ALTITUDE")
    BATTERY_STATUS_button = mav_link_button("BATTERY_STATUS")
    ESTIMATOR_STATUS_button = mav_link_button("ESTIMATOR_STATUS")
    VIBRATION_button = mav_link_button("VIBRATION")
    HOME_POSITION_button = mav_link_button("HOME_POSITION")
    EXTENDED_SYS_STATE_button = mav_link_button("EXTENDED_SYS_STATE")
    ODOMETRY_button = mav_link_button("ODOMETRY")
    UTM_GLOBAL_POSITION_button = mav_link_button("UTM_GLOBAL_POSITION")
    header_row = {
        "container": mavlink_inscpector_page,
        "id": "header",
        "type": "RowLayout",
        "unnamed": 1,
        "visible": True,
    }
    header = {"container": header_row, "type": "Text", "unnamed": 1, "visible": True}
    menu_layout = {
        "container": mavlink_inscpector_page,
        "id": "buttonCol",
        "type": "ColumnLayout",
        "unnamed": 1,
        "visible": True,
    }


class MavLinkInspector:
    @staticmethod
    def get_header():
        return squish.waitForObject(MavLinkInspectorPO.header).text

    @staticmethod
    def get_menu_witdh():
        return squish.waitForObject(MavLinkInspectorPO.menu_layout).width

    @staticmethod
    def get_menu_elem_width(message):
        menu_element = mav_link_button(message)
        return squish.waitForObject(menu_element, 30000).width

    @staticmethod
    def get_menu_elem_height(message):
        menu_element = mav_link_button(message)
        return squish.waitForObject(menu_element, 30000).height

    @staticmethod
    def get_message_buttons():
        return squish.findAllObjects(
            {
                "container": MavLinkInspectorPO.menu_layout,
                "type": "MAVLinkMessageButton",
                "unnamed": 1,
                "visible": True,
            }
        )

    @staticmethod
    def get_message_buttons_texts():
        return [
            str(button.text)
            for button in squish.findAllObjects(
                {
                    "container": MavLinkInspectorPO.menu_layout,
                    "type": "MAVLinkMessageButton",
                    "unnamed": 1,
                    "visible": True,
                }
            )
        ]

    @staticmethod
    def scroll_menu(delta):
        squish.flick(
            squish.waitForObject(
                {
                    "container": MavLinkInspectorPO.mavlink_inscpector_page,
                    "id": "buttonGrid",
                    "type": "QGCFlickable",
                    "unnamed": 1,
                    "visible": True,
                }
            ),
            0,
            delta,
        )
