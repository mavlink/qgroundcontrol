import squish
import test


class QGroundControlPO:
    application_window = {
        "type": "QQuickApplicationWindow",
        "unnamed": 1,
        "visible": True,
    }
    o_Overlay = {
        "container": application_window,
        "type": "Overlay",
        "unnamed": 1,
        "visible": True,
    }


class QGroundControl:
    @staticmethod
    def set_window_size(width, height):
        test.log(f"Change AUT window size to {width}x{height}")
        squish.setWindowSize(
            squish.waitForObject(QGroundControlPO.application_window), width, height
        )
