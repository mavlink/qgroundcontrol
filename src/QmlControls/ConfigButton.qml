import QGroundControl
import QGroundControl.Controls

SettingsButton {
    icon.color: setupComplete ? textColor : "red"

    property bool setupComplete: true
}
