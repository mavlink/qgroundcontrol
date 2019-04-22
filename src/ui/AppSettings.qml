/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts  1.2

import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

import QtQuick.Controls.Styles 1.4


Rectangle {
    id:     settingsView
    color:  qgcPal.window
    z:      QGroundControl.zOrderTopMost

    QGCPalette { id: qgcPal }

    TabView {
        id:                 tabview
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        anchors.left:       parent.left
        anchors.right:      parent.right

        Repeater{
            model:  QGroundControl.corePlugin.settingsPages
            Tab {
                title: modelData.title
                source: modelData.url
            }
        }
        style:TabViewStyle {
            tab: Item{
            implicitWidth: Math.max(text.width + 8, 100);
            implicitHeight: 80;
                Rectangle {
                    width: parent.width
                    height: parent.height
                    anchors.top: parent.top;
                    visible: styleData.selected;
                    gradient: Gradient {
                        GradientStop{position: 0.0; color: "#606060";}
                        GradientStop{position: 0.5; color: "#c0c0c0";}
                        GradientStop{position: 1.0; color: "#a0a0a0";}
                    }
                }
                Rectangle {
                    width: 2;
                    height: parent.height - 4;
                    anchors.top: parent.top;
                    anchors.right: parent.right;
                    visible: styleData.index < control.count - 1;
                    gradient: Gradient {
                        GradientStop{position: 0.0; color: "#404040";}
                        GradientStop{position: 0.5; color: "#707070";}
                        GradientStop{position: 1.0; color: "#404040";}
                    }
                }
                RowLayout {
                    implicitWidth: Math.max(text.width, 80);
                    height: 70;
                    anchors.centerIn: parent;
                    z: 1;
                    Text {
                        id: text;
                        text: styleData.title;
                        font.pointSize: 14;
                        color: styleData.selected ? "blue" : (styleData.hovered ? "green" : "white");
                    }
                }
           }
           tabBar:Rectangle {
                height: 80;
                gradient: Gradient {
                    GradientStop{position: 0.0; color: "#484848";}
                    GradientStop{position: 0.3; color: "#787878";}
                    GradientStop{position: 1.0; color: "#a0a0a0";}
                }
                Rectangle {
                   width: parent.width;
                   height: 4;
                   anchors.bottom: parent.bottom;
                   border.width: 2;
                   border.color: "#c7c7c7";
                 }
             }
        }
    }
}
