#pragma once

#include <cstdint>

#include <QtCore/QObject>
#include <QtQmlIntegration/qqmlintegration.h>

#include "digiview_commons/public_enums.hpp"

namespace DigiviewProtocol
{
Q_NAMESPACE
QML_NAMED_ELEMENT(DigiviewProtocol)

enum LayoutMode : uint8_t {
    LayoutSingleCamera = ::Layout::LAYOUT_1,
    LayoutTwoColumns = ::Layout::LAYOUT_2_COLUMNS,
    LayoutTwoRows = ::Layout::LAYOUT_2_ROWS,
    LayoutTop2Bottom1 = ::Layout::LAYOUT_TOP_2_BOTTOM_1,
    LayoutTop2Bottom2 = ::Layout::LAYOUT_TOP_2_BOTTOM_2,
    LayoutTop3Bottom1 = ::Layout::LAYOUT_TOP_3_BOTTOM_1,
    LayoutSourceFrame = ::Layout::LAYOUT_SOURCE_FRAME,
    LayoutMaximum = ::Layout::LAYOUT_MAX,
};
Q_ENUM_NS(LayoutMode)

enum DetectionOverlayMode : uint8_t {
    DetectionOverlayNone = ::Layout::DET_OVERLAY_NONE,
    DetectionOverlaySingleTopRight = ::Layout::DET_OVERLAY_SINGLE_TOP_RIGHT,
    DetectionOverlayColumnRight = ::Layout::DET_OVERLAY_COLUMN_RIGHT,
    DetectionOverlayColumnLeft = ::Layout::DET_OVERLAY_COLUMN_LEFT,
    DetectionOverlayRowTop = ::Layout::DET_OVERLAY_ROW_TOP,
    DetectionOverlayRowBottom = ::Layout::DET_OVERLAY_ROW_BOTTOM,
    DetectionOverlayMaximum = ::Layout::DET_OVERLAY_MAX,
};
Q_ENUM_NS(DetectionOverlayMode)

enum TargetingMode : uint8_t {
    TargetingDirectional = ::View::DIRECTIONAL,
    TargetingCoordinate = ::View::COORDINAL,
    TargetingDetection = ::View::DETECTION,
    TargetingSingleTargetTracking = ::View::SINGLE_TARGET_TRACKING,
};
Q_ENUM_NS(TargetingMode)

enum SingleTargetTrackingCommand : uint8_t {
    SttCommandOff = ::CMD_OFF,
    SttCommandSetTargetVector = ::CMD_SET_TARGET_VECTOR,
    SttCommandNone = ::CMD_NONE,
};
Q_ENUM_NS(SingleTargetTrackingCommand)

enum SingleTargetTrackingStatus : uint8_t {
    SttStatusOff = static_cast<uint8_t>(::single_target_tracking_status::OFF),
    SttStatusInit = static_cast<uint8_t>(::single_target_tracking_status::INIT),
    SttStatusRunning = static_cast<uint8_t>(::single_target_tracking_status::RUNNING),
    SttStatusDropped = static_cast<uint8_t>(::single_target_tracking_status::DROPPED),
};
Q_ENUM_NS(SingleTargetTrackingStatus)

enum CalibrationCommand : uint8_t {
    CalibrationCommandNone = ::CALIBRATION_CMD_NONE,
    CalibrationCommandStartAll = ::CALIBRATION_CMD_START_ALL,
    CalibrationCommandStart6Dof = ::CALIBRATION_CMD_START_6DOF,
    CalibrationCommandStartMag = ::CALIBRATION_CMD_START_MAG,
};
Q_ENUM_NS(CalibrationCommand)

enum CalibrationStatus : uint8_t {
    CalibrationStatusNotStarted = ::CALIBRATION_STATUS_NOT_STARTED,
    CalibrationStatus6DofXPositive = ::CALIBRATION_STATUS_6DOF_X_POS,
    CalibrationStatus6DofXNegative = ::CALIBRATION_STATUS_6DOF_X_NEG,
    CalibrationStatus6DofYPositive = ::CALIBRATION_STATUS_6DOF_Y_POS,
    CalibrationStatus6DofYNegative = ::CALIBRATION_STATUS_6DOF_Y_NEG,
    CalibrationStatus6DofZPositive = ::CALIBRATION_STATUS_6DOF_Z_POS,
    CalibrationStatus6DofReady = ::CALIBRATION_STATUS_6DOF_READY,
    CalibrationStatus6DofComplete = ::CALIBRATION_STATUS_6DOF_COMPLETE,
    CalibrationStatusMagComplete = ::CALIBRATION_STATUS_MAG_COMPLETE,
    CalibrationStatusMagFailed = ::CALIBRATION_STATUS_MAG_FAILED,
};
Q_ENUM_NS(CalibrationStatus)
} // namespace DigiviewProtocol

static_assert(DigiviewProtocol::LayoutMaximum == DigiviewProtocol::LayoutSourceFrame);
static_assert(DigiviewProtocol::DetectionOverlayMaximum == DigiviewProtocol::DetectionOverlayRowBottom);
static_assert(static_cast<uint8_t>(DigiviewProtocol::TargetingDetection)
              == static_cast<uint8_t>(::View::DETECTION));
static_assert(static_cast<uint8_t>(DigiviewProtocol::SttCommandNone) == static_cast<uint8_t>(::CMD_NONE));
static_assert(DigiviewProtocol::SttStatusDropped
              == static_cast<uint8_t>(::single_target_tracking_status::DROPPED));
static_assert(static_cast<uint8_t>(DigiviewProtocol::CalibrationStatusMagFailed)
              == static_cast<uint8_t>(::CALIBRATION_STATUS_MAG_FAILED));
