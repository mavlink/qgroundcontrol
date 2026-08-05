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
    property string menuDirection: direction

    property url source
    property url alternateSource
    property string menuText: ""
    property string menuDescription: ""

    property var model: []
    property string activeId: ""
    property var activeIds: []

    property real buttonSize: SVUnits.objectWidth - SVUnits.lineWidth * 2//Math.min(SVUnits.objectWidth - SVUnits.bigMargin, ScreenTools.implicitButtonHeight + (SVUnits.bigMargin * 2))

    readonly property bool isHorizontal: direction === horizontal
    readonly property bool isVertical: !isHorizontal



    property bool isLeft: true
    property bool isTop: true

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
            : + (root.headerless ? 0 : root.buttonSize)
              + (root.separatorVisible ? root.separatorSpan : 0)
              + (root.contentVisible ? contentGrid.implicitWidth : 0) + 2
    height: isHorizontal 
            ? SVUnits.objectWidth 
            : + (root.headerless ? 0 : root.buttonSize)
              + (root.separatorVisible ? root.separatorSpan : 0)
              + (root.contentVisible ? contentGrid.implicitHeight : 0) + 2

    QGCPalette { id: qgcPalette }

    Component {
        id: headerButtonComponent

        SVMenuStripButton {
            size: root.buttonSize
            borderRadius: SVUnits.radius
            text: root.menuText
            description: root.menuDescription
            iconSource: root.useAlternateHeaderIcon ? root.alternateSource : root.source
            checked: root.open
            enabled: root.enabled
            extendHeader: true
            expanded: root.open
            onClicked: root.open = !root.open
            isLeft: root.isLeft
            isTop: root.isTop
            isVertical: menuDirection === vertical
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
            isLeft: root.isLeft
            isTop: root.isTop
            isVertical: root.isVertical
            text: modelData.text ? modelData.text : ""
            description: modelData.description ? modelData.description : ""
            tintIcon: modelData && (modelData.tintIcon !== undefined) ? modelData.tintIcon : true
            checked: root.isItemChecked(modelData)
            enabled: (modelData.enabled !== undefined ? modelData.enabled : true) && root.enabled
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

    SVBackground {
        id: background
        anchors.fill: parent
        radius: SVUnits.radius
        borderColor: qgcPalette.windowShade
        enabled: true
        hoverEnabled: false
        checkable: false
        checked: false
        hovered: false
        pressed: false
        borderWidth: SVSettings.simplifiedUserInterface ? 0 : 1
    }

    Grid {
        id: layoutGrid
        anchors.fill: parent
        anchors.margins: SVUnits.lineWidth
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
                width: {
                    if(root.isVertical) {
                        return parent.width * (SVSettings.simplifiedUserInterface ? 0.75 : 0.9) + (SVUnits.lineWidth * 2)
                    } else {
                        return SVUnits.lineWidth
                    }
                }

                height: {
                    if(root.isHorizontal) {
                        return parent.height * (SVSettings.simplifiedUserInterface ? 0.75 : 0.9) + (SVUnits.lineWidth * 2)
                    } else {
                        return SVUnits.lineWidth
                    }
                }
                
                color: qgcPalette.windowShadeLight

                gradient: SVSettings.simplifiedUserInterface
                        ? null
                        : separatorGradient

                Gradient {
                    id: separatorGradient

                    orientation: root.isVertical ? Gradient.Horizontal : Gradient.Vertical

                    GradientStop {
                        position: 0.0
                        color: "transparent"
                    }

                    GradientStop {
                        position: 0.15
                        color: qgcPalette.windowShadeLight
                    }

                    GradientStop {
                        position: 0.85
                        color: qgcPalette.windowShadeLight
                    }

                    GradientStop {
                        position: 1.0
                        color: "transparent"
                    }
                }
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
