/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick

// Custom builds can override this resource to add additional custom actions
Item {
    visible: false

    property var guidedController

    property bool anyActionAvailable: false
    property var model: [ ]
}
