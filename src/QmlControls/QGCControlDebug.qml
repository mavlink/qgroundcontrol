/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick 2.12

Rectangle {
    id:             _root
    border.width:   1
    border.color:   borderColor
    color:          "transparent"

    property string name
    property color  borderColor: "red"

    function logEverything() {
        console.log(qsTr("%1 x:%2 y:%3 width:%4 height:%5 visible:%6 enabled:%7 z:%8 parent:%9 implicitWidth/Height:%10:%11", "Do not translate")
                    .arg(name).arg(parent.x).arg(parent.y).arg(parent.width).arg(parent.height).arg(parent.visible).arg(parent.enabled).arg(parent.z).arg(parent.parent).arg(implicitHeight).arg(implicitWidth))
    }

    Component.onCompleted: logEverything()

    Connections {
        target:             parent
        onXChanged:         { console.log(name, "xChanged",       parent.x);        logEverything() }
        onYChanged:         { console.log(name, "yChanged",       parent.y);        logEverything() }
        onWidthChanged:     { console.log(name, "widthChanged",   parent.width);    logEverything() }
        onHeightChanged:    { console.log(name, "heightChanged",  parent.height);   logEverything() }
        onVisibleChanged:   { console.log(name, "visibleChanged", parent.visible);  logEverything() }
        onZChanged:         { console.log(name, "zChanged",       parent.z);        logEverything() }
        onParentChanged:    { console.log(name, "parentChanged",  parent.parent);   logEverything() }
        onEnabledChanged:   { console.log(name, "enabledChanged", parent.enabled);  logEverything() }
    }
}
