/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

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

                        Rectangle
                        {
                            height: item.height
                            width: item.width - 1
                            color: qgcPal.transparent
                            border.width: 1
                            border.color: qgcPal.windowShade
                        }
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
