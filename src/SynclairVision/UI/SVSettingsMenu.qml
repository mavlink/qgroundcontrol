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

    readonly property real panelMargin: ScreenTools.defaultFontPixelHeight / 2
    readonly property real sectionSpacing: ScreenTools.defaultFontPixelHeight
    readonly property real controlSpacing: ScreenTools.defaultFontPixelHeight / 2
    readonly property real sectionPadding: ScreenTools.defaultFontPixelHeight * 0.75
    readonly property real navigationWidth: ScreenTools.defaultFontPixelWidth * 20
    readonly property real labelColumnWidth: ScreenTools.defaultFontPixelWidth * 16
    readonly property var categoryData: getCategoryData(activeSettingsId)
    readonly property var sectionModel: categoryData.sections ? categoryData.sections : []

    function getCategoryData(settingsId) {
        if (settingsId === 'Controls') {
            return {
                title: 'Controls',
                sections: SVSettingsControls.getSections()
            }
        }

        if (settingsId === 'Dev') {
            return {
                title: 'Dev',
                sections: SVSettingsDev.getSections()
            }
        }

        return {
            title: 'General',
            sections: SVSettingsGeneral.getSections()
        }
    }

    function optionLabels(options) {
        const labels = []

        for (let index = 0; index < options.length; index++) {
            labels.push(options[index].label)
        }

        return labels
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
        const nextContentY = Math.min(Math.max(0, sectionItem.y), maxContentY)

        contentFlickable.contentY = nextContentY
    }

    onActiveSettingsIdChanged: {
        selectedSectionIndex = 0
        contentFlickable.contentY = 0
    }

    QGCPalette { id: qgcPalette }

    Rectangle {
        anchors.fill: parent
        radius: ScreenTools.defaultBorderRadius
        color: qgcPalette.window
        border.width: 1
        border.color: qgcPalette.buttonBorder
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.panelMargin
        spacing: root.sectionSpacing

        QGCLabel {
            Layout.fillWidth: true
            text: root.categoryData.title
            font.pointSize: ScreenTools.largeFontPointSize
            font.bold: true
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: ScreenTools.defaultFontPixelWidth

            Rectangle {
                Layout.preferredWidth: root.navigationWidth
                Layout.fillHeight: true
                radius: ScreenTools.defaultBorderRadius
                color: qgcPalette.windowShade
                border.width: 1
                border.color: qgcPalette.buttonBorder

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: root.sectionPadding
                    spacing: root.controlSpacing

                    Repeater {
                        model: root.sectionModel

                        delegate: QGCButton {
                            Layout.fillWidth: true
                            checkable: true
                            checked: index === root.selectedSectionIndex
                            text: modelData.title
                            horizontalAlignment: Text.AlignLeft
                            wrapMode: Text.Wrap

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

            Flickable {
                id: contentFlickable

                Layout.fillWidth: true
                Layout.fillHeight: true
                boundsBehavior: Flickable.StopAtBounds
                clip: true
                contentWidth: width
                contentHeight: sectionColumn.height

                ScrollBar.vertical: ScrollBar {
                    id: contentScrollBar
                }

                Column {
                    id: sectionColumn

                    width: Math.max(0, contentFlickable.width - contentScrollBar.width - ScreenTools.defaultFontPixelWidth)
                    spacing: root.sectionSpacing

                    Repeater {
                        id: sectionRepeater

                        model: root.sectionModel

                        delegate: Column {
                            width: sectionColumn.width
                            spacing: root.controlSpacing

                            Rectangle {
                                width: parent.width
                                height: sectionTitle.implicitHeight + root.sectionPadding
                                color: 'transparent'

                                QGCLabel {
                                    id: sectionTitle

                                    anchors.left: parent.left
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: modelData.title
                                    font.pointSize: ScreenTools.mediumFontPointSize
                                    font.bold: true
                                }
                            }

                            Rectangle {
                                width: parent.width
                                height: sectionContent.implicitHeight + root.sectionPadding * 2
                                radius: ScreenTools.defaultBorderRadius
                                color: qgcPalette.windowShade
                                border.width: 1
                                border.color: qgcPalette.buttonBorder

                                ColumnLayout {
                                    id: sectionContent

                                    anchors.fill: parent
                                    anchors.margins: root.sectionPadding
                                    spacing: root.controlSpacing

                                    Repeater {
                                        model: modelData.items

                                        delegate: ColumnLayout {
                                            readonly property var settingData: modelData

                                            Layout.fillWidth: true
                                            spacing: root.controlSpacing / 2

                                            QGCCheckBox {
                                                Layout.fillWidth: true
                                                visible: settingData.type === 'checkbox'
                                                text: settingData.label
                                                checked: settingData.checked === true
                                            }

                                            RowLayout {
                                                Layout.fillWidth: true
                                                visible: settingData.type === 'dropdown'
                                                spacing: ScreenTools.defaultFontPixelWidth

                                                QGCLabel {
                                                    Layout.preferredWidth: root.labelColumnWidth
                                                    text: settingData.label
                                                    wrapMode: Text.Wrap
                                                }

                                                QGCComboBox {
                                                    Layout.fillWidth: true
                                                    model: root.optionLabels(settingData.options ? settingData.options : [])
                                                    currentIndex: settingData.currentIndex !== undefined ? settingData.currentIndex : 0
                                                }
                                            }

                                            ColumnLayout {
                                                Layout.fillWidth: true
                                                visible: settingData.type === 'slider'
                                                spacing: root.controlSpacing / 3

                                                RowLayout {
                                                    Layout.fillWidth: true
                                                    spacing: ScreenTools.defaultFontPixelWidth

                                                    QGCLabel {
                                                        Layout.fillWidth: true
                                                        text: settingData.label
                                                        wrapMode: Text.Wrap
                                                    }

                                                    QGCLabel {
                                                        text: root.formatValue(settingData.value)
                                                        color: qgcPalette.text
                                                    }
                                                }

                                                QGCSlider {
                                                    Layout.fillWidth: true
                                                    from: settingData.min !== undefined ? settingData.min : 0
                                                    to: settingData.max !== undefined ? settingData.max : 100
                                                    stepSize: settingData.step !== undefined ? settingData.step : 1
                                                    value: settingData.value !== undefined ? settingData.value : from
                                                }

                                                RowLayout {
                                                    Layout.fillWidth: true

                                                    QGCLabel {
                                                        text: root.formatValue(settingData.min)
                                                        color: qgcPalette.text
                                                    }

                                                    Item {
                                                        Layout.fillWidth: true
                                                    }

                                                    QGCLabel {
                                                        text: root.formatValue(settingData.max)
                                                        color: qgcPalette.text
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
            }
        }
    }
}
