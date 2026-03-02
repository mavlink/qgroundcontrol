#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QPointF>
#include <QtCore/QRectF>
#include <QtCore/QSizeF>

#include "FactGroup.h"
#include "QGCVideoStreamInfo.h"
#include "QmlObjectListModel.h"

#include <QtQmlIntegration/QtQmlIntegration>

class QGCCameraParamIO;
class Vehicle;

Q_DECLARE_LOGGING_CATEGORY(CameraControlLog)
Q_DECLARE_LOGGING_CATEGORY(CameraControlVerboseLog)

/// Abstract base class for all camera controls: real and simulated
class MavlinkCameraControl : public FactGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_PROPERTY(Fact*                exposureMode            READ exposureMode                                       NOTIFY parametersReady)
    Q_PROPERTY(Fact*                ev                      READ ev                                                 NOTIFY parametersReady)
    Q_PROPERTY(Fact*                iso                     READ iso                                                NOTIFY parametersReady)
    Q_PROPERTY(Fact*                shutterSpeed            READ shutterSpeed                                       NOTIFY parametersReady)
    Q_PROPERTY(Fact*                aperture                READ aperture                                           NOTIFY parametersReady)
    Q_PROPERTY(Fact*                wb                      READ wb                                                 NOTIFY parametersReady)
    Q_PROPERTY(Fact*                mode                    READ mode                                               NOTIFY parametersReady)

    Q_PROPERTY(int                  version                 READ version                                            NOTIFY infoChanged)
    Q_PROPERTY(QString              modelName               READ modelName                                          NOTIFY infoChanged)
    Q_PROPERTY(QString              vendor                  READ vendor                                             NOTIFY infoChanged)
    Q_PROPERTY(QString              firmwareVersion         READ firmwareVersion                                    NOTIFY infoChanged)
    Q_PROPERTY(qreal                focalLength             READ focalLength                                        NOTIFY infoChanged)
    Q_PROPERTY(QSizeF               sensorSize              READ sensorSize                                         NOTIFY infoChanged)
    Q_PROPERTY(QSize                resolution              READ resolution                                         NOTIFY infoChanged)
    Q_PROPERTY(bool                 hasModes                READ hasModes                                           NOTIFY infoChanged)
    Q_PROPERTY(bool                 hasZoom                 READ hasZoom                                            NOTIFY infoChanged)
    Q_PROPERTY(bool                 hasFocus                READ hasFocus                                           NOTIFY infoChanged)
    Q_PROPERTY(bool                 hasVideoStream          READ hasVideoStream                                     NOTIFY infoChanged)
    Q_PROPERTY(bool                 hasTracking             READ hasTracking                                        NOTIFY infoChanged)
    Q_PROPERTY(bool                 photosInVideoMode       READ photosInVideoMode                                  NOTIFY infoChanged)
    Q_PROPERTY(bool                 videoInPhotoMode        READ videoInPhotoMode                                   NOTIFY infoChanged)
    Q_PROPERTY(bool                 isBasic                 READ isBasic                                            NOTIFY infoChanged)


    Q_PROPERTY(CameraMode           cameraMode              READ cameraMode             WRITE setCameraMode         NOTIFY cameraModeChanged)

    Q_PROPERTY(quint32              storageFree             READ storageFree                                        NOTIFY storageFreeChanged)
    Q_PROPERTY(QString              storageFreeStr          READ storageFreeStr                                     NOTIFY storageFreeChanged)
    Q_PROPERTY(quint32              storageTotal            READ storageTotal                                       NOTIFY storageTotalChanged)
    Q_PROPERTY(int                  batteryRemaining        READ batteryRemaining                                   NOTIFY batteryRemainingChanged)
    Q_PROPERTY(QString              batteryRemainingStr     READ batteryRemainingStr                                NOTIFY batteryRemainingChanged)
    Q_PROPERTY(bool                 paramComplete           READ paramComplete                                      NOTIFY parametersReady)
    Q_PROPERTY(qreal                zoomLevel               READ zoomLevel              WRITE setZoomLevel          NOTIFY zoomLevelChanged)
    Q_PROPERTY(qreal                focusLevel              READ focusLevel             WRITE setFocusLevel         NOTIFY focusLevelChanged)
    Q_PROPERTY(QStringList          activeSettings          READ activeSettings                                     NOTIFY activeSettingsChanged)
    Q_PROPERTY(StorageStatus        storageStatus           READ storageStatus                                      NOTIFY storageStatusChanged)
    Q_PROPERTY(qreal                photoLapse              READ photoLapse             WRITE setPhotoLapse         NOTIFY photoLapseChanged)
    Q_PROPERTY(int                  photoLapseCount         READ photoLapseCount        WRITE setPhotoLapseCount    NOTIFY photoLapseCountChanged)
    Q_PROPERTY(PhotoCaptureMode     photoCaptureMode        READ photoCaptureMode       WRITE setPhotoCaptureMode   NOTIFY photoCaptureModeChanged)
    Q_PROPERTY(int                  currentStream           READ currentStream          WRITE setCurrentStream      NOTIFY currentStreamChanged)
    Q_PROPERTY(bool                 autoStream              READ autoStream                                         NOTIFY autoStreamChanged)
    Q_PROPERTY(QmlObjectListModel*  streams                 READ streams                                            NOTIFY streamsChanged)
    Q_PROPERTY(QGCVideoStreamInfo*  currentStreamInstance   READ currentStreamInstance                              NOTIFY currentStreamChanged)
    Q_PROPERTY(QGCVideoStreamInfo*  thermalStreamInstance   READ thermalStreamInstance                              NOTIFY thermalStreamChanged)
    Q_PROPERTY(quint32              recordTime              READ recordTime                                         NOTIFY recordTimeChanged)
    Q_PROPERTY(QString              recordTimeStr           READ recordTimeStr                                      NOTIFY recordTimeChanged)
    Q_PROPERTY(QStringList          streamLabels            READ streamLabels                                       NOTIFY streamLabelsChanged)
    Q_PROPERTY(ThermalViewMode      thermalMode             READ thermalMode            WRITE setThermalMode        NOTIFY thermalModeChanged)
    Q_PROPERTY(double               thermalOpacity          READ thermalOpacity         WRITE setThermalOpacity     NOTIFY thermalOpacityChanged)
    Q_PROPERTY(bool                 trackingEnabled         READ trackingEnabled        WRITE setTrackingEnabled    NOTIFY trackingEnabledChanged)
    Q_PROPERTY(TrackingStatus       trackingStatus          READ trackingStatus                                     CONSTANT)
    Q_PROPERTY(bool                 trackingImageStatus     READ trackingImageStatus                                NOTIFY trackingImageStatusChanged)
    Q_PROPERTY(QRectF               trackingImageRect       READ trackingImageRect                                  NOTIFY trackingImageStatusChanged)

    // These properties are used to determine what controls to show in the UI and are based on both the camera capabilities as well as the video manager status.
    // They are not necessarily directly related to the MAVLink camera capabilities.
    Q_PROPERTY(bool                 capturesVideo           READ capturesVideo                                      NOTIFY infoChanged)
    Q_PROPERTY(bool                 capturesPhotos          READ capturesPhotos                                     NOTIFY infoChanged)

    // These are "virtual" states which take into account both the mavlink camera capabilities as well as the gstreamer ability to locally record video
    // as well as to do a photo grab from the video stream. These are what the UI should use to determine what options to present to the user and are updated
    // based on both the camera info as well as the video manager status.
    Q_PROPERTY(CaptureVideoState    captureVideoState       READ captureVideoState                                  NOTIFY captureVideoStateChanged)
    Q_PROPERTY(CapturePhotosState   capturePhotosState      READ capturePhotosState                                 NOTIFY capturePhotosStateChanged)

    friend class QGCCameraParamIO;

