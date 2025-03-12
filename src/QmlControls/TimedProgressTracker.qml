import QtQuick

Item {
    id: root
    // Setable propterties
    property real   timeoutSeconds:     0
    property string progressLabel:      ""
    property bool   running:            false
    
    // Do not set these properties
    property real   progress:           0
    
    signal timeout()
    
    property double _lastUpdateTime:    0
    
    Timer {
        id:             timeoutTimer
        interval:       timeoutSeconds * 1000
        repeat:         false
        running:        running

        onTriggered: {
            root.running = false
            root.progress = 0
            root.progressLabel = ""
            root.timeout()
        }
    }
    
    SequentialAnimation on progress { 
        id:             progressAnimation
        running:        root.running
        loops:          1

        NumberAnimation { 
            target: root
            property: "progress"
            to: 1
            duration: timeoutSeconds * 1000 
        }
    }
    
    onProgressChanged: {
        const currentTime = Date.now() * 0.001
        if (currentTime - _lastUpdateTime < 0.1) {
            return
        }
        
        if (running) {
            var currentCount = (progress * timeoutSeconds)
            progressLabel = (timeoutSeconds - currentCount).toFixed(1)
        } else {
            progressLabel = ""
        }
        
        _lastUpdateTime = currentTime
    }
    
    function start() {
        running = true
        progress = 0
        timeoutTimer.restart()
        progressAnimation.restart()
    }
    
    function stop() {
        running = false
        progress = 0
        timeoutTimer.stop()
        progressAnimation.stop()
        progressLabel = ""
    }
}
