/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Mission Planning Action Toolbar
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QGroundControl
import QGroundControl.Controls
import VoladorTheme 1.0

Rectangle {
    id: toolbarRoot

    property var planMasterController: null
    property var missionController: planMasterController ? planMasterController.missionController : null
    property var planView: null

    implicitHeight: 44
    implicitWidth: toolbarLayout.implicitWidth + 24
    radius: 4
    color: "#E6151C24" // 90% dark glass
    border.color: "#2C3847"
    border.width: 1

    function getCenterCoordinate() {
        if (planView && typeof planView.mapCenter === "function") {
            return planView.mapCenter()
        }
        if (typeof QGroundControl !== "undefined") {
            return QGroundControl.flightMapPosition
        }
        return QtPositioning.coordinate(0, 0)
    }

    RowLayout {
        id: toolbarLayout
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        spacing: 4

        // 1. ADD WAYPOINT
        Rectangle {
            implicitWidth: wpText.implicitWidth + 16
            implicitHeight: 30
            radius: 3
            color: wpMouse.containsMouse ? "#2C3847" : "#1D2733"
            border.color: wpMouse.containsMouse ? "#FF6A00" : "#2C3847"
            border.width: 1

            RowLayout {
                anchors.centerIn: parent
                spacing: 4
                Text { text: "+"; font.family: "JetBrains Mono"; font.pixelSize: 12; font.weight: Font.Bold; color: "#FF6A00" }
                Text { id: wpText; text: "WAYPOINT"; font.family: "Inter"; font.pixelSize: 9; font.weight: Font.Bold; color: "#F5F7FA" }
            }

            MouseArea {
                id: wpMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (!missionController) return
                    var nextIndex = missionController.currentPlanViewVIIndex + 1
                    missionController.insertSimpleMissionItem(toolbarRoot.getCenterCoordinate(), nextIndex, true)
                }
            }

            ToolTip.visible: wpMouse.containsMouse
            ToolTip.text: "Insert simple waypoint at map center"
            ToolTip.delay: 400
        }

        // 2. TAKEOFF
        Rectangle {
            implicitWidth: toText.implicitWidth + 14
            implicitHeight: 30
            radius: 3
            color: toMouse.containsMouse ? "#2C3847" : "#1D2733"
            border.color: toMouse.containsMouse ? "#00C853" : "#2C3847"
            border.width: 1
            opacity: (missionController && missionController.isInsertTakeoffValid) ? 1.0 : 0.4

            RowLayout {
                anchors.centerIn: parent
                spacing: 4
                Text { text: "▲"; font.pixelSize: 9; color: "#00C853" }
                Text { id: toText; text: "TAKEOFF"; font.family: "Inter"; font.pixelSize: 9; font.weight: Font.Bold; color: "#F5F7FA" }
            }

            MouseArea {
                id: toMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (!missionController || !missionController.isInsertTakeoffValid) return
                    var nextIndex = missionController.currentPlanViewVIIndex + 1
                    missionController.insertTakeoffItem(toolbarRoot.getCenterCoordinate(), nextIndex, true)
                }
            }

            ToolTip.visible: toMouse.containsMouse
            ToolTip.text: "Insert takeoff sequence"
            ToolTip.delay: 400
        }

        // 3. LAND
        Rectangle {
            implicitWidth: landText.implicitWidth + 14
            implicitHeight: 30
            radius: 3
            color: landMouse.containsMouse ? "#2C3847" : "#1D2733"
            border.color: landMouse.containsMouse ? "#FFC107" : "#2C3847"
            border.width: 1
            opacity: (missionController && missionController.isInsertLandValid) ? 1.0 : 0.4

            RowLayout {
                anchors.centerIn: parent
                spacing: 4
                Text { text: "▼"; font.pixelSize: 9; color: "#FFC107" }
                Text { id: landText; text: "LAND"; font.family: "Inter"; font.pixelSize: 9; font.weight: Font.Bold; color: "#F5F7FA" }
            }

            MouseArea {
                id: landMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (!missionController || !missionController.isInsertLandValid) return
                    var nextIndex = missionController.currentPlanViewVIIndex + 1
                    missionController.insertLandItem(toolbarRoot.getCenterCoordinate(), nextIndex, true)
                }
            }

            ToolTip.visible: landMouse.containsMouse
            ToolTip.text: "Insert landing sequence"
            ToolTip.delay: 400
        }

        // 4. ROI
        Rectangle {
            implicitWidth: roiText.implicitWidth + 14
            implicitHeight: 30
            radius: 3
            color: roiMouse.containsMouse ? "#2C3847" : "#1D2733"
            border.color: (missionController && missionController.isROIActive) ? "#FF6A00" : (roiMouse.containsMouse ? "#FF6A00" : "#2C3847")
            border.width: 1

            RowLayout {
                anchors.centerIn: parent
                spacing: 4
                Text { text: "◎"; font.pixelSize: 10; color: "#FF6A00" }
                Text {
                    id: roiText
                    text: (missionController && missionController.isROIActive) ? "CANCEL ROI" : "ROI"
                    font.family: "Inter"
                    font.pixelSize: 9
                    font.weight: Font.Bold
                    color: "#F5F7FA"
                }
            }

            MouseArea {
                id: roiMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (!missionController) return
                    var nextIndex = missionController.currentPlanViewVIIndex + 1
                    if (missionController.isROIActive) {
                        missionController.insertCancelROIMissionItem(nextIndex, true)
                    } else {
                        missionController.insertROIMissionItem(toolbarRoot.getCenterCoordinate(), nextIndex, true)
                    }
                }
            }

            ToolTip.visible: roiMouse.containsMouse
            ToolTip.text: (missionController && missionController.isROIActive) ? "Cancel active Region of Interest" : "Insert Region of Interest target"
            ToolTip.delay: 400
        }

        Rectangle { width: 1; height: 18; color: "#2C3847" }

        // 5. SURVEY (Complex Pattern)
        Rectangle {
            implicitWidth: surveyText.implicitWidth + 14
            implicitHeight: 30
            radius: 3
            color: surveyMouse.containsMouse ? "#2C3847" : "#1D2733"
            border.color: surveyMouse.containsMouse ? "#00C853" : "#2C3847"
            border.width: 1

            RowLayout {
                anchors.centerIn: parent
                spacing: 4
                Text { text: "▤"; font.pixelSize: 9; color: "#00C853" }
                Text { id: surveyText; text: "SURVEY"; font.family: "Inter"; font.pixelSize: 9; font.weight: Font.Bold; color: "#F5F7FA" }
            }

            MouseArea {
                id: surveyMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (!missionController) return
                    var nextIndex = missionController.currentPlanViewVIIndex + 1
                    if (missionController.surveyComplexItemName) {
                        missionController.insertComplexMissionItem(missionController.surveyComplexItemName, toolbarRoot.getCenterCoordinate(), nextIndex, true)
                    } else if (planView && typeof planView.insertComplexItemAfterCurrent === "function") {
                        planView.insertComplexItemAfterCurrent(missionController.surveyComplexItemName)
                    }
                }
            }

            ToolTip.visible: surveyMouse.containsMouse
            ToolTip.text: "Create grid survey polygon"
            ToolTip.delay: 400
        }

        // 6. CORRIDOR SCAN
        Rectangle {
            implicitWidth: corrText.implicitWidth + 14
            implicitHeight: 30
            radius: 3
            color: corrMouse.containsMouse ? "#2C3847" : "#1D2733"
            border.color: corrMouse.containsMouse ? "#00C853" : "#2C3847"
            border.width: 1

            RowLayout {
                anchors.centerIn: parent
                spacing: 4
                Text { text: "═"; font.pixelSize: 9; color: "#00C853" }
                Text { id: corrText; text: "CORRIDOR"; font.family: "Inter"; font.pixelSize: 9; font.weight: Font.Bold; color: "#F5F7FA" }
            }

            MouseArea {
                id: corrMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (!missionController) return
                    var nextIndex = missionController.currentPlanViewVIIndex + 1
                    if (missionController.corridorScanComplexItemName) {
                        missionController.insertComplexMissionItem(missionController.corridorScanComplexItemName, toolbarRoot.getCenterCoordinate(), nextIndex, true)
                    } else if (planView && typeof planView.insertComplexItemAfterCurrent === "function") {
                        planView.insertComplexItemAfterCurrent(missionController.corridorScanComplexItemName)
                    }
                }
            }

            ToolTip.visible: corrMouse.containsMouse
            ToolTip.text: "Create corridor scan route"
            ToolTip.delay: 400
        }

        // 7. STRUCTURE SCAN
        Rectangle {
            implicitWidth: structText.implicitWidth + 14
            implicitHeight: 30
            radius: 3
            color: structMouse.containsMouse ? "#2C3847" : "#1D2733"
            border.color: structMouse.containsMouse ? "#00C853" : "#2C3847"
            border.width: 1

            RowLayout {
                anchors.centerIn: parent
                spacing: 4
                Text { text: "▦"; font.pixelSize: 9; color: "#00C853" }
                Text { id: structText; text: "STRUCTURE"; font.family: "Inter"; font.pixelSize: 9; font.weight: Font.Bold; color: "#F5F7FA" }
            }

            MouseArea {
                id: structMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (!missionController) return
                    var nextIndex = missionController.currentPlanViewVIIndex + 1
                    if (missionController.structureScanComplexItemName) {
                        missionController.insertComplexMissionItem(missionController.structureScanComplexItemName, toolbarRoot.getCenterCoordinate(), nextIndex, true)
                    }
                }
            }

            ToolTip.visible: structMouse.containsMouse
            ToolTip.text: "Create 3D structure scan route"
            ToolTip.delay: 400
        }

        Rectangle { width: 1; height: 18; color: "#2C3847" }

        // 8. DELETE CURRENT ITEM
        Rectangle {
            implicitWidth: delText.implicitWidth + 14
            implicitHeight: 30
            radius: 3
            color: delMouse.containsMouse ? "#3A1E24" : "#1D2733"
            border.color: delMouse.containsMouse ? "#F44336" : "#2C3847"
            border.width: 1
            opacity: (missionController && missionController.currentPlanViewVIIndex > 0) ? 1.0 : 0.4

            RowLayout {
                anchors.centerIn: parent
                spacing: 4
                Text { text: "✕"; font.pixelSize: 9; color: "#F44336" }
                Text { id: delText; text: "DELETE"; font.family: "Inter"; font.pixelSize: 9; font.weight: Font.Bold; color: "#F5F7FA" }
            }

            MouseArea {
                id: delMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (!missionController || missionController.currentPlanViewVIIndex <= 0) return
                    missionController.removeVisualItem(missionController.currentPlanViewVIIndex)
                }
            }

            ToolTip.visible: delMouse.containsMouse
            ToolTip.text: "Delete selected mission item"
            ToolTip.delay: 400
        }

        // 9. CLEAR ALL
        Rectangle {
            implicitWidth: clearText.implicitWidth + 14
            implicitHeight: 30
            radius: 3
            color: clearMouse.containsMouse ? "#3A1E24" : "#1D2733"
            border.color: clearMouse.containsMouse ? "#F44336" : "#2C3847"
            border.width: 1
            opacity: (missionController && missionController.visualItems && missionController.visualItems.count > 1) ? 1.0 : 0.4

            RowLayout {
                anchors.centerIn: parent
                spacing: 4
                Text { text: "⌫"; font.pixelSize: 10; color: "#F44336" }
                Text { id: clearText; text: "CLEAR"; font.family: "Inter"; font.pixelSize: 9; font.weight: Font.Bold; color: "#F5F7FA" }
            }

            MouseArea {
                id: clearMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (planMasterController) {
                        planMasterController.removeAll()
                    }
                }
            }

            ToolTip.visible: clearMouse.containsMouse
            ToolTip.text: "Clear all mission items from planner"
            ToolTip.delay: 400
        }

        Rectangle { width: 1; height: 18; color: "#2C3847" }

        // 10. SAVE PLAN FILE
        Rectangle {
            implicitWidth: saveText.implicitWidth + 14
            implicitHeight: 30
            radius: 3
            color: saveMouse.containsMouse ? "#2C3847" : "#1D2733"
            border.color: saveMouse.containsMouse ? "#FF6A00" : "#2C3847"
            border.width: 1

            RowLayout {
                anchors.centerIn: parent
                spacing: 4
                Text { text: "💾"; font.pixelSize: 9 }
                Text { id: saveText; text: "SAVE"; font.family: "Inter"; font.pixelSize: 9; font.weight: Font.Bold; color: "#F5F7FA" }
            }

            MouseArea {
                id: saveMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (planMasterController) {
                        if (planMasterController.currentPlanFile && planMasterController.currentPlanFile.length > 0) {
                            planMasterController.saveToCurrent()
                        } else {
                            planMasterController.saveToSelectedFile()
                        }
                    }
                }
            }

            ToolTip.visible: saveMouse.containsMouse
            ToolTip.text: "Save mission plan to .plan file"
            ToolTip.delay: 400
        }

        // 11. LOAD PLAN FILE
        Rectangle {
            implicitWidth: loadText.implicitWidth + 14
            implicitHeight: 30
            radius: 3
            color: loadMouse.containsMouse ? "#2C3847" : "#1D2733"
            border.color: loadMouse.containsMouse ? "#FF6A00" : "#2C3847"
            border.width: 1

            RowLayout {
                anchors.centerIn: parent
                spacing: 4
                Text { text: "📂"; font.pixelSize: 9 }
                Text { id: loadText; text: "LOAD"; font.family: "Inter"; font.pixelSize: 9; font.weight: Font.Bold; color: "#F5F7FA" }
            }

            MouseArea {
                id: loadMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (planMasterController) {
                        planMasterController.loadFromSelectedFile()
                    }
                }
            }

            ToolTip.visible: loadMouse.containsMouse
            ToolTip.text: "Open mission plan from .plan file"
            ToolTip.delay: 400
        }

        Rectangle { width: 1; height: 18; color: "#2C3847" }

        // 12. UPLOAD TO DRONE
        Rectangle {
            implicitWidth: upText.implicitWidth + (syncSpin.visible ? 28 : 16)
            implicitHeight: 30
            radius: 3
            color: upMouse.containsMouse ? "#00E676" : "#00C853"
            opacity: (planMasterController && !planMasterController.syncInProgress) ? 1.0 : 0.6

            RowLayout {
                anchors.centerIn: parent
                spacing: 4

                // Spinner if sync in progress
                Rectangle {
                    id: syncSpin
                    width: 10
                    height: 10
                    radius: 5
                    color: "transparent"
                    border.color: "#0A0F14"
                    border.width: 2
                    visible: !!planMasterController && planMasterController.syncInProgress

                    RotationAnimator on rotation {
                        from: 0
                        to: 360
                        duration: 1000
                        loops: Animation.Infinite
                        running: syncSpin.visible
                    }
                }

                Text {
                    text: syncSpin.visible ? "" : "⬆"
                    font.pixelSize: 10
                    font.weight: Font.Bold
                    color: "#0A0F14"
                    visible: !syncSpin.visible
                }

                Text {
                    id: upText
                    text: (planMasterController && planMasterController.syncInProgress) ? "SYNCING..." : "UPLOAD"
                    font.family: "Inter"
                    font.pixelSize: 9
                    font.weight: Font.Bold
                    color: "#0A0F14"
                }
            }

            MouseArea {
                id: upMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (planMasterController && !planMasterController.syncInProgress) {
                        planMasterController.upload()
                    }
                }
            }

            ToolTip.visible: upMouse.containsMouse
            ToolTip.text: "Upload active plan to drone autopilot"
            ToolTip.delay: 400
        }

        // 13. DOWNLOAD FROM DRONE
        Rectangle {
            implicitWidth: dlText.implicitWidth + 14
            implicitHeight: 30
            radius: 3
            color: dlMouse.containsMouse ? "#2C3847" : "#1D2733"
            border.color: dlMouse.containsMouse ? "#00C853" : "#2C3847"
            border.width: 1
            opacity: (planMasterController && !planMasterController.syncInProgress) ? 1.0 : 0.4

            RowLayout {
                anchors.centerIn: parent
                spacing: 4
                Text { text: "⬇"; font.pixelSize: 10; color: "#00C853" }
                Text { id: dlText; text: "DOWNLOAD"; font.family: "Inter"; font.pixelSize: 9; font.weight: Font.Bold; color: "#F5F7FA" }
            }

            MouseArea {
                id: dlMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (planMasterController && !planMasterController.syncInProgress) {
                        planMasterController.loadFromVehicle()
                    }
                }
            }

            ToolTip.visible: dlMouse.containsMouse
            ToolTip.text: "Download active mission from drone"
            ToolTip.delay: 400
        }
    }
}