public:
    explicit MavlinkCameraControl(Vehicle *vehicle, QObject *parent = nullptr);
    virtual ~MavlinkCameraControl();

    enum CameraMode {
        CAM_MODE_UNDEFINED = -1,
        CAM_MODE_PHOTO = 0,
        CAM_MODE_VIDEO = 1,
        CAM_MODE_SURVEY = 2,
    };
    Q_ENUM(CameraMode)

    enum PhotoCaptureMode {
        PHOTO_CAPTURE_SINGLE = 0,
        PHOTO_CAPTURE_TIMELAPSE,
    };
    Q_ENUM(PhotoCaptureMode)

    enum CaptureVideoState {
        CaptureVideoStateDisabled = 0,
        CaptureVideoStateIdle,
        CaptureVideoStateCapturing,
    };
    Q_ENUM(CaptureVideoState)

    enum CapturePhotosState {
        CapturePhotosStateDisabled = 0,
        CapturePhotosStateIdle,
        CapturePhotosStateCapturingSinglePhoto,
        CapturePhotosStateCapturingMultiplePhotos,
    };
    Q_ENUM(CapturePhotosState)

    enum StorageStatus {
        STORAGE_EMPTY = STORAGE_STATUS_EMPTY,
        STORAGE_UNFORMATTED = STORAGE_STATUS_UNFORMATTED,
        STORAGE_READY = STORAGE_STATUS_READY,
        STORAGE_NOT_SUPPORTED = STORAGE_STATUS_NOT_SUPPORTED
    };
    Q_ENUM(StorageStatus)

    enum ThermalViewMode {
        THERMAL_OFF = 0,
        THERMAL_BLEND,
        THERMAL_FULL,
        THERMAL_PIP,
    };
    Q_ENUM(ThermalViewMode)

    enum TrackingStatus {
        TRACKING_UNKNOWN = 0,
        TRACKING_SUPPORTED = 1,
        TRACKING_ENABLED = 2,
        TRACKING_RECTANGLE = 4,
        TRACKING_POINT = 8
    };
    Q_ENUM(TrackingStatus)

    Q_INVOKABLE virtual void setCameraModeVideo() = 0;
    Q_INVOKABLE virtual void setCameraModePhoto() = 0;
    Q_INVOKABLE virtual void toggleCameraMode() = 0;
    Q_INVOKABLE virtual bool takePhoto() = 0;
    Q_INVOKABLE virtual bool stopTakePhoto() = 0;
    Q_INVOKABLE virtual bool startVideoRecording() = 0;
    Q_INVOKABLE virtual bool stopVideoRecording() = 0;
    Q_INVOKABLE virtual bool toggleVideoRecording() = 0;
    Q_INVOKABLE virtual void resetSettings() = 0;
    Q_INVOKABLE virtual void formatCard(int id = 1) = 0;
    Q_INVOKABLE virtual void stepZoom(int direction) = 0;
    Q_INVOKABLE virtual void startZoom(int direction) = 0;
    Q_INVOKABLE virtual void stopZoom() = 0;
    Q_INVOKABLE virtual void stopStream() = 0;
    Q_INVOKABLE virtual void resumeStream() = 0;
    Q_INVOKABLE virtual void startTracking(QRectF rec) = 0;
    Q_INVOKABLE virtual void startTracking(QPointF point, double radius) = 0;
    Q_INVOKABLE virtual void stopTracking() = 0;

    virtual int version() const = 0;
    virtual QString modelName() const = 0;
    virtual QString vendor() const = 0;
    virtual QString firmwareVersion() const = 0;
    virtual qreal focalLength() const = 0;
    virtual QSizeF sensorSize() const = 0;
    virtual QSize resolution() const = 0;
    virtual bool capturesVideo() const = 0;
    virtual bool capturesPhotos() const = 0;
    virtual bool hasModes() const = 0;
    virtual bool hasZoom() const = 0;
    virtual bool hasFocus() const = 0;
    virtual bool hasTracking() const = 0;
    virtual bool hasVideoStream() const = 0;
    virtual bool photosInVideoMode() const = 0;
    virtual bool videoInPhotoMode() const = 0;
    virtual CaptureVideoState captureVideoState() const = 0;
    virtual CapturePhotosState capturePhotosState() const = 0;

    virtual int compID() const = 0;
    virtual bool isBasic() const = 0;
    virtual PhotoCaptureMode photoCaptureMode() const { return _photoCaptureMode; }
    virtual qreal photoLapse() const { return _photoLapse; }
    virtual int photoLapseCount() const { return _photoLapseCount; }
    virtual CameraMode cameraMode() const { return _cameraMode; }
    virtual StorageStatus storageStatus() const = 0;
    virtual QStringList activeSettings() const = 0;
    virtual quint32 storageFree() const = 0;
    virtual QString storageFreeStr() const = 0;
    virtual quint32 storageTotal() const = 0;
    virtual int batteryRemaining() const = 0;
    virtual QString batteryRemainingStr() const = 0;
    virtual bool paramComplete() const = 0;
    virtual qreal zoomLevel() const = 0;
    virtual qreal focusLevel() const = 0;

    virtual QmlObjectListModel *streams() = 0;
    virtual QGCVideoStreamInfo *currentStreamInstance() = 0;
    virtual QGCVideoStreamInfo *thermalStreamInstance() = 0;
    virtual int currentStream() const = 0;
    virtual void setCurrentStream(int stream) = 0;
    virtual bool autoStream() const = 0;
    virtual quint32 recordTime() const = 0;
    virtual QString recordTimeStr() const = 0;

    virtual Fact *exposureMode() = 0;
    virtual Fact *ev() = 0;
    virtual Fact *iso() = 0;
    virtual Fact *shutterSpeed() = 0;
    virtual Fact *aperture() = 0;
    virtual Fact *wb() = 0;
    virtual Fact *mode() = 0;

    virtual QStringList streamLabels() const = 0; ///< Stream names to show the user (for selection)

    virtual ThermalViewMode thermalMode() const = 0;
    virtual void setThermalMode(ThermalViewMode mode) = 0;
    virtual double thermalOpacity() const = 0;
    virtual void setThermalOpacity(double val) = 0;

    virtual void setZoomLevel(qreal level) = 0;
    virtual void setFocusLevel(qreal level) = 0;
    virtual void setCameraMode(CameraMode cameraMode) = 0;
    virtual void setPhotoCaptureMode(PhotoCaptureMode mode) = 0;
    virtual void setPhotoLapse(qreal interval) = 0;
    virtual void setPhotoLapseCount(int count) = 0;

    virtual bool trackingEnabled() const = 0;
    virtual void setTrackingEnabled(bool set) = 0;

    virtual TrackingStatus trackingStatus() const = 0;

    virtual bool trackingImageStatus() const = 0;
    virtual QRectF trackingImageRect() const = 0;

    virtual void factChanged(Fact *pFact) = 0;                              ///< Notify controller a parameter has changed
    virtual bool incomingParameter(Fact *pFact, QVariant &newValue) = 0;    ///< Allow controller to modify or invalidate incoming parameter
    virtual bool validateParameter(Fact *pFact, QVariant &newValue) = 0;    ///< Allow controller to modify or invalidate parameter change

    virtual void handleBatteryStatus(const mavlink_battery_status_t &bs) = 0;
    virtual void handleCameraCaptureStatus(const mavlink_camera_capture_status_t &cameraCaptureStatus) = 0;
    virtual void handleParamExtAck(const mavlink_param_ext_ack_t &paramExtAck) = 0;
    virtual void handleParamExtValue(const mavlink_param_ext_value_t &paramExtValue) = 0;
    virtual void handleCameraSettings(const mavlink_camera_settings_t &settings) = 0;
    virtual void handleStorageInformation(const mavlink_storage_information_t &storageInformation) = 0;
    virtual void handleTrackingImageStatus(const mavlink_camera_tracking_image_status_t &trackingImageStatus) = 0;
    virtual void handleVideoStreamInformation(const mavlink_video_stream_information_t &videoStreamInformation) = 0;
    virtual void handleVideoStreamStatus(const mavlink_video_stream_status_t &videoStreamStatus) = 0;

    QString cameraModeToStr(CameraMode mode);
    QString captureImageStatusToStr(uint8_t image_status);
    QString captureVideoStatusToStr(uint8_t video_status);
    QString storageStatusToStr(uint8_t status);

