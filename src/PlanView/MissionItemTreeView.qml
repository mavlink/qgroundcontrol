import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls
import QGroundControl.PlanView

/// Unified plan tree view showing Mission Items, GeoFence, and Rally Points
/// as collapsible sections using a real TreeView with type-discriminating delegates.
TreeView {
    id: root

    required property var editorMap
    required property var planMasterController

    signal editingLayerChangeRequested(int layer)

    readonly property int _layerMission: 1
    readonly property int _layerFence:   2
    readonly property int _layerRally:   3

    property var _missionController: planMasterController.missionController
    property var _geoFenceController: planMasterController.geoFenceController
    property var _rallyPointController: planMasterController.rallyPointController

    model: _missionController.visualItemsTree
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    reuseItems: false
    pointerNavigationEnabled: false
    selectionBehavior: TableView.SelectionDisabled
    rowSpacing: 2

    // Helper: convert a persistent model index to the current visual row
    function _rowFor(modelIndex) { return root.rowAtIndex(modelIndex) }

    // QGCFlickableScrollIndicator expects parent to have indicatorColor (provided by QGCFlickable/QGCListView)
    property color indicatorColor: qgcPal.text

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    QGCFlickableScrollIndicator { parent: root; orientation: QGCFlickableScrollIndicator.Horizontal }
    QGCFlickableScrollIndicator { parent: root; orientation: QGCFlickableScrollIndicator.Vertical }

    Connections {
        target: root._missionController
        function onVisualItemsChanged() {
            // Mission group always expanded after rebuild (clear / load)
            root.collapseRecursively()
            root.expand(_rowFor(_missionController.missionGroupIndex))
            root.editingLayerChangeRequested(root._layerMission)
        }
    }

    // Public API: select a layer and expand its group. Called by the layer tool buttons.
    function selectLayer(nodeType) {
        let targetRow = -1
        switch (nodeType) {
        case "missionGroup":
            targetRow = _rowFor(_missionController.missionGroupIndex)
            editingLayerChangeRequested(_layerMission)
            break
        case "fenceGroup":
            targetRow = _rowFor(_missionController.fenceGroupIndex)
            editingLayerChangeRequested(_layerFence)
            break
        case "rallyGroup":
            targetRow = _rowFor(_missionController.rallyGroupIndex)
            editingLayerChangeRequested(_layerRally)
            break
        }

        if (targetRow >= 0) {
            if (!root.isExpanded(targetRow))
                root.expand(targetRow)
            root.forceLayout()
            root.positionViewAtRow(targetRow, TableView.AlignTop)
        }
    }

    // Toggle expand/collapse for a group header. Does not affect the editing layer.
    function _toggleGroup(row) {
        if (root.isExpanded(row))
            root.collapse(row)
        else
            root.expand(row)
        root.forceLayout()
    }

    // Coalesces multiple delegate height changes into a single forceLayout() call
    Timer {
        id: layoutTimer
        interval: 0
        running: false
        repeat: false
        onTriggered: root.forceLayout()
    }

    delegate: Item {
        id: delegateRoot

        required property TreeView treeView
        required property bool isTreeNode
        required property bool expanded
        required property bool hasChildren
        required property int depth
        required property int row
        required property var model

        readonly property var nodeObject: model.object
        readonly property string nodeType: model.nodeType

        implicitWidth: root.width
        implicitHeight: loader.item ? loader.item.height : 1
        width: root.width
        height: implicitHeight

        onImplicitHeightChanged: layoutTimer.restart()

        Loader {
            id: loader
            width: parent.width
            sourceComponent: {
                // Guard: non-group delegates need a valid object. During model
                // row removal the role data goes null before the delegate is
                // destroyed, which would cause "Cannot read property of null"
                // warnings in every downstream editor binding.
                switch (delegateRoot.nodeType) {
                case "planFileGroup":   return groupHeaderComponent
                case "defaultsGroup":   return groupHeaderComponent
                case "missionGroup":    return groupHeaderComponent
                case "fenceGroup":      return groupHeaderComponent
                case "rallyGroup":      return groupHeaderComponent
                case "transformGroup":  return groupHeaderComponent
                case "planFileInfo":    return planFileInfoComponent
                case "defaultsInfo":    return defaultsEditorComponent
                case "missionItem":     return delegateRoot.nodeObject ? missionItemComponent  : null
                case "fenceEditor":     return delegateRoot.nodeObject ? fenceEditorComponent  : null
                case "rallyHeader":     return delegateRoot.nodeObject ? rallyHeaderComponent  : null
                case "rallyItem":       return delegateRoot.nodeObject ? rallyItemComponent    : null
                case "transformEditor": return transformEditorComponent
                default:                return null
                }
            }
        }

        // ── Group header (Mission Items / GeoFence / Rally Points) ──
        Component {
            id: groupHeaderComponent

            Rectangle {
                width:  delegateRoot.width
                height: ScreenTools.implicitComboBoxHeight + ScreenTools.defaultFontPixelWidth
                color:  qgcPal.windowShade

                Row {
                    id: groupHeaderRow
                    spacing: ScreenTools.defaultFontPixelWidth * 0.5
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: ScreenTools.defaultFontPixelWidth * 0.5

                    QGCColoredImage {
                        width: ScreenTools.defaultFontPixelHeight * 0.75
                        height: width
                        source: "/InstrumentValueIcons/cheveron-right.svg"
                        color: qgcPal.text
                        anchors.verticalCenter: parent.verticalCenter
                        rotation: delegateRoot.expanded ? 90 : 0
                    }

                    QGCLabel {
                        text: delegateRoot.nodeObject ? delegateRoot.nodeObject.objectName : ""
                        font.bold: true
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: root._toggleGroup(delegateRoot.row)
                }
            }
        }

        // ── Plan file info delegate ──
        Component {
            id: planFileInfoComponent

            Rectangle {
                width: delegateRoot.width
                height: planFileColumn.height + ScreenTools.defaultFontPixelHeight
                color: qgcPal.windowShadeDark

                Column {
                    id: planFileColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.margins: ScreenTools.defaultFontPixelWidth
                    spacing: ScreenTools.defaultFontPixelHeight * 0.25

                    QGCTextField {
                        id: planNameField
                        placeholderText: qsTr("Untitled")
                        width: parent.width

                        Component.onCompleted: text = root.planMasterController.currentPlanFileName

                        Connections {
                            target: root.planMasterController
                            function onCurrentPlanFileNameChanged() {
                                if (!planNameField.activeFocus) {
                                    planNameField.text = root.planMasterController.currentPlanFileName
                                }
                            }
                        }

                        onEditingFinished: root.planMasterController.currentPlanFileName = text
                    }
                }
            }
        }

        // ── Defaults editor delegate ──
        Component {
            id: defaultsEditorComponent

            Rectangle {
                id: defaultsRect
                width: delegateRoot.width
                height: defaultsColumn.height + ScreenTools.defaultFontPixelHeight
                color: qgcPal.windowShadeDark

                property var _missionController: root._missionController
                property var _controllerVehicle: root.planMasterController.controllerVehicle
                property var _visualItems: root._missionController.visualItems
                property bool _noMissionItemsAdded: _visualItems ? _visualItems.count <= 1 : true
                property var _settingsItem: _visualItems && _visualItems.count > 0 ? _visualItems.get(0) : null
                property bool _multipleFirmware: !QGroundControl.singleFirmwareSupport
                property bool _multipleVehicleTypes: !QGroundControl.singleVehicleSupport
                property bool _allowFWVehicleTypeSelection: _noMissionItemsAdded && !globals.activeVehicle
                property bool _showCruiseSpeed: _controllerVehicle ? !_controllerVehicle.multiRotor : false
                property bool _showHoverSpeed: _controllerVehicle ? (_controllerVehicle.multiRotor || _controllerVehicle.vtol) : false
                property bool _vehicleHasHomePosition: _controllerVehicle ? _controllerVehicle.homePosition.isValid : false
                property bool _waypointsOnlyMode: QGroundControl.corePlugin.options.missionWaypointsOnly
                property real _fieldWidth: ScreenTools.defaultFontPixelWidth * 16
                readonly property real _margin: ScreenTools.defaultFontPixelWidth / 2

                Connections {
                    target: defaultsRect._controllerVehicle
                    function onFirmwareTypeChanged() {
                        if (!defaultsRect._controllerVehicle.supports.terrainFrame
                                && defaultsRect._missionController.globalAltitudeMode === QGroundControl.AltitudeModeTerrainFrame) {
                            defaultsRect._missionController.globalAltitudeMode = QGroundControl.AltitudeModeCalcAboveTerrain
                        }
                    }
                }

                Component { id: altModeDialogComponent; AltModeDialog { } }

                QGCPopupDialogFactory {
                    id: defaultsAltModeDialogFactory
                    dialogComponent: altModeDialogComponent
                }

                ColumnLayout {
                    id: defaultsColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.margins: ScreenTools.defaultFontPixelWidth
                    spacing: ScreenTools.defaultFontPixelHeight * 0.5

                    LabelledButton {
                        Layout.fillWidth: true
                        label: qsTr("Altitude Mode")
                        buttonText: QGroundControl.altitudeModeShortDescription(defaultsRect._missionController.globalAltitudeMode)

                        onClicked: {
                            let removeModes = []
                            let updateFunction = function(altMode) { defaultsRect._missionController.globalAltitudeMode = altMode }
                            if (!defaultsRect._controllerVehicle.supports.terrainFrame) {
                                removeModes.push(QGroundControl.AltitudeModeTerrainFrame)
                            }
                            if (!defaultsRect._noMissionItemsAdded) {
                                if (defaultsRect._missionController.globalAltitudeMode !== QGroundControl.AltitudeModeRelative) {
                                    removeModes.push(QGroundControl.AltitudeModeRelative)
                                }
                                if (defaultsRect._missionController.globalAltitudeMode !== QGroundControl.AltitudeModeAbsolute) {
                                    removeModes.push(QGroundControl.AltitudeModeAbsolute)
                                }
                                if (defaultsRect._missionController.globalAltitudeMode !== QGroundControl.AltitudeModeCalcAboveTerrain) {
                                    removeModes.push(QGroundControl.AltitudeModeCalcAboveTerrain)
                                }
                                if (defaultsRect._missionController.globalAltitudeMode !== QGroundControl.AltitudeModeTerrainFrame) {
                                    removeModes.push(QGroundControl.AltitudeModeTerrainFrame)
                                }
                            }
                            defaultsAltModeDialogFactory.open({ rgRemoveModes: removeModes, updateAltModeFn: updateFunction })
                        }
                    }

                    FactTextFieldSlider {
                        Layout.fillWidth: true
                        label: qsTr("Waypoints Altitude")
                        fact: QGroundControl.settingsManager.appSettings.defaultMissionItemAltitude
                    }

                    FactTextFieldSlider {
                        Layout.fillWidth: true
                        label: qsTr("Flight Speed")
                        fact: defaultsRect._settingsItem ? defaultsRect._settingsItem.speedSection.flightSpeed : null
                        showEnableCheckbox: true
                        enableCheckBoxChecked: defaultsRect._settingsItem ? defaultsRect._settingsItem.speedSection.specifyFlightSpeed : false
                        visible: defaultsRect._settingsItem ? defaultsRect._settingsItem.speedSection.available : false

                        onEnableCheckboxClicked: {
                            if (defaultsRect._settingsItem) {
                                defaultsRect._settingsItem.speedSection.specifyFlightSpeed = enableCheckBoxChecked
                            }
                        }
                    }

                    // ── Vehicle Info ──
                    SectionHeader {
                        id: vehicleInfoSectionHeader
                        Layout.fillWidth: true
                        text: qsTr("Vehicle Info")
                        visible: !defaultsRect._waypointsOnlyMode
                        checked: false
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columnSpacing: ScreenTools.defaultFontPixelWidth
                        rowSpacing: columnSpacing
                        columns: 2
                        visible: vehicleInfoSectionHeader.visible && vehicleInfoSectionHeader.checked

                        QGCLabel {
                            text: qsTr("Firmware")
                            Layout.fillWidth: true
                            visible: defaultsRect._multipleFirmware
                        }
                        FactComboBox {
                            fact: QGroundControl.settingsManager.appSettings.offlineEditingFirmwareClass
                            indexModel: false
                            Layout.preferredWidth: defaultsRect._fieldWidth
                            visible: defaultsRect._multipleFirmware && defaultsRect._allowFWVehicleTypeSelection
                        }
                        QGCLabel {
                            text: defaultsRect._controllerVehicle ? defaultsRect._controllerVehicle.firmwareTypeString : ""
                            visible: defaultsRect._multipleFirmware && !defaultsRect._allowFWVehicleTypeSelection
                        }

                        QGCLabel {
                            text: qsTr("Vehicle")
                            Layout.fillWidth: true
                            visible: defaultsRect._multipleVehicleTypes
                        }
                        FactComboBox {
                            fact: QGroundControl.settingsManager.appSettings.offlineEditingVehicleClass
                            indexModel: false
                            Layout.preferredWidth: defaultsRect._fieldWidth
                            visible: defaultsRect._multipleVehicleTypes && defaultsRect._allowFWVehicleTypeSelection
                        }
                        QGCLabel {
                            text: defaultsRect._controllerVehicle ? defaultsRect._controllerVehicle.vehicleTypeString : ""
                            visible: defaultsRect._multipleVehicleTypes && !defaultsRect._allowFWVehicleTypeSelection
                        }

                        QGCLabel {
                            Layout.columnSpan: 2
                            Layout.alignment: Qt.AlignHCenter
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            font.pointSize: ScreenTools.smallFontPointSize
                            text: qsTr("The following speed values are used to calculate total mission time. They do not affect the flight speed for the mission.")
                            visible: defaultsRect._showCruiseSpeed || defaultsRect._showHoverSpeed
                        }

                        QGCLabel {
                            text: qsTr("Cruise speed")
                            visible: defaultsRect._showCruiseSpeed
                            Layout.fillWidth: true
                        }
                        FactTextField {
                            fact: QGroundControl.settingsManager.appSettings.offlineEditingCruiseSpeed
                            visible: defaultsRect._showCruiseSpeed
                            Layout.preferredWidth: defaultsRect._fieldWidth
                        }

                        QGCLabel {
                            text: qsTr("Hover speed")
                            visible: defaultsRect._showHoverSpeed
                            Layout.fillWidth: true
                        }
                        FactTextField {
                            fact: QGroundControl.settingsManager.appSettings.offlineEditingHoverSpeed
                            visible: defaultsRect._showHoverSpeed
                            Layout.preferredWidth: defaultsRect._fieldWidth
                        }
                    }

                    // ── Launch Position ──
                    SectionHeader {
                        id: plannedHomePositionSection
                        Layout.fillWidth: true
                        text: qsTr("Launch Position")
                        visible: !defaultsRect._vehicleHasHomePosition
                        checked: false
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columnSpacing: ScreenTools.defaultFontPixelWidth
                        rowSpacing: columnSpacing
                        columns: 2
                        visible: plannedHomePositionSection.checked && !defaultsRect._vehicleHasHomePosition

                        QGCLabel {
                            text: qsTr("Altitude")
                        }
                        FactTextField {
                            fact: defaultsRect._settingsItem ? defaultsRect._settingsItem.plannedHomePositionAltitude : null
                            Layout.fillWidth: true
                        }
                    }

                    QGCLabel {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        font.pointSize: ScreenTools.smallFontPointSize
                        text: qsTr("Actual position set by vehicle at flight time.")
                        horizontalAlignment: Text.AlignHCenter
                        visible: plannedHomePositionSection.checked && !defaultsRect._vehicleHasHomePosition
                    }

                    QGCButton {
                        text: qsTr("Set To Map Center")
                        Layout.alignment: Qt.AlignHCenter
                        visible: plannedHomePositionSection.checked && !defaultsRect._vehicleHasHomePosition
                        onClicked: {
                            if (defaultsRect._settingsItem) {
                                defaultsRect._settingsItem.coordinate = root.editorMap.center
                            }
                        }
                    }
                }
            }
        }

        // ── Mission item delegate ──
        Component {
            id: missionItemComponent

            MissionItemEditor {
                width: delegateRoot.width
                map: root.editorMap
                masterController: root.planMasterController
                missionItem: delegateRoot.nodeObject
                readOnly: false

                onClicked:  root._missionController.setCurrentPlanViewSeqNum(delegateRoot.nodeObject.sequenceNumber, false)

                onRemove: {
                    var viIndex = root._missionController.visualItemIndexForObject(delegateRoot.nodeObject)
                    if (viIndex > 0) {
                        root._missionController.removeVisualItem(viIndex)
                    }
                }

                onSelectNextNotReadyItem: {
                    for (var i = 0; i < root._missionController.visualItems.count; i++) {
                        var vmi = root._missionController.visualItems.get(i)
                        if (vmi.readyForSaveState === VisualMissionItem.NotReadyForSaveData) {
                            root._missionController.setCurrentPlanViewSeqNum(vmi.sequenceNumber, true)
                            break
                        }
                    }
                }
            }
        }

        // ── GeoFence editor (single child of fence group) ──
        Component {
            id: fenceEditorComponent

            GeoFenceEditor {
                width: delegateRoot.width
                myGeoFenceController: root._geoFenceController
                flightMap: root.editorMap
            }
        }

        // ── Rally header / instructions ──
        Component {
            id: rallyHeaderComponent

            RallyPointEditorHeader {
                width: delegateRoot.width
                controller: root._rallyPointController
            }
        }

        // ── Rally point item editor ──
        Component {
            id: rallyItemComponent

            RallyPointItemEditor {
                width: delegateRoot.width
                rallyPoint: delegateRoot.nodeObject
                controller: root._rallyPointController
            }
        }

        // ── Transform editor (single child of transform group) ──
        Component {
            id: transformEditorComponent

            TransformEditor {
                width: delegateRoot.width
                missionController: root._missionController
            }
        }
    }
}
