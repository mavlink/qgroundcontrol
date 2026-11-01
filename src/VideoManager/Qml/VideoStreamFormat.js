.pragma library

// Text for the video stream statistics the toolbar indicator and its drawer show.

function bitrateText(kbps) {
    return kbps >= 1000 ? qsTr("%1 Mbit/s").arg((kbps / 1000).toFixed(1)) : qsTr("%1 kbit/s").arg(kbps.toFixed(0))
}

function durationText(ms) {
    const totalSeconds = Math.floor(ms / 1000)
    const hours = Math.floor(totalSeconds / 3600)
    const minutes = Math.floor(totalSeconds / 60) % 60
    const seconds = String(totalSeconds % 60).padStart(2, "0")
    return hours > 0 ? qsTr("%1:%2:%3").arg(hours).arg(String(minutes).padStart(2, "0")).arg(seconds)
                     : qsTr("%1:%2").arg(minutes).arg(seconds)
}
