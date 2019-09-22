import QtQuick          2.11
import QtQuick.Controls 2.4

import QGroundControl.ScreenTools   1.0

Rectangle {
    id: sideMenu
    y: toolBar.height
    height: mainWindow.height - toolBar.height
    width: 180;
    color: qgcPal.window
    visible: false

    property bool rollUpMenu: false

    enum MenuItems { Setup }

        function triggerRoll()
        {
            visible = !rollUpMenu
            rollUpMenu = !rollUpMenu;

            if(!rollUpMenu)
                listView.currentIndex = -1

            return rollUpMenu
        }

            signal setupMenuClicked()

            ListView {
                id: listView
                anchors.fill: parent
                model: SideMenuModel {}

                Component.onCompleted: listView.currentIndex = -1

                delegate: Component {
                    Item {
                        id: item
                        width: parent.width
                        height: 60

                        Column {
                            Text {
                                text: name
                                leftPadding: ScreenTools.defaultFontPixelHeight / 2
                                topPadding: ScreenTools.defaultFontPixelHeight
                                font.pixelSize: 14
                                font.bold: true
                                opacity: enabled ? 1.0 : 0.3
                                color: qgcPal.buttonText
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                        }
                        MouseArea {
                            anchors.fill: parent

                            onClicked:
                            {
                                listView.currentIndex = index

                                switch(listView.currentIndex)
                                {
                                case SideMenu.MenuItems.Setup:
                                    setupMenuClicked()
                                    break;
                                }
                            }
                        }
                    }
                }

                highlight: Rectangle { color: qgcPal.brandingDarkBlue }

                focus: true
            }
        }




        //Menu {
        //    id: menu
        //    y: toolBar.height
        //    height: mainWindow.height - toolBar.height
        //    font.pixelSize: 17
        //    font.bold: true

        //    readonly property int menuItemHeight: 70
        //    readonly property int menuItemWidth: 180
        //    property bool rollUpMenu: false

        //    signal setupMenuClicked()

        //    function triggerRoll()
        //    {
        //        visible = !rollUpMenu
        //        rollUpMenu = !rollUpMenu;

        //        return rollUpMenu
        //    }

        //    background: Rectangle {
        //        implicitWidth: menuItemWidth
        //        implicitHeight: menuItemHeight
        //        color: qgcPal.window
        //        border.width: 2 * topWindow.sizeFactor; border.color: qgcPal.windowShade
        //    }

        //    MenuItem {
        //        id: menuItem
        //        implicitWidth: menuItemWidth
        //        implicitHeight: menuItemHeight

        //        text: qsTr("Settings")

        //        background: Rectangle {
        //            id: backgroundRectangle
        //            anchors.fill: menu

        //            opacity: enabled ? 1 : 0.3
        //            color: menuItem.highlighted ? qgcPal.brandingDarkBlue : qgcPal.transparent
        //        }

        //        contentItem: Text {
        //            leftPadding: 10
        //            rightPadding: 10
        //            text: menuItem.text
        //            font: menuItem.font
        //            opacity: enabled ? 1.0 : 0.3
        //            color: qgcPal.buttonText
        //            horizontalAlignment: Text.AlignLeft
        //            verticalAlignment: Text.AlignVCenter
        //            elide: Text.ElideRight
        //        }

        //        onTriggered:
        //        {
        //            //rollUpMenu = false
        //            menuItem.visible = true
        //           //menu.setupMenuClicked()
        //        }
        //    }
        //}
