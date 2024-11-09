// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QGroundControl.GraphicalEffects

/*!
    \qmltype ColorOverlay
    \inqmlmodule Qt5Compat.GraphicalEffects
    \since QtGraphicalEffects 1.0
    \inherits QtQuick2::Item
    \ingroup qtgraphicaleffects-color
    \brief Alters the colors of the source item by applying an overlay color.

    The effect is similar to what happens when a colorized glass is put on top
    of a grayscale image. The color for the overlay is given in the RGBA format.

    \table
    \header
        \li Source
        \li Effect applied
    \row
        \li \image Original_butterfly.png
        \li \image ColorOverlay_butterfly.png
    \endtable

    \section1 Example

    The following example shows how to apply the effect.
    \snippet ColorOverlay-example.qml example

*/
Item {
    id: rootItem

    /*!
        This property defines the source item that provides the source pixels
        for the effect.

        \note It is not supported to let the effect include itself, for
        instance by setting source to the effect's parent.
    */
    property variant source

    /*!
        This property defines the color value which is used to colorize the
        source.

        By default, the property is set to \c "transparent".

        \table
        \header
        \li Output examples with different color values
        \li
        \li
        \row
            \li \image ColorOverlay_color1.png
            \li \image ColorOverlay_color2.png
            \li \image ColorOverlay_color3.png
        \row
            \li \b { color: #80ff0000 }
            \li \b { color: #8000ff00 }
            \li \b { color: #800000ff }
        \endtable

    */
    property color color: "transparent"

    /*!
        This property allows the effect output pixels to be cached in order to
        improve the rendering performance.

        Every time the source or effect properties are changed, the pixels in
        the cache must be updated. Memory consumption is increased, because an
        extra buffer of memory is required for storing the effect output.

        It is recommended to disable the cache when the source or the effect
        properties are animated.

        By default, the property is set to \c false.

    */
    property bool cached: false

    SourceProxy {
        id: sourceProxy
        input: rootItem.source
        interpolation: input && input.smooth ? SourceProxy.LinearInterpolation : SourceProxy.NearestInterpolation
    }

    ShaderEffectSource {
        id: cacheItem
        anchors.fill: parent
        visible: rootItem.cached
        smooth: true
        sourceItem: shaderItem
        live: true
        hideSource: visible
    }

    ShaderEffect {
        id: shaderItem
        property variant source: sourceProxy.output
        property color color: rootItem.color

        anchors.fill: parent

        fragmentShader: "qrc:/qt/qml/QGroundControl/GraphicalEffects/coloroverlay.frag.qsb"
    }
}
