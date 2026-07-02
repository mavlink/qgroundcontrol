import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

Item {
    id: root

    readonly property int vertical: 0
    readonly property int horizontal: 1

    property bool open: false
    property bool headerless: false
    property bool exclusiveSelection: true
    property bool autoUpdateActiveId: true
    property int direction: vertical

    property url source
    property string menuText: ""
    property var model: []
    property string activeId: ""
    property var activeIds: []

    property real size: ScreenTools.defaultFontPixelWidth * 7
    property real buttonSize: size - (menuMargin * 2)
    property real borderRadius: ScreenTools.defaultBorderRadius
    property real menuMargin: ScreenTools.defaultFontPixelWidth * 0.4
    property real separatorThickness: 1
    property real separatorMargin: ScreenTools.defaultFontPixelHeight * 0.25

    readonly property bool isVertical: direction === vertical
    readonly property bool hasContent: !!model && model.length > 0
    readonly property bool headerVisible: !root.headerless
    readonly property bool contentVisible: hasContent && (headerless || open)
    readonly property bool separatorVisible: headerVisible && contentVisible

    signal itemSelected(string id)

    width: isVertical
           ? size
           : menuMargin * 2 + (headerVisible ? activeHeaderButton.width : 0)
             + (separatorVisible ? separatorThickness + separatorMargin * 2 : 0)
             + (contentVisible ? activeContentGrid.implicitWidth : 0)

    height: isVertical
            ? menuMargin * 2 + (headerVisible ? activeHeaderButton.height : 0)
              + (separatorVisible ? separatorThickness + separatorMargin * 2 : 0)
              + (contentVisible ? activeContentGrid.implicitHeight : 0)
            : size

    QGCPalette { id: qgcPalette }

    Rectangle {
        id: background
        anchors.fill: parent
        color: qgcPalette.windowTransparent
        radius: root.borderRadius
    }

    Column {
        id: verticalLayout
        visible: root.isVertical
        anchors.fill: parent
        anchors.margins: menuMargin
        spacing: 0

        SVMenuStripButton {
            id: headerButton
            visible: root.headerVisible
            size: root.buttonSize
            borderRadius: root.borderRadius
            text: root.menuText
            iconSource: root.source
            checked: root.open          // was: checked: false
            extendHeader: true          // ← add
            expanded: root.open         // ← add
            onClicked: root.open = !root.open
        }

        Item {
            visible: root.separatorVisible
            width: root.buttonSize
            height: root.separatorThickness + root.separatorMargin * 2

            Rectangle {
                anchors.centerIn: parent
                width: parent.width * 0.8
                height: 1
                color: qgcPalette.windowShadeLight
            }
        }

        Grid {
            id: contentGridVertical
            visible: root.contentVisible
            columns: 1
            spacing: 0

            Repeater {
                model: root.model

                delegate: SVMenuStripButton {
                    readonly property bool itemChecked: !!modelData.checkable
                                                        && modelData.id !== undefined
                                                        && (root.exclusiveSelection
                                                            ? modelData.id === root.activeId
                                                            : root.activeIds.indexOf(modelData.id) !== -1)

                    readonly property bool itemIconActive: modelData.iconActive !== undefined
                                                        ? modelData.iconActive
                                                        : itemChecked

                    size: root.buttonSize
                    borderRadius: root.borderRadius
                    text: modelData.text ? modelData.text : ""
                    checked: itemChecked
                    iconSource: itemIconActive && modelData.alternateIconSource
                                ? modelData.alternateIconSource
                                : (modelData.iconSource ? modelData.iconSource : "")

                    onClicked: {
                        if (modelData.checkable && root.autoUpdateActiveId && root.exclusiveSelection) {
                            root.activeId = modelData.id
                        }
                        root.itemSelected(modelData.id)
                    }
                }
            }
        }
    }

    Row {
        id: horizontalLayout
        visible: !root.isVertical
        anchors.fill: parent
        anchors.margins: menuMargin
        spacing: 0

        SVMenuStripButton {
            id: headerButtonHorizontal
            visible: root.headerVisible
            size: root.buttonSize
            borderRadius: root.borderRadius
            text: root.menuText
            iconSource: root.source
            checked: root.open
            extendHeader: true           // ← add
            expanded: root.open          // ← add
            onClicked: root.open = !root.open
        }

        Item {
            visible: root.separatorVisible
            width: root.separatorThickness + root.separatorMargin * 2
            height: root.buttonSize

            Rectangle {
                anchors.centerIn: parent
                width: root.separatorThickness
                height: parent.height
                color: qgcPalette.windowShadeLight
            }
        }

        Grid {
            id: contentGridHorizontal
            visible: root.contentVisible
            rows: 1
            spacing: 0

            Repeater {
                model: root.model

                delegate: SVMenuStripButton {
                    readonly property bool itemChecked: !!modelData.checkable
                                                        && modelData.id !== undefined
                                                        && (root.exclusiveSelection
                                                            ? modelData.id === root.activeId
                                                            : root.activeIds.indexOf(modelData.id) !== -1)

                    readonly property bool itemIconActive: modelData.iconActive !== undefined
                                                        ? modelData.iconActive
                                                        : itemChecked

                    size: root.buttonSize
                    borderRadius: root.borderRadius
                    text: modelData.text ? modelData.text : ""
                    checked: itemChecked
                    iconSource: itemIconActive && modelData.alternateIconSource
                                ? modelData.alternateIconSource
                                : (modelData.iconSource ? modelData.iconSource : "")

                    onClicked: {
                        if (modelData.checkable && root.autoUpdateActiveId && root.exclusiveSelection) {
                            root.activeId = modelData.id
                        }
                        root.itemSelected(modelData.id)
                    }
                }
            }
        }
    }

    readonly property var activeContentGrid: isVertical ? contentGridVertical : contentGridHorizontal
    readonly property var activeHeaderButton: isVertical ? headerButton : headerButtonHorizontal
}