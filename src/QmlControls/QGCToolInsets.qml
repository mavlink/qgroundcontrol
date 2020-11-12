/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick  2.12

QtObject {
    property real leftEdgeCenterInset:  0
    property real leftEdgeTopInset:         0
    property real leftEdgeBottomInset:      0
    property real rightEdgeCenterInset: 0
    property real rightEdgeTopInset:        0
    property real rightEdgeBottomInset:     0
    property real topEdgeCenterInset:     0
    property real topEdgeLeftInset:         0
    property real topEdgeRightInset:        0
    property real bottomEdgeCenterInset:  0
    property real bottomEdgeLeftInset:      0
    property real bottomEdgeRightInset:     0

    signal insetsChanged

    onLeftEdgeBottomInsetChanged:       insetsChanged()
    onLeftEdgeTopInsetChanged:          insetsChanged()
    onLeftEdgeCenterInsetChanged:   insetsChanged()
    onRightEdgeBottomInsetChanged:      insetsChanged()
    onRightEdgeCenterInsetChanged:  insetsChanged()
    onRightEdgeTopInsetChanged:         insetsChanged()
    onBottomEdgeLeftInsetChanged:       insetsChanged()
    onBottomEdgeRightInsetChanged:      insetsChanged()
    onBottomEdgeCenterInsetChanged:   insetsChanged()
    onTopEdgeLeftInsetChanged:          insetsChanged()
    onTopEdgeRightInsetChanged:         insetsChanged()
    onTopEdgeCenterInsetChanged:      insetsChanged()
}