signals:
    void infoChanged();
    void videoCaptureStatusChanged();
    void photoCaptureStatusChanged();
    void photoCaptureModeChanged();
    void photoLapseChanged();
    void photoLapseCountChanged();
    void cameraModeChanged();
    void activeSettingsChanged();
    void storageFreeChanged();
    void storageTotalChanged();
    void batteryRemainingChanged();
    void dataReady(const QByteArray &data);
    void parametersReady();
    void zoomLevelChanged();
    void focusLevelChanged();
    void streamsChanged();
    void currentStreamChanged();
    void thermalStreamChanged();
    void autoStreamChanged();
    void recordTimeChanged();
    void streamLabelsChanged();
    void trackingEnabledChanged();
    void trackingImageStatusChanged();
    void thermalModeChanged();
    void thermalOpacityChanged();
    void storageStatusChanged();
    void captureVideoStateChanged();
    void capturePhotosStateChanged();

protected slots:
    virtual void _paramDone() = 0;

protected:
    enum VideoCaptureStatus {
        VIDEO_CAPTURE_STATUS_STOPPED = 0,
        VIDEO_CAPTURE_STATUS_RUNNING,
        VIDEO_CAPTURE_STATUS_LAST,
        VIDEO_CAPTURE_STATUS_UNDEFINED = 255
    };

    enum PhotoCaptureStatus {
        PHOTO_CAPTURE_IDLE = 0,
        PHOTO_CAPTURE_IN_PROGRESS,
        PHOTO_CAPTURE_INTERVAL_IDLE,
        PHOTO_CAPTURE_INTERVAL_IN_PROGRESS,
        PHOTO_CAPTURE_LAST,
        PHOTO_CAPTURE_STATUS_UNDEFINED = 255
    };

    VideoCaptureStatus _videoCaptureStatus() const { return _videoCaptureStatusValue; }
    PhotoCaptureStatus _photoCaptureStatus() const { return _photoCaptureStatusValue; }

    Vehicle *_vehicle = nullptr;
    CameraMode _cameraMode = CAM_MODE_UNDEFINED;
    VideoCaptureStatus _videoCaptureStatusValue = VIDEO_CAPTURE_STATUS_STOPPED;
    PhotoCaptureStatus _photoCaptureStatusValue = PHOTO_CAPTURE_IDLE;
    PhotoCaptureMode _photoCaptureMode = PHOTO_CAPTURE_SINGLE;
    qreal _photoLapse = 1.0;
    int _photoLapseCount = 0;
    QTimer _videoRecordTimeUpdateTimer;
};
