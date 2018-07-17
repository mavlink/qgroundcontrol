/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick      2.3
import QtQml.Models 2.1

ObjectModel {
    id: _root

    function reset() {
        for (var i=0; i<_root.count; i++) {
            var group = _root.get(i)
            group.reset()
            group.enabled = i === 0
            group._checked = i === 0
        }
    }

    Component.onCompleted: reset()

}
