/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Effects

Item {
    id: glassBackdrop

    property Item  sourceItem:            null
    property Item  targetItem:            parent
    property bool  sampleAtItemPosition:  true
    property real  sampleX:               0
    property real  sampleY:               0
    property real  sourceScale:           0.50
    property real  blurAmount:            0.78
    property real  blurMax:               32
    property real  sourceBrightness:     -0.08
    property real  sourceSaturation:      0.62
    property color tintColor:             Qt.rgba(0.045, 0.048, 0.052, 0.80)
    property color sheenColor:            Qt.rgba(1, 1, 1, 0.0)
    property bool  backdropBlurEnabled:   true
    property real  cornerRadius:          0

    clip: true

    function sourceSampleRect() {
        if (!sourceItem || !targetItem) {
            return Qt.rect(0, 0, Math.max(1, width), Math.max(1, height))
        }

        var sourcePoint = sampleAtItemPosition ? targetItem.mapToItem(sourceItem, 0, 0) : Qt.point(sampleX, sampleY)
        return Qt.rect(sourcePoint.x, sourcePoint.y, Math.max(1, width), Math.max(1, height))
    }

    Item {
        id: emptySource
        width:  1
        height: 1
        visible: false
    }

    ShaderEffectSource {
        id:             backdropTexture
        anchors.fill:   parent
        visible:        false
        live:           glassBackdrop.backdropBlurEnabled && !!glassBackdrop.sourceItem && glassBackdrop.visible
        recursive:      false
        hideSource:     false
        sourceItem:     glassBackdrop.sourceItem ? glassBackdrop.sourceItem : emptySource
        sourceRect:     glassBackdrop.sourceSampleRect()
        textureSize:    Qt.size(Math.max(1, Math.round(glassBackdrop.width * glassBackdrop.sourceScale)),
                                Math.max(1, Math.round(glassBackdrop.height * glassBackdrop.sourceScale)))
    }

    MultiEffect {
        anchors.fill:   parent
        visible:        glassBackdrop.backdropBlurEnabled && !!glassBackdrop.sourceItem && glassBackdrop.blurAmount > 0
        source:         backdropTexture
        blurEnabled:    glassBackdrop.backdropBlurEnabled && glassBackdrop.blurAmount > 0
        blurMax:        glassBackdrop.blurMax
        blur:           glassBackdrop.blurAmount
        brightness:     glassBackdrop.sourceBrightness
        saturation:     glassBackdrop.sourceSaturation
        maskEnabled:    glassBackdrop.cornerRadius > 0
        maskSource:     roundedMask
    }

    Rectangle {
        anchors.fill: parent
        color:        glassBackdrop.tintColor
        radius:       glassBackdrop.cornerRadius
    }

    Rectangle {
        anchors.left:   parent.left
        anchors.right:  parent.right
        anchors.top:    parent.top
        height:         Math.max(1, parent.height * 0.18)
        color:          glassBackdrop.sheenColor
        radius:         glassBackdrop.cornerRadius
    }

    Item {
        id:             roundedMask
        width:          glassBackdrop.width
        height:         glassBackdrop.height
        visible:        false
        layer.enabled:  true

        Rectangle {
            anchors.fill:   parent
            radius:         glassBackdrop.cornerRadius
            color:          "black"
        }
    }
}
