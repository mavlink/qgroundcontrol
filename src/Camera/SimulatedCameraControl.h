/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "MavlinkCameraControl.h"

#include <QtCore/QTimer>
#include <QtCore/QElapsedTimer>

class Vehicle;

/// Creates a simulated Camera Control which supports:
///     Video record if a manual stream is available
///     Photo capture using DO_DIGICAM_CONTROL if the setting is enabled
///     It does not support time lapse capture

class SimulatedCameraControl : public MavlinkCameraControl
{
    Q_OBJECT

public:
    SimulatedCameraControl(Vehicle* vehicle, QObject* parent = nullptr);
    virtual ~SimulatedCameraControl();
 
    void                setCameraModeVideo  () override;
    void                setCameraModePhoto  () override;
    void                toggleCameraMode    () override;
    bool                takePhoto           () override;
    bool                startVideoRecording () override;
    bool                stopVideoRecording  () override;
    bool                toggleVideoRecording() override;
    bool                capturesVideo       () override;
    bool                capturesPhotos      () override;
    bool                hasVideoStream      () override;
    bool                photosInVideoMode   () override { return true; }
    bool                videoInPhotoMode    () override { return false; }
    void                setCameraMode       (CameraMode mode) override;
    void                setPhotoCaptureMode (PhotoCaptureMode mode) override;
    void                setPhotoLapse       (qreal interval) override;
    void                setPhotoLapseCount  (int count) override;
    bool                hasModes            () override;
    quint32             recordTime          () override;
    QString             recordTimeStr       () override;
    VideoCaptureStatus  videoCaptureStatus  () override;
    PhotoCaptureStatus  photoCaptureStatus  () override { return _photoCaptureStatus; }
    PhotoCaptureMode    photoCaptureMode    () override { return _photoCaptureMode; }
    CameraMode          cameraMode          () override { return _cameraMode; }
    qreal               photoLapse          () override { return _photoLapse; }
    int                 photoLapseCount     () override { return _photoLapseCount; }

    // The following overrides are defaulted/unsupported
    void                resetSettings               () override {}
    void                formatCard                  (int id = 1) override { Q_UNUSED(id);}
    void                stepZoom                    (int /*direction*/) override {}
    void                startZoom                   (int /*direction*/) override {}
    void                stopZoom                    () override {}
    void                stopStream                  () override {}
    bool                stopTakePhoto               () override { return false;}
    void                resumeStream                () override {}
    void                startTracking               (QRectF /*rec*/) override {}
    void                startTracking               (QPointF /*point*/, double /*radius*/) override {}
    void                stopTracking                () override {}
    int                 version                     () override { return 0; }
    QString             modelName                   () override { return QStringLiteral("Simulated Camera"); }
    QString             vendor                      () override { return QStringLiteral("QGroundControl"); }
    QString             firmwareVersion             () override { return QStringLiteral("1.0"); }
    qreal               focalLength                 () override { return qQNaN(); }
    QSizeF              sensorSize                  () override { return QSizeF(qQNaN(), qQNaN()); }
    QSize               resolution                  () override { return QSize(0, 0); }
    bool                hasZoom                     () override { return false; }
    bool                hasFocus                    () override { return false; }
    bool                hasTracking                 () override { return false; }
    int                 compID                      () override { return 0; }
    bool                isBasic                     () override { return true; }
    StorageStatus       storageStatus               () override { return STORAGE_NOT_SUPPORTED; }
    QStringList         activeSettings              () override { return QStringList(); }
    quint32             storageFree                 () override { return 0; }
    QString             storageFreeStr              () override { return QString(); }
    quint32             storageTotal                () override { return 0; }
    int                 batteryRemaining            () override { return -1;}
    QString             batteryRemainingStr         () override { return QString(); }
    bool                paramComplete               () override { return true; }
    qreal               zoomLevel                   () override { return 1.0; }
    qreal               focusLevel                  () override { return 1.0; }
    QGCVideoStreamInfo* thermalStreamInstance       () override { return nullptr; }
    QGCVideoStreamInfo* currentStreamInstance       () override { return nullptr; }
    int                 currentStream               () override { return 0; }
    QmlObjectListModel* streams                     () override { return nullptr; }
    void                setCurrentStream            (int /*stream*/) override {}
    bool                autoStream                  () override { return false; }
    Fact*               exposureMode                () override { return nullptr; }
    Fact*               ev                          () override { return nullptr; }
    Fact*               iso                         () override { return nullptr; }
    Fact*               shutterSpeed                () override { return nullptr; }
    Fact*               aperture                    () override { return nullptr; }
    Fact*               wb                          () override { return nullptr; }
    Fact*               mode                        () override { return nullptr; }
    QStringList         streamLabels                () override { return QStringList(); }
    ThermalViewMode     thermalMode                 () override { return THERMAL_OFF; }
    void                setThermalMode              (ThermalViewMode /*mode*/) override {}
    double              thermalOpacity              () override { return 0.0; }
    void                setThermalOpacity           (double /*val*/) override {}
    void                setZoomLevel                (qreal /*level*/) override {}
    void                setFocusLevel               (qreal /*level*/) override {}
    bool                trackingEnabled             () override { return false; }
    void                setTrackingEnabled          (bool /*set*/) override {}
    TrackingStatus      trackingStatus              () override { return TRACKING_UNKNOWN; }
    bool                trackingImageStatus         () override { return false; }
    QRectF              trackingImageRect           () override { return QRectF(); }
    void                factChanged                 (Fact* /*pFact*/) override {};
    bool                incomingParameter           (Fact* /*pFact*/, QVariant& /*newValue*/) override { return false; }
    bool                validateParameter           (Fact* /*pFact*/, QVariant& /*newValue*/) override { return false; }
    void                handleSettings              (const mavlink_camera_settings_t& /*settings*/) override {}
    void                handleCaptureStatus         (const mavlink_camera_capture_status_t& /*capStatus*/) override {}
    void                handleParamAck              (const mavlink_param_ext_ack_t& /*ack*/) override {}
    void                handleParamValue            (const mavlink_param_ext_value_t& /*value*/) override {}
    void                handleStorageInfo           (const mavlink_storage_information_t& /*st*/) override {}
    void                handleBatteryStatus         (const mavlink_battery_status_t& /*bs*/) override {}
    void                handleTrackingImageStatus   (const mavlink_camera_tracking_image_status_t* /*tis*/) override {}
    void                handleVideoInfo             (const mavlink_video_stream_information_t* /*vi*/) override {}
    void                handleVideoStatus           (const mavlink_video_stream_status_t* /*vs*/) override {}

protected slots:
    void _paramDone() override {};

private:
    void _setCameraMode(CameraMode mode);

    Vehicle*            _vehicle                    = nullptr;
    CameraMode          _cameraMode                 = CAM_MODE_UNDEFINED;
    VideoCaptureStatus  _videoCaptureStatus         = VIDEO_CAPTURE_STATUS_STOPPED;
    PhotoCaptureStatus  _photoCaptureStatus         = PHOTO_CAPTURE_IDLE;
    PhotoCaptureMode    _photoCaptureMode           = PHOTO_CAPTURE_SINGLE;
    qreal               _photoLapse                 = 1.0;
    int                 _photoLapseCount            = 0;
    QTimer              _videoRecordTimeUpdateTimer;
    QElapsedTimer       _videoRecordTimeElapsedTimer;
};
