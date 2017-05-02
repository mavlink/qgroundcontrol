/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick 2.2
import QtQuick.Layouts 1.2

import QGroundControl.Controls 1.0
import QGroundControl.ScreenTools 1.0

QGCButton {
    Layout.minimumHeight: ScreenTools.mobileMinimumButtonSize
    Layout.minimumWidth: ScreenTools.mobileMinimumButtonSize

    height: Math.max(ScreenTools.mobileMinimumButtonSize, implicitHeight)
    width: Math.max(ScreenTools.mobileMinimumButtonSize, implicitWidth)

    checkable:  true
}
