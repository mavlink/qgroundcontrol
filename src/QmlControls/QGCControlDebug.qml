/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick 2.12

Item {
    property string name: "control"

    Connections {
        target:             parent
        onXChanged:         console.log(name, "xChanged",       parent.x)
        onYChanged:         console.log(name, "yChanged",       parent.y)
        onWidthChanged:     console.log(name, "widthChanged",   parent.width)
        onHeightChanged:    console.log(name, "heightChanged",  parent.height)
        onVisibleChanged:   console.log(name, "visibleChanged", parent.visible)
        onZChanged:         console.log(name, "zChanged",       parent.z)
        onParentChanged:    console.log(name, "parentChanged",  parent.parent)
    }
}
