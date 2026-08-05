import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs
import QGroundControl.FirstRunPromptDialogs

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls

FirstRunPrompt {
    id: root
    title: qsTr("Welcome to Synclair: QGroundControl")
    promptId: QGroundControl.corePlugin.svInitialWelcomePromptId

    // Ensure it updates settings when closed so it won't open again
    onClosed: {
        var appSettings = QGroundControl.settingsManager.appSettings
        var shownIds = appSettings.firstRunPromptIdsShown.rawValue
        
        if (!shownIds.includes(promptId)) {
            shownIds.push(promptId)
            appSettings.firstRunPromptIdsShown.rawValue = shownIds
        }
    }

    QGCPalette { id: qgcPalette }

    property int selectedDropdown: -1

    Flickable {
        id: contentFlickable

        width: SVUnits.objectWidth * 13
        height: SVUnits.objectHeight * 4.5
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        contentWidth: width
        contentHeight: contentColumn.height

        ScrollBar.vertical: ScrollBar {
            id: contentScrollBar
        }

        Column {
            id: contentColumn
            width: parent.width
            spacing: SVUnits.bigMargin * 2

            Rectangle {
                width: parent.width
                height: SVUnits.objectHeight * 2
                color: "transparent"
                radius: SVUnits.radius

                Image {
                    id: noVideo
                    anchors.fill: parent
                    source: Qt.resolvedUrl("../Resources/Images/no_video_background.png")                    
                    fillMode: Image.PreserveAspectCrop
                    visible: true
                }   

                Rectangle {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: SVUnits.objectWidth * 4

                    gradient: Gradient {
                        GradientStop { position: 0.0; color: "transparent" }
                        GradientStop { position: 1.0; color: qgcPalette.window }
                    }
                }

                Row {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.leftMargin: SVUnits.bigMargin * 2
                    anchors.bottomMargin: SVUnits.bigMargin * 2
                    spacing: SVUnits.bigMargin

                    Image {
                        width: SVUnits.objectWidth * 1
                        height: SVUnits.objectWidth * 1
                        source: "/res/resources/svlogo.png"
                        fillMode: Image.PreserveAspectCrop 
                        smooth: true
                        mipmap: true
                        antialiasing: true
                        asynchronous: true
                    }

                    Column {
                        anchors.top: parent.top
                        anchors.topMargin: - SVUnits.margin
                        spacing: 0

                        QGCLabel {
                            text: qsTr("Welcome to SynclairVision's QGroundControl")
                            font.pointSize: SVUnits.largeText
                            color: qgcPalette.text
                        }

                        QGCLabel {
                            text: qsTr("Test test, test test, and test test")
                            font.pointSize: SVUnits.svText
                            color: qgcPalette.text
                        }
                    }
                }

                SVBorder {
                    anchors.fill: parent
                    borderVisible: true
                    radius: SVUnits.radius
                }
            }

            Column {
                width: parent.width
                spacing: SVUnits.bigMargin * 4

                Column {
                    id: about
                    width: parent.width
                    spacing: SVUnits.margin

                    QGCLabel {
                        text: qsTr("About")
                        font.pointSize: SVUnits.svText
                        color: qgcPalette.buttonHighlight
                    }

                    QGCLabel {
                        width: parent.width
                        wrapMode: Text.WordWrap
                        text: qsTr("Lalalalal al al la la l l l al al al fld fdf afeuiahfeuia uifeuai fehuia huiefahuf ehuif huieaf uhiea uhlfeahlu flehuaflhu eabf ebiulfe abiulf eabliufe abiulfe abiuf ebau fbiuf abiu fbiuf  febiuf ebiuf bbuibiuf bulfeabulfb ubu leafbl e bualfbeualfb uelbua lbufelbu albueablufebafbeablfeabfebiuafbuebufbeuabfuebuafbefbueafbuiebuafbiubuiubibu ")
                        font.pointSize: SVUnits.svText
                        color: qgcPalette.text
                    }

                    Rectangle {
                        width: SVUnits.objectWidth
                        height: SVUnits.margin - SVUnits.lineWidth * 2
                        color: "transparent"
                        border.width: 0
                    }

                    Rectangle {
                        width: SVUnits.objectWidth
                        height: SVUnits.lineWidth
                        color: qgcPalette.windowShade
                        border.width: 0
                    }
                }

                Column {
                    id: getStarted
                    width: parent.width
                    spacing: SVUnits.margin * 1

                    QGCLabel {
                        text: qsTr("Get Started")
                        font.pointSize: SVUnits.svText
                        color: qgcPalette.buttonHighlight
                    }

                    RowLayout {
                        id: getStartedRow
                        width: parent.width
                        spacing: SVUnits.bigMargin

                        readonly property real collapsedCardWidth: SVUnits.objectWidth
                        readonly property real defaultCardWidth: (width - (3 * spacing)) / 4
                        readonly property real expandedCardWidth: width - (3 * collapsedCardWidth) - (3 * spacing)

                        Repeater {
                            model: 4

                            Item {
                                id: cardItem


                                required property int index

                                readonly property bool isSelected: root.selectedDropdown === index
                                readonly property bool hasSelection: root.selectedDropdown !== -1
                                readonly property bool isCollapsed: hasSelection && !isSelected

                                Layout.fillWidth: false
                                Layout.preferredWidth: {
                                    if (!cardItem.hasSelection) return getStartedRow.defaultCardWidth;
                                    return cardItem.isSelected ? getStartedRow.expandedCardWidth : getStartedRow.collapsedCardWidth;
                                }
                                
                                // Consistent card height prevents vertical size jumping
                                implicitHeight: SVUnits.objectHeight * 0.5

                                Behavior on Layout.preferredWidth {
                                    NumberAnimation { duration: 200; easing.type: Easing.InOutQuad }
                                }

                                SVBackground {
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    normalColor: qgcPalette.windowShade
                                    hovered: mouseArea.containsMouse
                                    checkable: true
                                    checked: cardItem.isSelected
                                    pressed: mouseArea.pressed
                                    hoverPosition: Qt.point(mouseArea.mouseX, mouseArea.mouseY)
                                    radius: SVUnits.radius
                                }

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: cardItem.isCollapsed ? 0 : SVUnits.bigMargin * 1.5
                                    spacing: SVUnits.margin

                                    clip: true

                                    QGCColoredImage {
                                        Layout.alignment: cardItem.isCollapsed ? (Qt.AlignVCenter | Qt.AlignHCenter) : (Qt.AlignVCenter | Qt.AlignLeft)
                                        Layout.preferredWidth: SVUnits.width * 2
                                        Layout.preferredHeight: SVUnits.width * 2
                                        source: "/qmlimages/settings_main.svg"
                                        color: "white"
                                    }

                                    QGCLabel {
                                        Layout.fillWidth: true
                                        Layout.alignment: Qt.AlignLeft
                                        text: qsTr("Connect to Digiview")
                                        font.pointSize: SVUnits.svText
                                        color: qgcPalette.text
                                        horizontalAlignment: Text.AlignLeft
                                        visible: !cardItem.isCollapsed

                                        
                                    }
                                }

                                Rectangle {
                                    anchors.left: parent.left
                                    anchors.right: parent.right

                                    anchors.leftMargin: cardItem.index === 0 ? 0 : 5
                                    anchors.rightMargin: cardItem.index === getStartedRow.count - 1 ? 0 : 5
                                    height: 20
                                    color: qgcPalette.windowShade
                                    visible: cardItem.isSelected
                                    opacity: 0.5
                                    y: parent.height
                                }

                                MouseArea {
                                    id: mouseArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onClicked: {
                                        if (root.selectedDropdown === cardItem.index) {
                                            root.selectedDropdown = -1
                                        } else {
                                            root.selectedDropdown = cardItem.index
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        width: SVUnits.objectWidth
                        height: SVUnits.margin / 4 - 2
                        color: "transparent"
                        border.width: 0
                    }

                    Rectangle {
                        width: parent.width
                        height: SVUnits.objectHeight * 2
                        color: qgcPalette.windowShade
                        radius: SVUnits.radius
                        visible: root.selectedDropdown !== -1
                        clip: true

                        Behavior on visible {
                            NumberAnimation { duration: 200 }
                        }

                        QGCLabel {
                            anchors.centerIn: parent
                            text: qsTr("Content panel for option %1").arg(root.selectedDropdown + 1)
                            font.pointSize: SVUnits.svText
                            color: qgcPalette.text
                        }
                    }

                    Rectangle {
                        width: SVUnits.objectWidth
                        height: SVUnits.margin / 4
                        color: "transparent"
                        border.width: 0
                    }

                    Rectangle {
                        width: parent.width
                        height: SVUnits.lineWidth
                        color: qgcPalette.windowShade
                        border.width: 0
                    }
                }

                Column {
                    id: learnMore
                    width: parent.width
                    spacing: SVUnits.margin * 1

                    QGCLabel {
                        text: qsTr("Learn More")
                        font.pointSize: SVUnits.svText
                        color: qgcPalette.buttonHighlight
                    }

                    Column {
                        width: parent.width
                        spacing: SVUnits.margin

                        Item {
                            width: parent.width
                            height: SVUnits.objectWidth / 1.5

                            SVBackground {
                                anchors.fill: parent
                                hoverEnabled: true
                                transparentBackground: true
                                hovered: mouseAreaDocumentation.containsMouse
                                checkable: true
                                pressed: mouseAreaDocumentation.pressed
                                hoverPosition: Qt.point(mouseAreaDocumentation.mouseX, mouseAreaDocumentation.mouseY)
                                radius: SVUnits.radius
                            }
                                        
                            MouseArea {
                                id: mouseAreaDocumentation
                                anchors.fill: parent
                                hoverEnabled: true
                            }

                            QGCLabel {
                                anchors.left: parent.left
                                anchors.leftMargin: SVUnits.bigMargin
                                anchors.verticalCenter: parent.verticalCenter
                                text: qsTr("User Documentation")
                                font.pointSize: SVUnits.svText
                                color: qgcPalette.text
                            }

                            QGCColoredImage {
                                anchors.right: parent.right
                                anchors.rightMargin: SVUnits.bigMargin
                                anchors.verticalCenter: parent.verticalCenter
                                width: SVUnits.width * 2
                                height: SVUnits.width * 2
                                source: "/qmlimages/external_link.svg"
                                color: "white"
                            }
                        }

                        Rectangle {
                            width: parent.width
                            height: SVUnits.lineWidth
                            color: qgcPalette.windowShade
                            border.width: 0
                        }

                        Item {
                            width: parent.width
                            height: SVUnits.objectWidth / 1.5

                            SVBackground {
                                anchors.fill: parent
                                hoverEnabled: true
                                transparentBackground: true
                                hovered: mouseAreaPatch.containsMouse
                                checkable: true
                                pressed: mouseAreaPatch.pressed
                                hoverPosition: Qt.point(mouseAreaPatch.mouseX, mouseAreaPatch.mouseY)
                                radius: SVUnits.radius
                            }
                                        
                            MouseArea {
                                id: mouseAreaPatch
                                anchors.fill: parent
                                hoverEnabled: true
                            }

                            QGCLabel {
                                anchors.left: parent.left
                                anchors.leftMargin: SVUnits.bigMargin
                                anchors.verticalCenter: parent.verticalCenter
                                text: qsTr("Patch Notes")
                                font.pointSize: SVUnits.svText
                                color: qgcPalette.text
                            }

                            QGCColoredImage {
                                anchors.right: parent.right
                                anchors.rightMargin: SVUnits.bigMargin
                                anchors.verticalCenter: parent.verticalCenter
                                width: SVUnits.width * 2
                                height: SVUnits.width * 2
                                source: "/qmlimages/external_link.svg"
                                color: "white"
                            }
                        }
                    }

                    Rectangle {
                        width: SVUnits.objectWidth
                        height: SVUnits.lineWidth
                        color: qgcPalette.windowShade
                        border.width: 0
                    }
                }
            }
        }
    }
}