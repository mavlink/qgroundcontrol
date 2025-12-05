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

import QtQuick
import QtQuick.Controls
import org.freedesktop.gstreamer.Qt6GLVideoItem

GstGLQt6VideoItem {
    id: videoBackground
}
