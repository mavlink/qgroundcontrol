.pragma library

// Row definitions for PX4CollisionComponent.qml, in the PX4ConfigRows.js schema

function collisionRows() {
    return [
        {
            id: "prevention", group: qsTr("Collision Prevention"), title: qsTr("Prevention"),
            description: qsTr("Keeps the vehicle away from obstacles seen by distance sensors or a companion computer. Position mode only."),
            activeWhen: [{ param: "CP_DIST", op: "gt", value: 0 }],
            cells: [
                { param: "CP_DIST", label: qsTr("Minimum distance"), note: qsTr("0 or negative disables.") },
                { param: "CP_GO_NO_DATA", label: qsTr("Move without sensor data"),
                  note: qsTr("Allows moving where no sensor is looking.") }
            ]
        },
        {
            id: "tuning", group: qsTr("Collision Prevention"), title: qsTr("Tuning"),
            description: qsTr("The defaults suit most vehicles."),
            activeWhen: [{ param: "CP_DIST", op: "gt", value: 0 }],
            cells: [
                { param: "CP_DELAY", label: qsTr("Sensor and control delay") },
                { param: "CP_GUIDE_ANG", label: qsTr("Guidance angle"),
                  note: qsTr("How far the vehicle may steer left or right of the commanded direction.") }
            ]
        }
    ]
}
