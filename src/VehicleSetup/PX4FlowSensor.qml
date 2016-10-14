/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2
import QtQuick.Dialogs          1.2

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.FactSystem            1.0
import QGroundControl.FactControls          1.0
import QGroundControl.Palette               1.0
import QGroundControl.Controllers           1.0
import QGroundControl.ScreenTools           1.0

QGCView {
    id:         qgcView
    viewPanel:  panel

    property var _activeVehicle:  QGroundControl.multiVehicleManager.activeVehicle

    QGCPalette { id: qgcPal; colorGroupEnabled: panel.enabled }

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent

        QGCLabel {
            id:             titleLabel
            text:           qsTr("PX4Flow Camera")
            font.family:    ScreenTools.demiboldFontFamily
        }

        Image {
            source:     _activeVehicle ? "image://QGCImages/" + _activeVehicle.id + "/" + _activeVehicle.flowImageIndex : ""
            width:      parent.width * 0.5
            height:     width * 0.75
            cache:      false
            anchors.centerIn: parent
            fillMode: Image.PreserveAspectFit
        }
    }
}
