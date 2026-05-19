.pragma library

// Status constants — extended for auto-sampling mission stages
var StatusIdle = 0
var StatusSampling = 1
var StatusDetecting = 2
var StatusFault = 3
var StatusCalibrating = 4
var StatusNavigating = 5
var StatusWaypointReached = 6
var StatusHolding = 7
var StatusWaitingStable = 8
var StatusSamplingDone = 9
var StatusResumingAuto = 10
var StatusPaused = 11
var StatusAborted = 12
var StatusHoldNoMission = 13
var StatusSurveying = 14

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
            || payloadStatus === StatusNavigating
            || payloadStatus === StatusHolding
            || payloadStatus === StatusWaitingStable
            || payloadStatus === StatusResumingAuto
            || payloadStatus === StatusSurveying
    const isFault = payloadStatus === StatusFault
            || payloadStatus === StatusAborted
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

function shouldSampleAbsorbance(payloadStatus, baselineSet) {
    const chartActive = payloadStatus === StatusSampling
            || payloadStatus === StatusDetecting
            || payloadStatus === StatusCalibrating
            || payloadStatus === StatusWaitingStable
            || payloadStatus === StatusSurveying
    return !!baselineSet && chartActive
}
