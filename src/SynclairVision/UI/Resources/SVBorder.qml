import QtQuick

Item {
    id: root

    property real borderWidth: SVUnits.lineWidth
    property color borderColor: "white"
    property real radius: 0
    property color fillColor: "transparent"

    property bool borderVisible: false
    property bool pulse: false
    property real pulseMinOpacity: 0.4
    property real pulseMaxOpacity: 1.0
    property int pulseDuration: 2000

    property int flashDuration: 1000
    property real flashStartOpacity: 1.0
    property real flashEndOpacity: 0.0

    property real _pulseOpacity: pulseMaxOpacity
    property real _flashOpacity: 0.0
    readonly property real _baseOpacity: borderVisible ? (pulse ? _pulseOpacity : 1.0) : 0.0

    function trigger() {
        flashAnimation.stop()
        _flashOpacity = flashStartOpacity
        flashAnimation.start()
    }

    onPulseChanged: _pulseOpacity = pulseMaxOpacity

    onBorderVisibleChanged: {
        if (borderVisible) {
            _pulseOpacity = pulseMaxOpacity
        }
    }

    Rectangle {
        id: borderFrame

        anchors.fill: parent
        color: root.fillColor
        radius: root.radius * 4
        border.width: root.borderWidth
        border.color: root.borderColor
        opacity: Math.max(root._baseOpacity, root._flashOpacity)
        visible: opacity > 0.01
    }

    Rectangle {
        id: outerBorderFrame

        anchors.fill: parent
        color: root.fillColor
        anchors.margins: root.borderWidth
        border.width: SVUnits.lineWidth
        border.color: "black"
        opacity: Math.max(root._baseOpacity, root._flashOpacity)
        visible: opacity > 0.01
    }

    SequentialAnimation {
        id: pulseAnimation

        running: root.borderVisible && root.pulse
        loops: Animation.Infinite

        NumberAnimation {
            target: root
            property: "_pulseOpacity"
            from: root.pulseMaxOpacity
            to: root.pulseMinOpacity
            duration: root.pulseDuration / 2
            easing.type: Easing.InOutSine
        }

        NumberAnimation {
            target: root
            property: "_pulseOpacity"
            from: root.pulseMinOpacity
            to: root.pulseMaxOpacity
            duration: root.pulseDuration / 2
            easing.type: Easing.InOutSine
        }
    }

    NumberAnimation {
        id: flashAnimation

        target: root
        property: "_flashOpacity"
        from: root.flashStartOpacity
        to: root.flashEndOpacity
        duration: root.flashDuration
        easing.type: Easing.InOutSine
    }
}
