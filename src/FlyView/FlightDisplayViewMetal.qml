import QtQuick
import QtMultimedia

import QGroundControl

// macOS Metal rendering path for GStreamer video.
// Frames are pushed from the C++ GstAppSinkAdapter to this VideoOutput's QVideoSink.
VideoOutput {
    objectName: "videoContent"
    fillMode:   VideoOutput.PreserveAspectFit

    Connections {
        target: QGroundControl.videoManager
        function onImageFileChanged(filename) {
            grabToImage(function(result) {
                if (!result.saveToFile(filename)) {
                    console.error('Error capturing video frame');
                }
            });
        }
    }
}
