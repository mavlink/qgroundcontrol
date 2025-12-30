/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QTimer>

#include "MavlinkCameraControl.h"

class QGCVideoStreamInfo;
class Vehicle;

/// Creates a simulated Camera Control which supports:
///     Video record if a manual stream is available
///     Photo capture using DO_DIGICAM_CONTROL if the setting is enabled
///     It does not support time lapse capture
class SimulatedCameraControl : public MavlinkCameraControl
{
    Q_OBJECT

public:
    explicit SimulatedCameraControl(Vehicle *vehicle, QObject *parent = nullptr);
    ~SimulatedCameraControl() override;

    void setCameraModeVideo() override;
    void setCameraModePhoto() override;
    void toggleCameraMode() override;
    bool takePhoto() override;
    bool startVideoRecording() override;
    bool stopVideoRecording() override;
    bool toggleVideoRecording() override;
    void resetSettings() override {}
    void formatCard(int id = 1) override { Q_UNUSED(id);}
    void stepZoom(int /*direction*/) override {}
    void startZoom(int /*direction*/) override {}
    void stopZoom() override {}
    void stopStream() override {}
    bool stopTakePhoto() override { return false;}
    void resumeStream() override {}
    void startTracking(QRectF /*rec*/) override {}
    void startTracking(QPointF /*point*/, double /*radius*/) override {}
    void stopTracking() override {}

    int version() const override { return 0; }
    QString modelName() const override { return QStringLiteral("Simulated Camera"); }
    QString vendor() const override { return QStringLiteral("QGroundControl"); }
    QString firmwareVersion() const override { return QStringLiteral("1.0"); }
    qreal focalLength() const override { return qQNaN(); }
    QSizeF sensorSize() const override { return QSizeF(qQNaN(), qQNaN()); }
    QSize resolution() const override { return QSize(0, 0); }
    bool capturesVideo() const override;
    bool capturesPhotos() const override;
    bool hasModes() const override { return (capturesPhotos() && capturesVideo()); }
    bool hasZoom() const override { return false; }
    bool hasFocus() const override { return false; }
    bool hasTracking() const override { return false; }
    bool hasVideoStream() const override;
    bool photosInVideoMode() const override { return true; }
    bool videoInPhotoMode() const override { return false; }

    int compID() const override { return 0; }
    bool isBasic() const override { return true; }
    StorageStatus storageStatus() const override { return STORAGE_NOT_SUPPORTED; }
    QStringList activeSettings() const override { return QStringList(); }
    quint32 storageFree() const override { return 0; }
    QString storageFreeStr() const override { return QString(); }
    quint32 storageTotal() const override { return 0; }
    int batteryRemaining() const override { return -1;}
    QString batteryRemainingStr() const override { return QString(); }
    bool paramComplete() const override { return true; }
    qreal zoomLevel() const override { return 1.0; }
    qreal focusLevel() const override { return 1.0; }

    QmlObjectListModel *streams() override { return nullptr; }
    QGCVideoStreamInfo *currentStreamInstance() override { return nullptr; }
    QGCVideoStreamInfo *thermalStreamInstance() override { return nullptr; }
    int currentStream() const override { return 0; }
    void setCurrentStream(int /*stream*/) override {}
    bool autoStream() const override { return false; }
    quint32 recordTime() const override;
    QString recordTimeStr() const override;

    Fact *exposureMode() override { return nullptr; }
    Fact *ev() override { return nullptr; }
    Fact *iso() override { return nullptr; }
    Fact *shutterSpeed() override { return nullptr; }
    Fact *aperture() override { return nullptr; }
    Fact *wb() override { return nullptr; }
    Fact *mode() override { return nullptr; }

    QStringList streamLabels() const override { return QStringList(); }

    ThermalViewMode thermalMode() const override { return THERMAL_OFF; }
    void setThermalMode(ThermalViewMode /*mode*/) override {}
    double thermalOpacity() const override { return 0.0; }
    void setThermalOpacity(double /*val*/) override {}

    void setZoomLevel(qreal /*level*/) override {}
    void setFocusLevel(qreal /*level*/) override {}
    void setCameraMode(CameraMode mode) override;
    void setPhotoCaptureMode(PhotoCaptureMode mode) override;
    void setPhotoLapse(qreal interval) override {}
    void setPhotoLapseCount(int count) override {}

    bool trackingEnabled() const override { return false; }
    void setTrackingEnabled(bool /*set*/) override {}

    TrackingStatus trackingStatus() const override { return TRACKING_UNKNOWN; }

    bool trackingImageStatus() const override { return false; }
    QRectF trackingImageRect() const override { return QRectF(); }

    void factChanged(Fact* /*pFact*/) override {};
    bool incomingParameter(Fact* /*pFact*/, QVariant& /*newValue*/) override { return false; }
    bool validateParameter(Fact* /*pFact*/, QVariant& /*newValue*/) override { return false; }

    void handleBatteryStatus(const mavlink_battery_status_t& /*bs*/) override {}
    void handleCaptureStatus(const mavlink_camera_capture_status_t& /*capStatus*/) override {}
    void handleParamAck(const mavlink_param_ext_ack_t& /*ack*/) override {}
    void handleParamValue(const mavlink_param_ext_value_t& /*value*/) override {}
    void handleSettings(const mavlink_camera_settings_t& /*settings*/) override {}
    void handleStorageInfo(const mavlink_storage_information_t& /*st*/) override {}
    void handleTrackingImageStatus(const mavlink_camera_tracking_image_status_t* /*tis*/) override {}
    void handleVideoInfo(const mavlink_video_stream_information_t* /*vi*/) override {}
    void handleVideoStatus(const mavlink_video_stream_status_t* /*vs*/) override {}

protected slots:
    void _paramDone() override {};

private:
    void _setCameraMode(CameraMode mode);

    QElapsedTimer _videoRecordTimeElapsedTimer;
};
