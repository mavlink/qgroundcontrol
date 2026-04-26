import QtQuick

import QGroundControl

// Tints an SVG (or raster) by routing through the `coloredsvg` C++ image provider,
// which rasterizes the source and composites the tint over its alpha mask.
Item {
    id: root

    property color  color:  "white"
    property url    source

    property alias asynchronous:        image.asynchronous
    property alias cache:               image.cache
    property alias fillMode:            image.fillMode
    property alias horizontalAlignment: image.horizontalAlignment
    property alias mirror:              image.mirror
    property alias paintedHeight:       image.paintedHeight
    property alias paintedWidth:        image.paintedWidth
    property alias progress:            image.progress
    property alias mipmap:              image.mipmap
    property alias sourceSize:          image.sourceSize
    property alias status:              image.status
    property alias verticalAlignment:   image.verticalAlignment

    width:  image.width
    height: image.height

    // Strip qrc: scheme and ensure leading '/' so the provider URL stays well-formed.
    readonly property string _path: {
        const s = source.toString()
        if (s.length === 0)        return ""
        if (s.startsWith("qrc:/")) return s.substring(4)
        if (s.startsWith("/"))     return s
        return "/" + s
    }
    // QColor in C++ parses "#" prefixes as URL fragments, so strip it.
    readonly property string _hex: color.toString().replace("#", "")

    Image {
        id:                 image
        smooth:             true
        mipmap:             true
        antialiasing:       true
        asynchronous:       true
        fillMode:           Image.PreserveAspectFit
        anchors.fill:       parent
        sourceSize.height:  height
        source:             root._path.length > 0
                            ? "image://coloredsvg" + root._path + "?color=" + root._hex
                            : ""
    }
}
