import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

import "SVSettingsControls.js" as SVSettingsControls
import "SVSettingsDev.js" as SVSettingsDev
import "SVSettingsGeneral.js" as SVSettingsGeneral

Item {
    id: root

    property string activeSettingsId: ""
    property int selectedSectionIndex: 0
    property int pendingProgrammaticSectionIndex: -1

    readonly property real panelMargin: ScreenTools.defaultFontPixelHeight / 2
    readonly property real sectionSpacing: ScreenTools.defaultFontPixelHeight / 2
    readonly property real controlSpacing: ScreenTools.defaultFontPixelHeight / 2
    readonly property real sectionScrollTopMargin: ScreenTools.defaultFontPixelHeight * 1.5
    readonly property real topContentPadding: 1
    readonly property real bottomContentPadding: 1
    readonly property real sectionPadding: ScreenTools.defaultFontPixelHeight * 0.75
    readonly property real navigationWidth: ScreenTools.defaultFontPixelWidth * 25
    readonly property real labelColumnWidth: ScreenTools.defaultFontPixelWidth * 18
    readonly property real controlColumnWidth: ScreenTools.defaultFontPixelWidth * 18
    readonly property real edgeGradientHeight: ScreenTools.defaultFontPixelHeight * 1.5
    readonly property real settingRowVerticalPadding: ScreenTools.defaultFontPixelHeight * 0.45
    readonly property real settingColumnSpacing: ScreenTools.defaultFontPixelWidth * 1.5
    readonly property real settingTextSpacing: ScreenTools.defaultFontPixelHeight * 0.15
    readonly property color edgeGradientColor: Qt.alpha(qgcPalette.window, 0.96)
    readonly property real maxScrollContentY: Math.max(0, contentFlickable.contentHeight - contentFlickable.height)
    readonly property real topEdgeGradientOpacity: Math.min(1, Math.max(0, contentFlickable.contentY) / edgeGradientHeight)
    readonly property real bottomHiddenContent: Math.max(0, maxScrollContentY - contentFlickable.contentY)
    readonly property real bottomEdgeGradientOpacity: Math.min(1, bottomHiddenContent / edgeGradientHeight)
    readonly property var categoryData: getCategoryData(activeSettingsId)
    readonly property var sectionModel: categoryData.sections ? categoryData.sections : []
        function getCategoryData(settingsId) {
        if (settingsId === 'General') {
            return { title: 'General', sections: SVSettingsGeneral.getSections() }
        } else if (settingsId === 'Controls') {
            return { title: 'Controls', sections: SVSettingsControls.getSections() }
        } else if (settingsId === 'Dev') {
            return { title: 'Dev', sections: SVSettingsDev.getSections() }
        }
    }

    function optionLabels(options) {
        const labels = []

        for (let index = 0; index < options.length; index++) {
            labels.push(options[index].label)
        }

        return labels
    }

    function useControlsSettingBridge(settingData) {
        return activeSettingsId === 'Controls'
            && settingData.property !== undefined
            && SVSettings[settingData.property] !== undefined
    }

    function settingValue(settingData, fallbackValue) {
        if (useControlsSettingBridge(settingData)) {
            return SVSettings[settingData.property]
        }

        return fallbackValue
    }

    function setSettingValue(settingData, value) {
        if (!useControlsSettingBridge(settingData)) {
            return
        }

        SVSettings[settingData.property] = value
    }

    function dropdownCurrentIndex(settingData) {
        const options = settingData.options ? settingData.options : []

        if (useControlsSettingBridge(settingData)) {
            const currentValue = SVSettings[settingData.property]

            for (let index = 0; index < options.length; index++) {
                if (options[index].value === currentValue) {
                    return index
                }
            }
        }

        return settingData.currentIndex !== undefined ? settingData.currentIndex : 0
    }

    function setDropdownSettingValue(settingData, currentIndex) {
        const options = settingData.options ? settingData.options : []

        if (currentIndex < 0 || currentIndex >= options.length) {
            return
        }

        setSettingValue(settingData, options[currentIndex].value)
    }

    function matchesCondition(conditionData) {
        if (!conditionData
                || conditionData.property === undefined
                || conditionData.equals === undefined) {
            return true
        }

        const currentValue = settingValue({ property: conditionData.property }, undefined)

        return currentValue === undefined || currentValue === conditionData.equals
    }

    function isSettingVisible(settingData) {
        return matchesCondition(settingData.visibleWhen)
    }

    function hasVisibleSettingBefore(items, startIndex) {
        if (!items) {
            return false
        }

        for (let settingIndex = 0; settingIndex < startIndex; settingIndex++) {
            if (isSettingVisible(items[settingIndex])) {
                return true
            }
        }

        return false
    }

    function hasVisibleSettingAfter(items, startIndex) {
        if (!items) {
            return false
        }

        for (let settingIndex = startIndex + 1; settingIndex < items.length; settingIndex++) {
            if (isSettingVisible(items[settingIndex])) {
                return true
            }
        }

        return false
    }

    function isSectionEnabled(sectionData) {
        return matchesCondition(sectionData ? sectionData.enabledWhen : undefined)
    }

    function isSettingEnabled(settingData) {
        return matchesCondition(settingData ? settingData.enabledWhen : undefined)
    }

    function sliderDisplayValue(settingData, sliderValue) {
        if (useControlsSettingBridge(settingData)) {
            return SVSettings[settingData.property]
        }

        return sliderValue
    }

    function formatValue(value) {
        if (typeof value !== 'number') {
            return value === undefined ? '' : value.toString()
        }

        return Math.abs(value - Math.round(value)) < 0.001
            ? Math.round(value).toString()
            : value.toFixed(2)
    }

    function scrollToSection(sectionIndex) {
        const sectionItem = sectionRepeater.itemAt(sectionIndex)

        if (!sectionItem) {
            return
        }

        selectedSectionIndex = sectionIndex

        const maxContentY = Math.max(0, contentFlickable.contentHeight - contentFlickable.height)
        const nextContentY = Math.min(Math.max(0, sectionItem.y - root.sectionScrollTopMargin), maxContentY)
        const isProgrammaticScroll = Math.abs(contentFlickable.contentY - nextContentY) > 1

        pendingProgrammaticSectionIndex = isProgrammaticScroll ? sectionIndex : -1

        contentFlickable.contentY = nextContentY
    }

    function updateSelectedSectionFromScroll() {
        const sectionCount = root.sectionModel.length

        if (sectionCount === 0) {
            selectedSectionIndex = 0
            return
        }

        const maxContentY = Math.max(0, contentFlickable.contentHeight - contentFlickable.height)

        if (maxContentY <= 0) {
            return
        }

        if (pendingProgrammaticSectionIndex >= 0) {
            const pendingSectionIndex = pendingProgrammaticSectionIndex

            pendingProgrammaticSectionIndex = -1

            if (contentFlickable.contentY >= maxContentY - 1
                    && pendingSectionIndex !== sectionCount - 1) {
                return
            }
        }

        if (contentFlickable.contentY >= maxContentY - 1) {
            selectedSectionIndex = sectionCount - 1
            return
        }

        const scrollTopReferenceY = contentFlickable.contentY + root.sectionScrollTopMargin
        let nextSelectedSectionIndex = 0

        for (let sectionIndex = 0; sectionIndex < sectionCount; sectionIndex++) {
            const nextSectionItem = sectionRepeater.itemAt(sectionIndex + 1)

            if (!nextSectionItem || scrollTopReferenceY < nextSectionItem.y) {
                nextSelectedSectionIndex = sectionIndex
                break
            }
        }

        if (selectedSectionIndex !== nextSelectedSectionIndex) {
            selectedSectionIndex = nextSelectedSectionIndex
        }
    }

    onActiveSettingsIdChanged: {
        selectedSectionIndex = 0
        pendingProgrammaticSectionIndex = -1
        contentFlickable.contentY = 0
    }

    QGCPalette { id: qgcPalette }

    Rectangle {
        anchors.fill: parent
        radius: ScreenTools.defaultBorderRadius * 2
        color: qgcPalette.window
    }

    

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: ScreenTools.defaultFontPixelWidth * 1.7

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: ScreenTools.defaultFontPixelWidth * 1.7

            Rectangle {
                Layout.preferredWidth: root.navigationWidth
                Layout.fillHeight: true
                color: "transparent"

                ColumnLayout {
                    anchors.fill: parent

                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    //anchors.leftMargin: 2
                    //anchors.rightMargin: 2
                    //anchors.margins: root.sectionPadding

                    Rectangle {
                        Layout.fillWidth: true
                        height: ScreenTools.defaultFontPixelWidth * 7 - 5
                        color: "transparent"

                        QGCLabel {
                            anchors.top: parent.top
                            anchors.topMargin: parent.height / 4
                            //anchors.verticalCenter: parent.verticalCenter
                            anchors.horizontalCenter: parent.horizontalCenter
                            //anchors.left: parent.left
                            //anchors.leftMargin: ScreenTools.defaultFontPixelWidth

                            text: root.categoryData.title
                            font.pointSize: ScreenTools.largeFontPointSize
                            
                        }
                    }
                    
                    Repeater {
                        model: root.sectionModel

                        delegate: Button {
                            id: button
                            readonly property bool isSelected: root.selectedSectionIndex === index

                            width: ScreenTools.defaultFontPixelHeight * 7
                            height: ScreenTools.defaultFontPixelWidth * 7
                            Layout.fillWidth: true
                            padding: ScreenTools.defaultFontPixelWidth * 1.3
                            hoverEnabled: !ScreenTools.isMobile
                            text: modelData.title

                            background: Rectangle {
                                radius: ScreenTools.defaultBorderRadius
                                color: button.isSelected
                                    ? qgcPalette.buttonHighlight
                                    : (button.hovered ? qgcPalette.toolStripHoverColor : "transparent")
                            }

                            Rectangle {
                                anchors.fill: parent
                                anchors.margins: ScreenTools.defaultFontPixelWidth * 0.25 / 2
                                //anchors.leftMargin: 0
                                color: "transparent"
                                border.width: 1
                                border.color: qgcPalette.window
                                radius: 3
                                visible: button.isSelected
                            }

                            contentItem: QGCLabel {
                                text: button.text
                                color: button.isSelected ? qgcPalette.buttonHighlightText : qgcPalette.buttonText
                                horizontalAlignment: Text.AlignLeft
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                                leftPadding: ScreenTools.defaultFontPixelWidth * 0.5
                            }

                            onClicked: {
                                root.scrollToSection(index)
                            }
                        }
                    }

                    Item {
                        Layout.fillHeight: true
                    }
                }

                
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Flickable {
                    id: contentFlickable
                    readonly property real scrollBarGutterWidth: Math.max(contentScrollBar.implicitWidth, ScreenTools.defaultFontPixelWidth)

                    anchors.fill: parent
                    anchors.bottomMargin: 4
                    anchors.topMargin: 4
                    boundsBehavior: Flickable.StopAtBounds
                    clip: true
                    contentWidth: width
                    contentHeight: sectionColumn.height

                    onContentYChanged: root.updateSelectedSectionFromScroll()

                    ScrollBar.vertical: ScrollBar {
                        id: contentScrollBar
                    }

                    Column {
                        id: sectionColumn

                        width: Math.max(0, contentFlickable.width - contentFlickable.scrollBarGutterWidth - ScreenTools.defaultFontPixelWidth)
                        spacing: root.sectionSpacing * 2

                        Item {
                            width: parent.width
                            height: root.topContentPadding
                        }

                        Repeater {
                            id: sectionRepeater


                            model: root.sectionModel

                            delegate: Column {
                                readonly property var sectionData: modelData
                                readonly property bool sectionEnabled: root.isSectionEnabled(sectionData)

                                width: sectionColumn.width
                                spacing: root.controlSpacing / 2
                                enabled: sectionEnabled
                                opacity: sectionEnabled ? 1 : 0.5

                                Rectangle {
                                    width: parent.width
                                    height: sectionTitle.implicitHeight // + root.sectionPadding
                                    color: 'transparent'

                                    QGCLabel {
                                        id: sectionTitle

                                        anchors.left: parent.left
                                        anchors.leftMargin: ScreenTools.defaultFontPixelHeight / 2
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: modelData.title
                                        //font.bold: true
                                    }
                                }

                                Rectangle {
                                    width: parent.width
                                    height: sectionContent.implicitHeight + root.sectionPadding * 2
                                    radius: ScreenTools.defaultBorderRadius * 3
                                    color: qgcPalette.window
                                    border.width: 1
                                    border.color: qgcPalette.windowShadeLight

                                    ColumnLayout {
                                        id: sectionContent

                                        anchors.fill: parent
                                        anchors.margins: root.sectionPadding / 1.5
                                        spacing: 0

                                        Repeater {
                                            id: settingRepeater

                                            model: sectionData.items

                                            delegate: ColumnLayout {
                                                readonly property var settingData: modelData
                                                readonly property bool settingEnabled: root.isSettingEnabled(settingData)

                                                Layout.fillWidth: true
                                                spacing: 0
                                                visible: root.isSettingVisible(settingData)
                                                enabled: settingEnabled
                                                opacity: settingEnabled ? 1 : 0.5

                                                Loader {
                                                    id: settingControlLoader

                                                    Layout.fillWidth: true
                                                    Layout.topMargin: root.hasVisibleSettingBefore(sectionData.items, index)
                                                        ? root.settingRowVerticalPadding
                                                        : 0
                                                    Layout.bottomMargin: root.hasVisibleSettingAfter(sectionData.items, index)
                                                        ? (settingDescription.visible
                                                            ? root.settingTextSpacing
                                                            : root.settingRowVerticalPadding)
                                                        : 0
                                                    sourceComponent: {
                                                        if (settingData.type === 'checkbox') {
                                                            return checkboxComponent
                                                         }

                                                        if (settingData.type === 'dropdown') {
                                                            return dropdownComponent
                                                        }

                                                        if (settingData.type === 'slider') {
                                                            return sliderComponent
                                                        }

                                                        return null
                                                    }
                                                }

                                                QGCLabel {
                                                    id: settingDescription

                                                    Layout.fillWidth: true
                                                    Layout.bottomMargin: visible ? root.settingRowVerticalPadding : 0
                                                    Layout.topMargin: 5
                                                    text: settingData.description ? settingData.description : ''
                                                    visible: text !== ''
                                                    wrapMode: Text.WordWrap
                                                    font.pointSize: ScreenTools.smallFontPointSize
                                                    color: qgcPalette.buttonText
                                                }

                                                Rectangle {
                                                    Layout.fillWidth: true
                                                    //Layout.topMargin: root.controlSpacing / 2
                                                    //Layout.bottomMargin: root.controlSpacing / 2
                                                    height: 1
                                                    color: qgcPalette.windowShadeLight
                                                    visible: root.hasVisibleSettingAfter(sectionData.items, index)
                                                }

                                                Component {
                                                    id: checkboxComponent

                                                    RowLayout {
                                                        width: settingControlLoader.width
                                                        spacing: root.settingColumnSpacing

                                                        QGCCheckBox {
                                                            Layout.alignment: Qt.AlignTop
                                                            checked: root.settingValue(settingData, settingData.checked === true)
                                                            text: ''

                                                            onClicked: root.setSettingValue(settingData, checked)
                                                        }

                                                        ColumnLayout {
                                                            Layout.fillWidth: true
                                                            spacing: 0

                                                            QGCLabel {
                                                                Layout.fillWidth: true
                                                                text: settingData.label
                                                                wrapMode: Text.WordWrap
                                                            }
                                                        }
                                                    }
                                                }

                                                Component {
                                                    id: dropdownComponent

                                                    RowLayout {
                                                        width: settingControlLoader.width
                                                        spacing: root.settingColumnSpacing

                                                        ColumnLayout {
                                                            Layout.fillWidth: true
                                                            Layout.preferredWidth: root.labelColumnWidth
                                                            spacing: 0

                                                            QGCLabel {
                                                                Layout.fillWidth: true
                                                                text: settingData.label
                                                                wrapMode: Text.WordWrap
                                                            }
                                                        }

                                                        QGCComboBox {
                                                            Layout.alignment: Qt.AlignTop
                                                            Layout.preferredWidth: root.controlColumnWidth
                                                            model: root.optionLabels(settingData.options ? settingData.options : [])
                                                            currentIndex: root.dropdownCurrentIndex(settingData)

                                                            onActivated: root.setDropdownSettingValue(settingData, currentIndex)
                                                        }
                                                    }
                                                }

                                                Component {
                                                    id: sliderComponent

                                                    ColumnLayout {
                                                        width: settingControlLoader.width
                                                        spacing: root.controlSpacing / 3

                                                        RowLayout {
                                                            Layout.fillWidth: true
                                                            spacing: root.settingColumnSpacing

                                                            ColumnLayout {
                                                                Layout.fillWidth: true
                                                                spacing: 0

                                                                QGCLabel {
                                                                    Layout.fillWidth: true
                                                                    text: settingData.label
                                                                    wrapMode: Text.WordWrap
                                                                }
                                                            }

                                                            QGCLabel {
                                                                Layout.alignment: Qt.AlignTop
                                                                text: root.formatValue(root.sliderDisplayValue(settingData, sliderControl.value))
                                                                color: qgcPalette.text
                                                            }
                                                        }

                                                        QGCSlider {
                                                            id: sliderControl

                                                            Layout.fillWidth: true
                                                            from: settingData.min !== undefined ? settingData.min : 0
                                                            to: settingData.max !== undefined ? settingData.max : 100
                                                            stepSize: settingData.step !== undefined ? settingData.step : 1
                                                            value: root.settingValue(settingData, settingData.value !== undefined ? settingData.value : from)

                                                            onMoved: root.setSettingValue(settingData, value)
                                                        }

                                                        RowLayout {
                                                            Layout.fillWidth: true

                                                            QGCLabel {
                                                                text: root.formatValue(settingData.min)
                                                                font.pointSize: ScreenTools.smallFontPointSize
                                                                color: qgcPalette.buttonText
                                                            }

                                                            Item {
                                                                Layout.fillWidth: true
                                                            }

                                                            QGCLabel {
                                                                text: root.formatValue(settingData.max)
                                                                font.pointSize: ScreenTools.smallFontPointSize
                                                                color: qgcPalette.buttonText
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        Item {
                            width: parent.width
                            height: root.bottomContentPadding
                        }
                    }
                }

                Rectangle {
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 2
                    anchors.leftMargin: 0
                    anchors.rightMargin: 5
                    height: root.edgeGradientHeight
                    color: "transparent"
                    opacity: root.topEdgeGradientOpacity
                    visible: opacity > 0.01
                    z: 1

                    gradient: Gradient {
                        GradientStop { position: 0.0; color: root.edgeGradientColor }
                        GradientStop { position: 1.0; color: "transparent" }
                    }
                }

                Rectangle {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 2
                    anchors.leftMargin: 0
                    anchors.rightMargin: 5
                    height: root.edgeGradientHeight
                    color: "transparent"
                    opacity: root.bottomEdgeGradientOpacity
                    visible: opacity > 0.01
                    z: 1

                    gradient: Gradient {
                        GradientStop { position: 0.0; color: "transparent" }
                        GradientStop { position: 1.0; color: root.edgeGradientColor }
                    }
                }
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: ScreenTools.defaultBorderRadius * 2
        color: "transparent"
        border.width: 1
        border.color: qgcPalette.windowShadeLight
    }
}
