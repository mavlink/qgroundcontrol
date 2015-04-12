/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0

Item {
    id: slider;
    height: 10
    property real value // value is read/write.
    property real minimum: 0
    property real maximum: 1
    property int length: width - handle.width

    Rectangle {
        anchors.fill: parent
        border.width: 1;
        border.color: "lightgrey"
        radius: 8
        color: "white"
        opacity: 1
    }

    Rectangle {
        anchors.left: parent.left
        anchors.leftMargin: 4
        anchors.top: parent.top
        anchors.topMargin: (parent.height - height)/2
        height: 3
        width: handle.x - x
        color: "#1c94fc"
    }


    Rectangle {
        id: labelRect
        width: label.width
        height: label.height + 4
        radius: 4
        smooth: true
        color: "white"
        border.color: "lightgrey"
        anchors.bottom: handle.top
        anchors.bottomMargin: 4
        x: Math.max(Math.min(handle.x + (handle.width - width )/2, slider.width - width),0)
        visible: mouseRegion.pressed
        Text{
            id: label
            color: "darkgrey"
            text: slider.value.toFixed(2)
            width: font.pointSize * 3.5
            anchors.horizontalCenter: labelRect.horizontalCenter
            horizontalAlignment: Text.AlignHCenter
            anchors.baseline: parent.bottom
            anchors.baselineOffset: -6
            font.pixelSize: 14
        }
    }

    Rectangle {
        id: handle;
        smooth: true
        width: 26;
        y: (slider.height - height)/2;
        x: (slider.value - slider.minimum) * slider.length / (slider.maximum - slider.minimum)

        height: width; radius: width/2
        gradient: normalGradient
        border.width: 2
        border.color: "white"

        Gradient {
            id: normalGradient
            GradientStop { position: 0.0; color: "#b0b0b0" }
            GradientStop { position: 0.66; color: "#909090" }
            GradientStop { position: 1.0; color: "#545454" }
        }

        MouseArea {
            id: mouseRegion
            hoverEnabled: false
            anchors.fill: parent; drag.target: parent
            drag.axis: Drag.XAxis; drag.minimumX: 0; drag.maximumX: slider.length
            preventStealing: true
            onPositionChanged: { slider.value = (slider.maximum - slider.minimum) * handle.x / slider.length + slider.minimum; }
        }
    }
}
