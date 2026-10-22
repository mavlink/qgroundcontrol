import QtQuick

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView

/// Checks which can be completed before a vehicle is connected, independent of the vehicle type. QGC ships none:
/// custom builds override this file to provide them. Designed for a single connected vehicle: the after-flight reset
/// in MainStatusIndicator only follows the active vehicle.
Item {
    property var model: listModel

    PreFlightCheckModel {
        id: listModel
    }
}
