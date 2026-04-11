.pragma library

var StatusIdle = 0
var StatusSampling = 1
var StatusDetecting = 2
var StatusFault = 3
var StatusCalibrating = 4

var Tokens = {
    opacity: {
        panel: 0.92,
        overlay: 0.85,
        disabled: 0.35,
        subtle: 0.55
    },
    spacing: {
        xs: 0.3,
        sm: 0.6,
        md: 1.0,
        lg: 1.5,
        xl: 2.0
    },
    radius:  {
        sm: 0.4,
        md: 0.7,
        lg: 1.0,
        pill: 999
    },
    touch:   {
        minHeight: 2.2
    }
}

function payloadState(hasVehicle, payloadStatus, linkOk, expanded) {
    const isWorking = payloadStatus === StatusSampling
            || payloadStatus === StatusDetecting
            || payloadStatus === StatusCalibrating
    const isFault = payloadStatus === StatusFault
    const isOffline = hasVehicle && !linkOk
    const forcedDetail = isWorking || isFault || isOffline

    return {
        compact: !expanded && !forcedDetail,
        showDetail: expanded || forcedDetail,
        showDiagnostics: expanded || isFault || isOffline,
        showQuickActions: hasVehicle && (isWorking || isFault),
        emphasizeSummary: isWorking || isFault || isOffline,
        severity: isFault ? "critical" : (isOffline ? "warning" : (isWorking ? "active" : "normal"))
    }
}
