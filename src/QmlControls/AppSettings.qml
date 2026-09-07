import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.AppSettings

Rectangle {
    id:     settingsView
    color:  qgcPal.window
    z:      QGroundControl.zOrderTopMost

    readonly property real _defaultTextHeight:  ScreenTools.defaultFontPixelHeight
    readonly property real _defaultTextWidth:   ScreenTools.defaultFontPixelWidth
    readonly property real _horizontalMargin:   _defaultTextWidth / 2
    readonly property real _verticalMargin:     _defaultTextHeight / 2

    property bool _first: true
    property bool _commingFromRIDSettings: false
    property int  _selectedPageIndex: -1
    property int  _selectedSectionIndex: -1
    property var  _expandedPages: ({})  // pageIndex -> bool
    property int  _expandedRevision: 0  // bumped to trigger re-evaluation
    property string _searchQuery: ""

    function _setExpanded(pageIndex, value) {
        _expandedPages[pageIndex] = value
        _expandedRevision++
    }

    function _isExpanded(pageIndex) {
        void _expandedRevision  // create binding dependency
        return !!_expandedPages[pageIndex]
    }

    function _pageSections(entry) {
        return entry && typeof entry.sections === "function" ? entry.sections() : []
    }

    function _pageAvailable(entry) {
        if (!entry || entry.name === "Divider" ||
                (typeof entry.pageVisible === "function" && !entry.pageVisible())) {
            return false
        }

        var sections = _pageSections(entry)
        return sections.length === 0 || sections.some(function(section) { return section.visible })
    }

    function _sectionAvailable(sections, sectionIndex) {
        if (sectionIndex === -1) return true
        for (var i = 0; i < sections.length; i++) {
            if (sections[i].index === sectionIndex) return sections[i].visible
        }
        return false
    }

    // Search: returns array of matching section indices for a page, or empty if no match
    function _matchingSections(pageIndex) {
        var query = _searchQuery.toLowerCase().trim()
        if (query === "") return []  // empty = no filtering

        var entry = settingsPagesModel.get(pageIndex)
        if (!_pageAvailable(entry)) return []

        var sections = _pageSections(entry)
        var matches = []
        for (var i = 0; i < sections.length; i++) {
            if (!sections[i].visible) continue
            for (var j = 0; j < sections[i].searchTerms.length; j++) {
                if (sections[i].searchTerms[j].indexOf(query) !== -1) {
                    matches.push(sections[i].index)
                    break
                }
            }
        }
        return matches
    }

    // Does this page have any search matches? (or is search empty = show all)
    function _pageMatchesSearch(pageIndex) {
        if (_searchQuery.trim() === "") return true
        return _matchingSections(pageIndex).length > 0
    }

    function _navigateTo(pageIndex, sectionIndex) {
        var entry = settingsPagesModel.get(pageIndex)
        if (!entry || entry.name === "Divider") return
        if (!_pageAvailable(entry)) return
        if (!_sectionAvailable(_pageSections(entry), sectionIndex)) return

        var url = entry.url
        _selectedSectionIndex = sectionIndex

        if (_selectedPageIndex !== pageIndex) {
            _selectedPageIndex = pageIndex
            rightPanel.source = url
        }

        // Apply section filter after the page is loaded
        if (rightPanel.item && typeof rightPanel.item.sectionFilter !== "undefined") {
            rightPanel.item.sectionFilter = sectionIndex
        }
    }

    function _navigateToFirstAvailablePage() {
        for (var i = 0; i < settingsPagesModel.count; i++) {
            if (_pageAvailable(settingsPagesModel.get(i))) {
                _navigateTo(i, -1)
                return
            }
        }

        _selectedPageIndex = -1
        _selectedSectionIndex = -1
        rightPanel.source = ""
    }

    // settingsPage is the untranslated page name from SettingsPages.json
    function showSettingsPage(settingsPage) {
        for (var i = 0; i < settingsPagesModel.count; i++) {
            var entry = settingsPagesModel.get(i)
            if (entry && entry.nameKey === settingsPage) {
                _navigateTo(i, -1)
                break
            }
        }
    }

    // This need to block click event leakage to underlying map.
    DeadMouseArea {
        anchors.fill: parent
    }

    QGCPalette { id: qgcPal }

    Component.onCompleted: {
        if (globals.commingFromRIDIndicator) {
            globals.commingFromRIDIndicator = false
            showSettingsPage("Remote ID")
        }

        if (_selectedPageIndex === -1) {
            _navigateToFirstAvailablePage()
        }
    }

    Connections {
        target: rightPanel
        function onLoaded() {
            if (rightPanel.item && typeof rightPanel.item.sectionFilter !== "undefined") {
                rightPanel.item.sectionFilter = _selectedSectionIndex
            }
        }
    }

    SettingsPagesModel { id: settingsPagesModel }

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
            objectName:         "settings_searchField"
            Layout.fillWidth:   true
            placeholderText:    qsTr("Search settings...")

            onTextChanged: {
                settingsView._searchQuery = text
            }
        }

        QGCFlickable {
            id:                 buttonList
            objectName:         "settings_buttonList"
            Layout.fillWidth:   true
            Layout.fillHeight:  true
            contentHeight:      buttonColumn.height + _verticalMargin
            flickableDirection:  Flickable.VerticalFlick
            clip:               true

        ColumnLayout {
            id:         buttonColumn
            width:      buttonList.width
            spacing:    0

            Repeater {
                id:     buttonRepeater
                model:  settingsPagesModel

                ColumnLayout {
                    id:     pageColumn
                    spacing: 0
                    Layout.fillWidth: true

                    required property int index
                    required property var model

                    property string pageName:    model.name ?? ""
                    property string pageUrl:     model.url ?? ""
                    property string pageIconUrl: model.iconUrl ?? ""
                    property var    pageVisible: model.pageVisible ?? function() { return true }
                    property var    pageSections: pageVisible() ? settingsView._pageSections(model) : []
                    property var    visiblePageSections: pageSections.filter(function(section) {
                        return section.visible
                    })
                    property bool pageAvailable: pageVisible() &&
                                                 (pageSections.length === 0 || visiblePageSections.length > 0)
                    property bool isSelected: settingsView._selectedPageIndex === index
                    property bool hasMultipleSections: visiblePageSections.length > 1
                    property bool isSearching: settingsView._searchQuery.trim() !== ""
                    property bool matchesSearch: pageAvailable && settingsView._pageMatchesSearch(index)
                    property bool isExpanded: hasMultipleSections && (isSearching ? matchesSearch : settingsView._isExpanded(index))

                    onPageSectionsChanged: {
                        let sections = pageSections ?? []
                        let visibleCount = sections.filter(function(section) { return section.visible }).length
                        if (isSelected && pageAvailable && settingsView._selectedSectionIndex !== -1 &&
                                (visibleCount <= 1 ||
                                 !settingsView._sectionAvailable(sections, settingsView._selectedSectionIndex))) {
                            settingsView._navigateTo(index, -1)
                        }
                    }

                    onPageAvailableChanged: {
                        if (isSelected && !pageAvailable) {
                            settingsView._navigateToFirstAvailablePage()
                        }
                    }

                    visible: {
                        if (pageName === "Divider") return !isSearching
                        if (!pageAvailable) return false
                        if (isSearching) return matchesSearch
                        return true
                    }

                    // Divider
                    Item {
                        Layout.fillWidth: true
                        height: ScreenTools.defaultFontPixelHeight / 2
                        visible: pageName === "Divider"
                    }

                    // Page button
                    SettingsButton {
                        Layout.fillWidth: true
                        objectName:    "settingsButton_" + (model.nameKey ?? pageName)
                        text:          pageName
                        icon.source:   pageIconUrl
                        expandable:    hasMultipleSections
                        expanded:      isExpanded
                        checked:       isSelected && settingsView._selectedSectionIndex === -1
                        visible:       pageName !== "Divider" && pageAvailable

                        onClicked: {
                            if (mainWindow.allowViewSwitch()) {
                                settingsView._navigateTo(index, -1)
                                if (hasMultipleSections) {
                                    // Toggle expand/collapse when re-clicking the same page
                                    if (isSelected && isExpanded) {
                                        settingsView._setExpanded(index, false)
                                    } else if (!isExpanded) {
                                        settingsView._setExpanded(index, true)
                                    }
                                }
                            }
                        }

                        onToggleExpand: {
                            if (!mainWindow.allowViewSwitch()) {
                                return
                            }
                            var expanding = !isExpanded
                            settingsView._setExpanded(index, expanding)
                            if (!expanding && isSelected) {
                                settingsView._navigateTo(index, -1)
                            }
                        }
                    }

                    // Section sub-items (indented, shown when page is expanded)
                    Repeater {
                        model: isExpanded ? visiblePageSections : []

                        Button {
                            id:             sectionBtn
                            Layout.fillWidth: true
                            padding:        ScreenTools.defaultFontPixelWidth * 0.75
                            leftPadding:    ScreenTools.defaultFontPixelWidth * 3
                            hoverEnabled:   !ScreenTools.isMobile

                            property int sectionIndex: modelData.index
                            property bool sectionChecked: pageColumn.isSelected && settingsView._selectedSectionIndex === sectionIndex
                            property bool sectionMatchesSearch: {
                                if (!pageColumn.isSearching) return true
                                var matches = settingsView._matchingSections(pageColumn.index)
                                return matches.indexOf(sectionIndex) !== -1
                            }
                            property color textColor: sectionChecked || pressed ? qgcPal.buttonHighlightText : qgcPal.buttonText
                            visible: sectionMatchesSearch

                            background: Rectangle {
                                color:   qgcPal.buttonHighlight
                                opacity: sectionBtn.sectionChecked || sectionBtn.pressed ? 1 : sectionBtn.enabled && sectionBtn.hovered ? 0.2 : 0
                                radius:  ScreenTools.defaultFontPixelWidth / 2
                            }

                            contentItem: QGCLabel {
                                text:  modelData.name
                                color: sectionBtn.textColor
                                font.pointSize: ScreenTools.defaultFontPointSize * 0.9
                                horizontalAlignment: Text.AlignLeft
                            }

                            onClicked: {
                                if (mainWindow.allowViewSwitch()) {
                                    settingsView._navigateTo(pageColumn.index, sectionIndex)
                                }
                            }
                        }
                    }
                }
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

    //-- Panel Contents
    Loader {
        id:                     rightPanel
        objectName:             "settings_rightPanel"
        anchors.leftMargin:     _horizontalMargin
        anchors.rightMargin:    _horizontalMargin
        anchors.topMargin:      _verticalMargin
        anchors.bottomMargin:   _verticalMargin
        anchors.left:           divider.right
        anchors.right:          parent.right
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
    }
}
