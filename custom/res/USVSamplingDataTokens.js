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

// PID Mode constants
var PidIdle = 0
var PidRunning = 1
var PidDone = 2
var PidError = 3

var Tokens = {
    opacity: {
        panel: 0.95,
        cardBg: 0.08,
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
    radius: {
        sm: 0.4,
        md: 0.7,
        lg: 1.0
    },
    chart: {
        maxPoints: 120,        // 60s × 2Hz
        sampleIntervalMs: 500,
        voltageColor: "#3b82f6",
        absorbanceColor: "#f59e0b",
        marginPercent: 0.15
    }
}

// Map status integer to display text
function statusText(status) {
    switch (status) {
    case StatusIdle: return "空闲"
    case StatusSampling: return "采样中"
    case StatusDetecting: return "检测中"
    case StatusFault: return "故障"
    case StatusCalibrating: return "校准中"
    case StatusNavigating: return "航行中"
    case StatusWaypointReached: return "到达航点"
    case StatusHolding: return "保持"
    case StatusWaitingStable: return "稳定等待"
    case StatusSamplingDone: return "采样完成"
    case StatusResumingAuto: return "恢复航行"
    case StatusPaused: return "已暂停"
    case StatusAborted: return "已中止"
    case StatusHoldNoMission: return "无任务保持"
    case StatusSurveying: return "走航检测"
    default: return "未知"
    }
}

// Map PID mode to display text
function pidModeText(mode) {
    switch (mode) {
    case PidIdle: return "空闲"
    case PidRunning: return "运行中"
    case PidDone: return "已完成"
    case PidError: return "错误"
    default: return "未知"
    }
}

// Map PID mode to color name (use with QGCPalette)
function pidModeColorName(mode) {
    switch (mode) {
    case PidIdle: return "text"       // gray/neutral
    case PidRunning: return "colorBlue"
    case PidDone: return "colorGreen"
    case PidError: return "colorRed"
    default: return "text"
    }
}
