import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// Drop-down menu with the position and edit-mode actions for a mission item.
/// Shared by the tree view item editor and the tabbed panel inspector.
Item {
    id: root

    required property var missionItem

    readonly property var _missionController: missionItem.masterController.missionController
    readonly property bool _waypointsOnlyMode: QGroundControl.corePlugin.options.missionWaypointsOnly

    /// Opens the menu at \a position within \a sourceItem
    function openAt(sourceItem, position) {
        // mapToItem against mainWindow does not work here, globals.parent also gets us mainWindow
        const point = sourceItem.mapToItem(globals.parent, position.x, position.y)
        menuComponent.createObject(mainWindow, { clickRect: Qt.rect(point.x, point.y, 0, 0) }).open()
    }

    QGCPopupDialogFactory {
        id: editPositionDialogFactory

        dialogComponent: editPositionDialog
    }

    Component {
        id: editPositionDialog

        EditPositionDialog {
            property bool _editCenterCoordinate: false

            onCoordinateChanged: {
                if (_editCenterCoordinate) {
                    root.missionItem.centerCoordinate = coordinate
                } else {
                    root.missionItem.coordinate = coordinate
                }
            }
        }
    }

    Component {
        id: menuComponent

        DropPanel {
            id: menuDropPanel
            onClosed: destroy()

            sourceComponent: Component {
                ColumnLayout {
                    spacing: ScreenTools.defaultFontPixelHeight / 2

                    QGCButton {
                        objectName: "missionItemMenu_moveToVehicle"
                        Layout.fillWidth: true
                        text: qsTr("Move to vehicle position")
                        enabled: _activeVehicle && root.missionItem.specifiesCoordinate && _activeVehicle.coordinate.isValid

                        onClicked: {
                            root.missionItem.coordinate = _activeVehicle.coordinate
                            menuDropPanel.close()
                        }

                        property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
                    }

                    QGCButton {
                        objectName: "missionItemMenu_moveToPrevious"
                        Layout.fillWidth: true
                        text: qsTr("Move to previous item position")
                        enabled: root._missionController.previousCoordinate.isValid

                        onClicked: {
                            root.missionItem.coordinate = root._missionController.previousCoordinate
                            menuDropPanel.close()
                        }
                    }

                    QGCButton {
                        objectName: "missionItemMenu_editPosition"
                        Layout.fillWidth: true
                        text: qsTr("Edit position...")
                        enabled: root.missionItem.specifiesCoordinate

                        onClicked: {
                            const editCenterCoordinate = root.missionItem.isSurveyItem
                            editPositionDialogFactory.open({
                                _editCenterCoordinate: editCenterCoordinate,
                                coordinate: editCenterCoordinate ? root.missionItem.centerCoordinate : root.missionItem.coordinate,
                                altitudeFact: !editCenterCoordinate && root.missionItem.specifiesAltitude ? root.missionItem.altitude : null,
                                altitudeFrame: !editCenterCoordinate && root.missionItem.specifiesAltitude ? root.missionItem.altitudeFrame : QGroundControl.AltitudeFrameNone,
                            })
                            menuDropPanel.close()
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: qgcPal.borderSubtle
                    }

                    QGCCheckBoxSlider {
                        Layout.fillWidth: true
                        text: qsTr("Show all values")
                        visible: QGroundControl.corePlugin.showAdvancedUI
                        checked: root.missionItem.isSimpleItem ? root.missionItem.rawEdit : false
                        enabled: root.missionItem.isSimpleItem && !root._waypointsOnlyMode

                        onClicked: {
                            root.missionItem.rawEdit = checked
                            if (root.missionItem.rawEdit && !root.missionItem.friendlyEditAllowed) {
                                root.missionItem.rawEdit = false
                                checked = false
                                QGroundControl.showMessageDialog(root, qsTr("Mission Edit"), qsTr("You have made changes to the mission item which cannot be shown in Simple Mode"))
                            }
                            menuDropPanel.close()
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: qgcPal.borderSubtle
                    }

                    QGCLabel {
                        text: qsTr("Item #%1").arg(root.missionItem.sequenceNumber)
                        enabled: false
                    }
                }
            }
        }
    }

    QGCPalette { id: qgcPal }
}
