/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MavlinkCameraControl.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(CameraControlLog, "Camera.MavlinkCameraControl")
QGC_LOGGING_CATEGORY(CameraControlVerboseLog, "Camera.MavlinkCameraControl:verbose")

MavlinkCameraControl::MavlinkCameraControl(Vehicle *vehicle, QObject *parent)
    : FactGroup(0, parent, true /* ignore camel case */)
    , _vehicle(vehicle)
{
    qCDebug(CameraControlLog) << this;
}

MavlinkCameraControl::~MavlinkCameraControl()
{
    qCDebug(CameraControlLog) << this;
}

QString MavlinkCameraControl::captureImageStatusToStr(uint8_t image_status)
{
    switch (image_status) {
        case PHOTO_CAPTURE_IDLE:
            return QStringLiteral("Idle");
        case PHOTO_CAPTURE_IN_PROGRESS:
            return QStringLiteral("Capturing");
        case PHOTO_CAPTURE_INTERVAL_IDLE:
            return QStringLiteral("Idle: Interval set");
        case PHOTO_CAPTURE_INTERVAL_IN_PROGRESS:
            return QStringLiteral("Capturing: Interval set");
        default:
            return QStringLiteral("Unknown");
    }
}

QString MavlinkCameraControl::captureVideoStatusToStr(uint8_t video_status)
{
    switch (video_status) {
        case VIDEO_CAPTURE_STATUS_STOPPED:
            return QStringLiteral("Idle");
        case VIDEO_CAPTURE_STATUS_RUNNING:
            return QStringLiteral("Capturing");
        default:
            return QStringLiteral("Unknown");
    }
}

QString MavlinkCameraControl::storageStatusToStr(uint8_t status)
{
    switch (status) {
        case STORAGE_STATUS_EMPTY:
            return QStringLiteral("Empty");
        case STORAGE_STATUS_UNFORMATTED:
            return QStringLiteral("Unformatted");
        case STORAGE_STATUS_READY:
            return QStringLiteral("Ready");
        case STORAGE_STATUS_NOT_SUPPORTED:
            return QStringLiteral("Not Supported");
        default:
            return QStringLiteral("Unknown");
    }
}

QString MavlinkCameraControl::cameraModeToStr(CameraMode mode)
{
    switch (mode) {
        case CAM_MODE_UNDEFINED:
            return QStringLiteral("CAM_MODE_UNDEFINED");
        case CAM_MODE_PHOTO:
            return QStringLiteral("CAM_MODE_PHOTO");
        case CAM_MODE_VIDEO:
            return QStringLiteral("CAM_MODE_VIDEO");
        case CAM_MODE_SURVEY:
            return QStringLiteral("CAM_MODE_SURVEY");
        default:
            return QStringLiteral("Unknown");
    }
}
