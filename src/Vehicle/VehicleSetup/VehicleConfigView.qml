import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

Rectangle {
    id:     vehicleConfigView
    color:  qgcPal.window
    z:      QGroundControl.zOrderTopMost

    // This need to block click event leakage to underlying map.
    DeadMouseArea {
        anchors.fill: parent
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    readonly property real      _defaultTextHeight: ScreenTools.defaultFontPixelHeight
    readonly property real      _defaultTextWidth:  ScreenTools.defaultFontPixelWidth
    readonly property real      _horizontalMargin:  _defaultTextWidth / 2
    readonly property real      _verticalMargin:    _defaultTextHeight / 2
    readonly property real      _buttonWidth:       _defaultTextWidth * 18
    readonly property string    _armedVehicleText:  qsTr("This operation cannot be performed while the vehicle is armed.")

    property var    _activeVehicle:                 QGroundControl.multiVehicleManager.activeVehicle
    property bool   _vehicleArmed:                  _activeVehicle ? _activeVehicle.armed : false
    property string _messagePanelText:              qsTr("missing message panel text")
    property bool   _fullParameterVehicleAvailable: _activeVehicle && QGroundControl.multiVehicleManager.parameterReadyVehicleAvailable && !_activeVehicle.parameterManager.missingParameters
    property var    _corePlugin:                    QGroundControl.corePlugin

    // Tree view state
    property int    _selectedComponentIndex: -1     // -1 = summary or special button
    property int    _selectedSectionIndex:   -1
    property string _selectedSpecial:        ""     // "summary", "parameters", "firmware", "opticalflow"
    property var    _expandedComponents:     ({})
    property int    _expandedRevision:       0
    property string _searchQuery:            ""

    function _setExpanded(compIndex, value) {
        _expandedComponents[compIndex] = value
        _expandedRevision++
    }

    function _isExpanded(compIndex) {
        void _expandedRevision
        return !!_expandedComponents[compIndex]
    }

    /// Translate a section name using the component's JSON filename as context.
    /// Falls back to the raw name when no vehicleConfigJson is set.
    function _translateSection(component, name) {
        var context = _translationContext(component)
        if (!context) return name
        return qsTranslate(context, name)
    }

    /// Get the section name for a sidebar entry.
    function _sectionName(compIndex, sectionIndex) {
        if (sectionIndex < 0 || !_fullParameterVehicleAvailable) return ""
        var components = _activeVehicle.autopilotPlugin.vehicleComponents
        if (compIndex < 0 || compIndex >= components.length) return ""
        var secs = components[compIndex].sections
        if (sectionIndex < secs.length) return secs[sectionIndex]
        return ""
    }

    /// Extract the translation context (JSON filename) from a component.
    function _translationContext(component) {
        if (!component || !component.vehicleConfigJson) return ""
        var path = component.vehicleConfigJson.toString()
        var slash = path.lastIndexOf("/")
        return slash >= 0 ? path.substring(slash + 1) : path
    }

    function _componentMatchesSearch(component) {
        if (_searchQuery.trim() === "") return true
        var query = _searchQuery.toLowerCase().trim()
        if (component.name.toLowerCase().indexOf(query) !== -1) return true
        var context = _translationContext(component)
        var secs = component.sections
        if (secs) {
            for (var i = 0; i < secs.length; i++) {
                if (secs[i].toLowerCase().indexOf(query) !== -1) return true
                if (context && qsTranslate(context, secs[i]).toLowerCase().indexOf(query) !== -1) return true
            }
        }
        var keywords = component.sectionKeywords
        if (keywords) {
            for (var key in keywords) {
                var terms = keywords[key]
                for (var j = 0; j < terms.length; j++) {
                    if (terms[j].toLowerCase().indexOf(query) !== -1) return true
                    if (context && qsTranslate(context, terms[j]).toLowerCase().indexOf(query) !== -1) return true
                }
            }
        }
        return false
    }

    function _sectionMatchesSearch(component, sectionName) {
        if (_searchQuery.trim() === "") return true
        var query = _searchQuery.toLowerCase().trim()
        if (sectionName.toLowerCase().indexOf(query) !== -1) return true
        var context = _translationContext(component)
        if (context && qsTranslate(context, sectionName).toLowerCase().indexOf(query) !== -1) return true
        var keywords = component.sectionKeywords
        if (keywords && keywords[sectionName]) {
            var terms = keywords[sectionName]
            for (var i = 0; i < terms.length; i++) {
                if (terms[i].toLowerCase().indexOf(query) !== -1) return true
                if (context && qsTranslate(context, terms[i]).toLowerCase().indexOf(query) !== -1) return true
            }
        }
        return false
    }

    function showSummaryPanel() {
        if (mainWindow.allowViewSwitch()) {
            _showSummaryPanel()
        }
    }

    function _showSummaryPanel() {
        _selectedSpecial = "summary"
        _selectedComponentIndex = -1
        _selectedSectionIndex = -1
        if (_fullParameterVehicleAvailable) {
            if (_activeVehicle.autopilotPlugin.vehicleComponents.length === 0) {
                panelLoader.setSourceComponent(noComponentsVehicleSummaryComponent)
            } else {
                panelLoader.setSource("qrc:/qml/QGroundControl/VehicleSetup/VehicleSummary.qml")
            }
        } else if (QGroundControl.multiVehicleManager.parameterReadyVehicleAvailable) {
            panelLoader.setSourceComponent(missingParametersVehicleSummaryComponent)
        } else {
            panelLoader.setSourceComponent(disconnectedVehicleAndParamsSummaryComponent)
        }
    }

    function showPanel(specialName, qmlSource) {
        if (mainWindow.allowViewSwitch()) {
            _selectedSpecial = specialName
            _selectedComponentIndex = -1
            _selectedSectionIndex = -1
            panelLoader.setSource(qmlSource)
        }
    }

    function _navigateToComponent(compIndex, sectionIndex) {
        if (!mainWindow.allowViewSwitch()) return
        if (!_fullParameterVehicleAvailable) return

        var components = _activeVehicle.autopilotPlugin.vehicleComponents
        if (compIndex < 0 || compIndex >= components.length) return
        var vehicleComponent = components[compIndex]

        var autopilotPlugin = _activeVehicle.autopilotPlugin
        var prereq = autopilotPlugin.prerequisiteSetup(vehicleComponent)
        if (prereq !== "") {
            _messagePanelText = qsTr("%1 setup must be completed prior to %2 setup.").arg(prereq).arg(vehicleComponent.name)
            panelLoader.setSourceComponent(messagePanelComponent)
            return
        }

        _selectedSpecial = ""

        // If component opts in and root was clicked, auto-select first section
        if (sectionIndex < 0 && vehicleComponent.showFirstSectionOnRootClick && vehicleComponent.sections.length > 0) {
            sectionIndex = 0
        }
        _selectedSectionIndex = sectionIndex

        if (_selectedComponentIndex !== compIndex) {
            _selectedComponentIndex = compIndex
            panelLoader.setSource(vehicleComponent.setupSource, vehicleComponent)
        }

        // Apply section filter
        if (panelLoader.item && typeof panelLoader.item.sectionNameFilter !== "undefined") {
            panelLoader.item.sectionNameFilter = _sectionName(compIndex, sectionIndex)
        }
    }

    function showParametersPanel() {
        showPanel("parameters", "qrc:/qml/QGroundControl/VehicleSetup/SetupParameterEditor.qml")
    }

    function showVehicleComponentPanel(vehicleComponent) {
        if (!mainWindow.allowViewSwitch()) return
        if (!_fullParameterVehicleAvailable) return

        var components = _activeVehicle.autopilotPlugin.vehicleComponents
        for (var i = 0; i < components.length; i++) {
            if (components[i] === vehicleComponent) {
                _navigateToComponent(i, -1)
                return
            }
        }
    }

    Component.onCompleted: _showSummaryPanel()

    Connections {
        target: QGroundControl.corePlugin
        function onShowAdvancedUIChanged(showAdvancedUI) {
            if (!showAdvancedUI) {
                _showSummaryPanel()
            }
        }
    }

    Connections {
        target: QGroundControl.multiVehicleManager
        function onParameterReadyVehicleAvailableChanged(parametersReady) {
            if (parametersReady || _selectedSpecial === "summary" || _selectedSpecial !== "firmware") {
                _showSummaryPanel()
            }
        }
    }

    Connections {
        target: panelLoader
        function onLoaded() {
            if (panelLoader.item && typeof panelLoader.item.sectionNameFilter !== "undefined") {
                panelLoader.item.sectionNameFilter = _sectionName(_selectedComponentIndex, _selectedSectionIndex)
            }
        }
    }

    Component {
        id: noComponentsVehicleSummaryComponent
        Rectangle {
            color: qgcPal.windowShade
            QGCLabel {
                anchors.margins:        _defaultTextWidth * 2
                anchors.fill:           parent
                verticalAlignment:      Text.AlignVCenter
                horizontalAlignment:    Text.AlignHCenter
                wrapMode:               Text.WordWrap
                font.pointSize:         ScreenTools.mediumFontPointSize
                text:                   qsTr("%1 does not currently support configuration of your vehicle. ").arg(QGroundControl.appName) +
                                        "If your vehicle is already configured you can still Fly."
            }
        }
    }

    Component {
        id: disconnectedVehicleAndParamsSummaryComponent
        Rectangle {
            id: disconnectedRect
            color: qgcPal.windowShade
            Column {
                anchors.centerIn:   parent
                spacing:            ScreenTools.defaultFontPixelHeight
                QGCLabel {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width:              disconnectedRect.width - _defaultTextWidth * 4
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode:           Text.WordWrap
                    font.pointSize:     ScreenTools.largeFontPointSize
                    text:               !_activeVehicle
                                            ? qsTr("Vehicle configuration pages will display after you connect your vehicle and parameters have been downloaded.")
                                            : (_activeVehicle.parameterManager.parameterDownloadSkipped
                                                ? qsTr("Parameter download was skipped because the vehicle is flying. Configuration pages will be available after parameters are downloaded.")
                                                : qsTr("Waiting for vehicle parameters to download…"))
                }
                QGCButton {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text:       qsTr("Download Parameters")
                    visible:    _activeVehicle && _activeVehicle.parameterManager.parameterDownloadSkipped
                    enabled:    _activeVehicle && _activeVehicle.parameterManager.parameterDownloadSkipped && _activeVehicle.parameterManager.loadProgress === 0
                    onClicked:  _activeVehicle.parameterManager.refreshAllParameters()
                }
            }
        }
    }

    Component {
        id: missingParametersVehicleSummaryComponent

        Rectangle {
            color: qgcPal.windowShade

            QGCLabel {
                anchors.margins:        _defaultTextWidth * 2
                anchors.fill:           parent
                verticalAlignment:      Text.AlignVCenter
                horizontalAlignment:    Text.AlignHCenter
                wrapMode:               Text.WordWrap
                font.pointSize:         ScreenTools.mediumFontPointSize
                text:                   qsTr("Vehicle did not return the full parameter list. ") +
                                        qsTr("As a result, the configuration pages are not available.")
            }
        }
    }

    Component {
        id: messagePanelComponent

        Item {
            QGCLabel {
                anchors.margins:        _defaultTextWidth * 2
                anchors.fill:           parent
                verticalAlignment:      Text.AlignVCenter
                horizontalAlignment:    Text.AlignHCenter
                wrapMode:               Text.WordWrap
                font.pointSize:         ScreenTools.mediumFontPointSize
                text:                   _messagePanelText
            }
        }
    }

    ColumnLayout {
        id:                 leftPanel
        width:              Math.max(buttonColumn.implicitWidth + _horizontalMargin, ScreenTools.defaultFontPixelWidth * 22)
        anchors.topMargin:  _verticalMargin
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        anchors.leftMargin: _horizontalMargin
        anchors.left:       parent.left
        spacing:            _verticalMargin / 2

        QGCTextField {
            id:                 searchField
            Layout.fillWidth:   true
            placeholderText:    qsTr("Search configuration...")
            visible:            _fullParameterVehicleAvailable

            onTextChanged: {
                vehicleConfigView._searchQuery = text
            }
        }

        QGCFlickable {
            Layout.fillWidth:   true
            Layout.fillHeight:  true
            contentHeight:      buttonColumn.height + _verticalMargin
            flickableDirection:  Flickable.VerticalFlick
            clip:               true

            ColumnLayout {
                id:         buttonColumn
                width:      parent.width
                spacing:    0

                // Summary button
                ConfigButton {
                    id:                 summaryButton
                    icon.source:        "/qmlimages/VehicleSummaryIcon.png"
                    checked:            vehicleConfigView._selectedSpecial === "summary"
                    text:               qsTr("Summary")
                    Layout.fillWidth:   true
                    visible:            vehicleConfigView._searchQuery.trim() === ""

                    onClicked: showSummaryPanel()
                }

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: ScreenTools.defaultFontPixelHeight / 2
                    visible: vehicleConfigView._searchQuery.trim() === ""
                }

                // Vehicle component tree
                Repeater {
                    id:     componentRepeater
                    model:  _fullParameterVehicleAvailable ? _activeVehicle.autopilotPlugin.vehicleComponents : 0

                    ColumnLayout {
                        id:             compColumn
                        spacing:        0
                        Layout.fillWidth: true

                        required property int index
                        required property var modelData

                        property var    comp:           modelData
                        property string compName:       comp ? comp.name : ""
                        property var    compSections:   comp ? comp.sections : []
                        property bool   isSelected:     vehicleConfigView._selectedComponentIndex === index && vehicleConfigView._selectedSpecial === ""
                        property bool   hasSections:    compSections.length > 1
                        property bool   isSearching:    vehicleConfigView._searchQuery.trim() !== ""
                        property bool   matchesSearch:  comp ? vehicleConfigView._componentMatchesSearch(comp) : false
                        property bool   isExpanded:     hasSections && (isSearching ? matchesSearch : vehicleConfigView._isExpanded(index))

                        visible: {
                            if (!comp) return false
                            if (comp.setupSource.toString() === "") return false
                            if (isSearching) return matchesSearch
                            return true
                        }

                        ConfigButton {
                            Layout.fillWidth:   true
                            icon.source:        compColumn.comp ? compColumn.comp.iconResource : ""
                            setupComplete:      compColumn.comp ? compColumn.comp.setupComplete : true
                            text:               compColumn.compName
                            expandable:         compColumn.hasSections
                            expanded:           compColumn.isExpanded
                            checked:            compColumn.isSelected && vehicleConfigView._selectedSectionIndex === -1

                            onClicked: {
                                vehicleConfigView._navigateToComponent(compColumn.index, -1)
                                if (compColumn.hasSections) {
                                    if (compColumn.isSelected && compColumn.isExpanded) {
                                        vehicleConfigView._setExpanded(compColumn.index, false)
                                    } else if (!compColumn.isExpanded) {
                                        vehicleConfigView._setExpanded(compColumn.index, true)
                                    }
                                }
                            }

                            onToggleExpand: {
                                if (!mainWindow.allowViewSwitch()) return
                                var expanding = !compColumn.isExpanded
                                vehicleConfigView._setExpanded(compColumn.index, expanding)
                                if (!expanding && compColumn.isSelected) {
                                    vehicleConfigView._navigateToComponent(compColumn.index, -1)
                                }
                            }
                        }

                        // Section sub-items
                        Repeater {
                            model: compColumn.isExpanded ? compColumn.compSections : []

                            Button {
                                id:             sectionBtn
                                Layout.fillWidth: true
                                padding:        ScreenTools.defaultFontPixelWidth * 0.75
                                leftPadding:    ScreenTools.defaultFontPixelWidth * 3
                                hoverEnabled:   !ScreenTools.isMobile

                                property int sectionIndex: index
                                property bool sectionChecked: compColumn.isSelected && vehicleConfigView._selectedSectionIndex === sectionIndex
                                property bool sectionMatchesSearch: {
                                    if (!compColumn.isSearching) return true
                                    return vehicleConfigView._sectionMatchesSearch(compColumn.comp, modelData)
                                }
                                property bool sectionContentVisible: {
                                    if (!compColumn.isSelected) return true
                                    if (!panelLoader.item) return true
                                    if (typeof panelLoader.item.sectionVisible !== "function") return true
                                    return panelLoader.item.sectionVisible(modelData)
                                }
                                property color textColor: sectionChecked || pressed ? qgcPal.buttonHighlightText : qgcPal.buttonText
                                visible: sectionMatchesSearch && sectionContentVisible

                                background: Rectangle {
                                    color:   qgcPal.buttonHighlight
                                    opacity: sectionBtn.sectionChecked || sectionBtn.pressed ? 1 : sectionBtn.enabled && sectionBtn.hovered ? 0.2 : 0
                                    radius:  ScreenTools.defaultFontPixelWidth / 2
                                }

                                contentItem: RowLayout {
                                    spacing: ScreenTools.defaultFontPixelWidth * 0.5

                                    Rectangle {
                                        width:   ScreenTools.defaultFontPixelWidth
                                        height:  width
                                        radius:  width / 2
                                        color:   compColumn.comp && typeof compColumn.comp.sectionSetupComplete === "function"
                                                     ? (compColumn.comp.sectionSetupComplete(modelData) ? qgcPal.colorGreen : qgcPal.colorOrange)
                                                     : "transparent"
                                        visible: compColumn.comp && typeof compColumn.comp.sectionSetupComplete === "function"
                                                     && !compColumn.comp.sectionSetupComplete(modelData)
                                    }

                                    QGCLabel {
                                        text:  vehicleConfigView._translateSection(compColumn.comp, modelData)
                                        color: sectionBtn.textColor
                                        font.pointSize: ScreenTools.defaultFontPointSize * 0.9
                                        horizontalAlignment: Text.AlignLeft
                                        Layout.fillWidth: true
                                    }
                                }

                                onClicked: {
                                    vehicleConfigView._navigateToComponent(compColumn.index, sectionIndex)
                                }
                            }
                        }
                    }
                }

                // Optical Flow (special)
                ConfigButton {
                    visible:            _activeVehicle ? _activeVehicle.flowImageIndex > 0 : false
                    text:               qsTr("Optical Flow")
                    Layout.fillWidth:   true
                    checked:            vehicleConfigView._selectedSpecial === "opticalflow"
                    onClicked:          showPanel("opticalflow", "qrc:/qml/QGroundControl/VehicleSetup/OpticalFlowSensor.qml")
                }

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: ScreenTools.defaultFontPixelHeight / 2
                    visible: vehicleConfigView._searchQuery.trim() === ""
                }

                ConfigButton {
                    id:                 parametersButton
                    visible:            QGroundControl.multiVehicleManager.parameterReadyVehicleAvailable &&
                                        !_activeVehicle.usingHighLatencyLink &&
                                        _corePlugin.showAdvancedUI &&
                                        vehicleConfigView._searchQuery.trim() === ""
                    text:               qsTr("Parameters")
                    Layout.fillWidth:   true
                    icon.source:        "/qmlimages/subMenuButtonImage.png"
                    checked:            vehicleConfigView._selectedSpecial === "parameters"
                    onClicked:          showPanel("parameters", "qrc:/qml/QGroundControl/VehicleSetup/SetupParameterEditor.qml")
                }

                ConfigButton {
                    id:                 firmwareButton
                    icon.source:        "/qmlimages/FirmwareUpgradeIcon.png"
                    visible:            !ScreenTools.isMobile && _corePlugin.options.showFirmwareUpgrade &&
                                        vehicleConfigView._searchQuery.trim() === ""
                    text:               qsTr("Firmware")
                    Layout.fillWidth:   true
                    checked:            vehicleConfigView._selectedSpecial === "firmware"

                    onClicked: showPanel("firmware", "qrc:/qml/QGroundControl/VehicleSetup/FirmwareUpgrade.qml")
                }
            }
        }
    }

    Rectangle {
        id:                     divider
        anchors.topMargin:      _verticalMargin
        anchors.bottomMargin:   _verticalMargin
        anchors.leftMargin:     _horizontalMargin
        anchors.left:           leftPanel.right
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
        width:                  1
        color:                  qgcPal.windowShade
    }

    Loader {
        id:                     panelLoader
        anchors.topMargin:      _verticalMargin
        anchors.bottomMargin:   _verticalMargin
        anchors.leftMargin:     _horizontalMargin
        anchors.rightMargin:    _horizontalMargin
        anchors.left:           divider.right
        anchors.right:          parent.right
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom

        function setSource(source, vehicleComponent) {
            panelLoader.source = ""
            panelLoader.vehicleComponent = vehicleComponent
            panelLoader.source = source
        }

        function setSourceComponent(sourceComponent, vehicleComponent) {
            panelLoader.sourceComponent = undefined
            panelLoader.vehicleComponent = vehicleComponent
            panelLoader.sourceComponent = sourceComponent
        }

        property var vehicleComponent
    }
}
