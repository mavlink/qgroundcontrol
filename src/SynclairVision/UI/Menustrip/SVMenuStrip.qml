import QtQuick
import QGroundControl
import QGroundControl.Controls

Item {
    id: root

    readonly property string vertical: 'vertical'
    readonly property string horizontal: 'horizontal'

    property bool open: false
    property bool headerless: false
    property bool exclusiveSelection: true
    property bool autoUpdateActiveId: true
    
    property string direction: vertical

    property url source
    property url alternateSource
    property string menuText: ""
    property var model: []
    property string activeId: ""
    property var activeIds: []

    property real buttonSize: Math.min(SVUnits.objectWidth - SVUnits.bigMargin, ScreenTools.implicitButtonHeight + (SVUnits.bigMargin * 2))

    readonly property bool isHorizontal: direction === horizontal
    readonly property bool isVertical: !isHorizontal

    readonly property bool hasContent: !!model && model.length > 0
    readonly property bool contentVisible: hasContent && (headerless || open)
    readonly property bool separatorVisible: !root.headerless && contentVisible

    readonly property bool useAlternateHeaderIcon: root.isVertical && (root.alternateSource.toString() !== "") && root.open
    readonly property int gridItemCount: Math.max(hasContent ? model.length : 0, 1)
    readonly property real separatorSpan: SVUnits.lineWidth + SVUnits.height / 2

    signal itemSelected(string id)

    function isItemChecked(item) {
        if (!item || !item.checkable || (item.id === undefined)) {
            return false
        }

        if (root.exclusiveSelection) {
            return item.id === root.activeId
        }

        return root.activeIds.indexOf(item.id) !== -1
    }

    width: isVertical
           ? SVUnits.objectWidth
           : SVUnits.bigMargin
             + (root.headerless ? 0 : root.buttonSize)
             + (root.separatorVisible ? root.separatorSpan : 0)
             + (root.contentVisible ? contentGrid.implicitWidth : 0)

    height: isHorizontal
             ? SVUnits.objectWidth
             : SVUnits.bigMargin
               + (root.headerless ? 0 : root.buttonSize)
               + (root.separatorVisible ? root.separatorSpan : 0)
               + (root.contentVisible ? contentGrid.implicitHeight : 0)

    QGCPalette { id: qgcPalette }

    Component {
        id: headerButtonComponent

        SVMenuStripButton {
            size: root.buttonSize
            borderRadius: SVUnits.radius
            text: root.menuText
            iconSource: root.useAlternateHeaderIcon ? root.alternateSource : root.source
            checked: root.open
            extendHeader: true
            expanded: root.open
            onClicked: root.open = !root.open
        }
    }

    Component {
        id: menuButtonDelegate

        SVMenuStripButton {
            readonly property bool itemIconActive: modelData && (modelData.iconActive !== undefined)
                                                     ? modelData.iconActive
                                                     : checked

            size: root.buttonSize
            borderRadius: SVUnits.radius
            text: modelData.text ? modelData.text : ""
            checked: root.isItemChecked(modelData)
            enabled: modelData.enabled !== undefined ? modelData.enabled : true
            iconSource: itemIconActive && modelData && modelData.alternateIconSource
                        ? modelData.alternateIconSource
                        : (modelData && modelData.iconSource ? modelData.iconSource : "")

            onClicked: {
                if (modelData.checkable && root.autoUpdateActiveId && root.exclusiveSelection) {
                    root.activeId = modelData.id
                }

                root.itemSelected(modelData.id)
            }
        }
    }

    Rectangle {
        id: background
        anchors.fill: parent
        color: qgcPalette.windowTransparent
        radius: SVUnits.radius
    }

    Grid {
        id: layoutGrid
        anchors.fill: parent
        anchors.margins: SVUnits.margin
        rows: root.isVertical ? 3 : 1
        columns: root.isVertical ? 1 : 3
        spacing: 0

        Loader {
            id: headerButton
            visible: !root.headerless
            sourceComponent: headerButtonComponent
        }

        Item {
            id: separator
            visible: root.separatorVisible
            width: root.isVertical ? root.buttonSize : root.separatorSpan
            height: root.isVertical ? root.separatorSpan : root.buttonSize

            Rectangle {
                anchors.centerIn: parent
                width: root.isVertical ? parent.width * 0.8 : SVUnits.lineWidth
                height: root.isHorizontal ? parent.height : SVUnits.lineWidth
                color: qgcPalette.windowShadeLight
            }
        }

        Grid {
            id: contentGrid
            visible: root.contentVisible
            columns: root.isVertical ? 1 : root.gridItemCount
            rows: root.isHorizontal ? 1 : root.gridItemCount
            spacing: 0

            Repeater {
                model: root.model
                delegate: menuButtonDelegate
            }
        }
    }

}
