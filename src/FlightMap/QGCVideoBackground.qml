/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief QGC Video Background
 *   @author Gus Grubba <gus@auterion.com>
 */

import QtQuick                          2.11
import QtQuick.Controls                 2.4
import QGroundControl.QgcQtGStreamer    1.0

VideoItem {
    id: videoBackground
    property var display
    property var receiver
    surface: display
}
