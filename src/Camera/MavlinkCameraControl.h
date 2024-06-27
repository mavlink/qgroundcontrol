/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "FactGroup.h"

#include <QtCore/QObject>
#include <QtCore/QSizeF>
#include <QtCore/QRectF>
#include <QtCore/QPointF>
#include <QtCore/QLoggingCategory>

class QGCCameraParamIO;
class QmlObjectListModel;

Q_DECLARE_LOGGING_CATEGORY(CameraControlLog)
Q_DECLARE_LOGGING_CATEGORY(CameraControlVerboseLog)

//-----------------------------------------------------------------------------
/// Video Stream Info
/// Encapsulates the contents of a [VIDEO_STREAM_INFORMATION](https://mavlink.io/en/messages/common.html#VIDEO_STREAM_INFORMATION) message
class QGCVideoStreamInfo : public QObject
{
    Q_OBJECT
    Q_MOC_INCLUDE("QmlObjectListModel.h")
public:
    QGCVideoStreamInfo(QObject* parent, const mavlink_video_stream_information_t* si);

    Q_PROPERTY(QString      uri                 READ uri                NOTIFY infoChanged)
    Q_PROPERTY(QString      name                READ name               NOTIFY infoChanged)
    Q_PROPERTY(int          streamID            READ streamID           NOTIFY infoChanged)
    Q_PROPERTY(int          type                READ type               NOTIFY infoChanged)
    Q_PROPERTY(qreal        aspectRatio         READ aspectRatio        NOTIFY infoChanged)
    Q_PROPERTY(qreal        hfov                READ hfov               NOTIFY infoChanged)
    Q_PROPERTY(bool         isThermal           READ isThermal          NOTIFY infoChanged)

    QString uri             () const { return QString(_streamInfo.uri);  }
    QString name            () const { return QString(_streamInfo.name); }
    qreal   aspectRatio     () const;
    qreal   hfov            () const{ return _streamInfo.hfov; }
    int     type            () const{ return _streamInfo.type; }
    int     streamID        () const{ return _streamInfo.stream_id; }
    bool    isThermal       () const{ return _streamInfo.flags & VIDEO_STREAM_STATUS_FLAGS_THERMAL; }

    bool    update          (const mavlink_video_stream_status_t* vs);

signals:
    void    infoChanged     ();

private:
    mavlink_video_stream_information_t _streamInfo;
};


/// Abstract base class for all camera controls: real and simulated
class MavlinkCameraControl : public FactGroup
{
    Q_OBJECT
    friend class QGCCameraParamIO;

public:
    MavlinkCameraControl(QObject* parent = nullptr);
    virtual ~MavlinkCameraControl();

    enum CameraMode {
        CAM_MODE_UNDEFINED = -1,
        CAM_MODE_PHOTO  = 0,
        CAM_MODE_VIDEO  = 1,
        CAM_MODE_SURVEY = 2,
    };

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

    enum PhotoCaptureMode {
        PHOTO_CAPTURE_SINGLE = 0,
        PHOTO_CAPTURE_TIMELAPSE,
    };

    enum StorageStatus {
        STORAGE_EMPTY = STORAGE_STATUS_EMPTY,
        STORAGE_UNFORMATTED = STORAGE_STATUS_UNFORMATTED,
        STORAGE_READY = STORAGE_STATUS_READY,
        STORAGE_NOT_SUPPORTED = STORAGE_STATUS_NOT_SUPPORTED
    };

    enum ThermalViewMode {
        THERMAL_OFF = 0,
        THERMAL_BLEND,
        THERMAL_FULL,
        THERMAL_PIP,
    };

    enum TrackingStatus {
        TRACKING_UNKNOWN        = 0,
        TRACKING_SUPPORTED      = 1,
        TRACKING_ENABLED        = 2,
        TRACKING_RECTANGLE      = 4,
        TRACKING_POINT          = 8
    };

    Q_ENUM(CameraMode)
    Q_ENUM(VideoCaptureStatus)
    Q_ENUM(PhotoCaptureStatus)
    Q_ENUM(PhotoCaptureMode)
    Q_ENUM(StorageStatus)
    Q_ENUM(ThermalViewMode)
    Q_ENUM(TrackingStatus)

    Q_PROPERTY(int                  version                 READ version                                            NOTIFY infoChanged)
    Q_PROPERTY(QString              modelName               READ modelName                                          NOTIFY infoChanged)
    Q_PROPERTY(QString              vendor                  READ vendor                                             NOTIFY infoChanged)
    Q_PROPERTY(QString              firmwareVersion         READ firmwareVersion                                    NOTIFY infoChanged)
    Q_PROPERTY(qreal                focalLength             READ focalLength                                        NOTIFY infoChanged)
    Q_PROPERTY(QSizeF               sensorSize              READ sensorSize                                         NOTIFY infoChanged)
    Q_PROPERTY(QSize                resolution              READ resolution                                         NOTIFY infoChanged)
    Q_PROPERTY(bool                 capturesVideo           READ capturesVideo                                      NOTIFY infoChanged)
    Q_PROPERTY(bool                 capturesPhotos          READ capturesPhotos                                     NOTIFY infoChanged)
    Q_PROPERTY(bool                 hasModes                READ hasModes                                           NOTIFY infoChanged)
    Q_PROPERTY(bool                 hasZoom                 READ hasZoom                                            NOTIFY infoChanged)
    Q_PROPERTY(bool                 hasFocus                READ hasFocus                                           NOTIFY infoChanged)
    Q_PROPERTY(bool                 hasVideoStream          READ hasVideoStream                                     NOTIFY infoChanged)
    Q_PROPERTY(bool                 hasTracking             READ hasTracking                                        NOTIFY infoChanged)
    Q_PROPERTY(bool                 photosInVideoMode       READ photosInVideoMode                                  NOTIFY infoChanged)
    Q_PROPERTY(bool                 videoInPhotoMode        READ videoInPhotoMode                                   NOTIFY infoChanged)
    Q_PROPERTY(bool                 isBasic                 READ isBasic                                            NOTIFY infoChanged)

    Q_PROPERTY(quint32              storageFree             READ storageFree                                        NOTIFY storageFreeChanged)
    Q_PROPERTY(QString              storageFreeStr          READ storageFreeStr                                     NOTIFY storageFreeChanged)
    Q_PROPERTY(quint32              storageTotal            READ storageTotal                                       NOTIFY storageTotalChanged)
    Q_PROPERTY(int                  batteryRemaining        READ batteryRemaining                                   NOTIFY batteryRemainingChanged)
    Q_PROPERTY(QString              batteryRemainingStr     READ batteryRemainingStr                                NOTIFY batteryRemainingChanged)
    Q_PROPERTY(bool                 paramComplete           READ paramComplete                                      NOTIFY parametersReady)
    Q_PROPERTY(qreal                zoomLevel               READ zoomLevel              WRITE  setZoomLevel         NOTIFY zoomLevelChanged)
    Q_PROPERTY(qreal                focusLevel              READ focusLevel             WRITE  setFocusLevel        NOTIFY focusLevelChanged)
    Q_PROPERTY(QStringList          activeSettings          READ activeSettings                                     NOTIFY activeSettingsChanged)
    Q_PROPERTY(VideoCaptureStatus   videoCaptureStatus      READ videoCaptureStatus                                 NOTIFY videoCaptureStatusChanged)
    Q_PROPERTY(PhotoCaptureStatus   photoCaptureStatus      READ photoCaptureStatus                                 NOTIFY photoCaptureStatusChanged)
    Q_PROPERTY(CameraMode           cameraMode              READ cameraMode             WRITE   setCameraMode       NOTIFY cameraModeChanged)
    Q_PROPERTY(StorageStatus        storageStatus           READ storageStatus                                      NOTIFY storageStatusChanged)
    Q_PROPERTY(qreal                photoLapse              READ photoLapse             WRITE   setPhotoLapse       NOTIFY photoLapseChanged)
    Q_PROPERTY(int                  photoLapseCount         READ photoLapseCount        WRITE   setPhotoLapseCount  NOTIFY photoLapseCountChanged)
    Q_PROPERTY(PhotoCaptureMode     photoCaptureMode        READ photoCaptureMode       WRITE   setPhotoCaptureMode NOTIFY photoCaptureModeChanged)
    Q_PROPERTY(int                  currentStream           READ currentStream          WRITE   setCurrentStream    NOTIFY currentStreamChanged)
    Q_PROPERTY(bool                 autoStream              READ autoStream                                         NOTIFY autoStreamChanged)
    Q_PROPERTY(QmlObjectListModel*  streams                 READ streams                                            NOTIFY streamsChanged)
    Q_PROPERTY(QGCVideoStreamInfo*  currentStreamInstance   READ currentStreamInstance                              NOTIFY currentStreamChanged)
    Q_PROPERTY(QGCVideoStreamInfo*  thermalStreamInstance   READ thermalStreamInstance                              NOTIFY thermalStreamChanged)
    Q_PROPERTY(quint32              recordTime              READ recordTime                                         NOTIFY recordTimeChanged)
    Q_PROPERTY(QString              recordTimeStr           READ recordTimeStr                                      NOTIFY recordTimeChanged)
    Q_PROPERTY(QStringList          streamLabels            READ streamLabels                                       NOTIFY streamLabelsChanged)
    Q_PROPERTY(ThermalViewMode      thermalMode             READ thermalMode            WRITE  setThermalMode       NOTIFY thermalModeChanged)
    Q_PROPERTY(double               thermalOpacity          READ thermalOpacity         WRITE  setThermalOpacity    NOTIFY thermalOpacityChanged)
    Q_PROPERTY(bool                 trackingEnabled         READ trackingEnabled        WRITE setTrackingEnabled    NOTIFY trackingEnabledChanged)
    Q_PROPERTY(TrackingStatus       trackingStatus          READ trackingStatus                                     CONSTANT)
    Q_PROPERTY(bool                 trackingImageStatus     READ trackingImageStatus                                NOTIFY trackingImageStatusChanged)
    Q_PROPERTY(QRectF               trackingImageRect       READ trackingImageRect                                  NOTIFY trackingImageStatusChanged)

    Q_PROPERTY(Fact*                exposureMode            READ exposureMode                                       NOTIFY parametersReady)
    Q_PROPERTY(Fact*                ev                      READ ev                                                 NOTIFY parametersReady)
    Q_PROPERTY(Fact*                iso                     READ iso                                                NOTIFY parametersReady)
    Q_PROPERTY(Fact*                shutterSpeed            READ shutterSpeed                                       NOTIFY parametersReady)
    Q_PROPERTY(Fact*                aperture                READ aperture                                           NOTIFY parametersReady)
    Q_PROPERTY(Fact*                wb                      READ wb                                                 NOTIFY parametersReady)
    Q_PROPERTY(Fact*                mode                    READ mode                                               NOTIFY parametersReady)

    Q_INVOKABLE virtual void setCameraModeVideo       () = 0;
    Q_INVOKABLE virtual void setCameraModePhoto       () = 0;
    Q_INVOKABLE virtual void toggleCameraMode         () = 0;
    Q_INVOKABLE virtual bool takePhoto          () = 0;
    Q_INVOKABLE virtual bool stopTakePhoto      () = 0;
    Q_INVOKABLE virtual bool startVideoRecording() = 0;
    Q_INVOKABLE virtual bool stopVideoRecording () = 0;
    Q_INVOKABLE virtual bool toggleVideoRecording() = 0;
    Q_INVOKABLE virtual void resetSettings      () = 0;
    Q_INVOKABLE virtual void formatCard         (int id = 1) = 0;
    Q_INVOKABLE virtual void stepZoom           (int direction) = 0;
    Q_INVOKABLE virtual void startZoom          (int direction) = 0;
    Q_INVOKABLE virtual void stopZoom           () = 0;
    Q_INVOKABLE virtual void stopStream         () = 0;
    Q_INVOKABLE virtual void resumeStream       () = 0;
    Q_INVOKABLE virtual void startTracking      (QRectF rec) = 0;
    Q_INVOKABLE virtual void startTracking      (QPointF point, double radius) = 0;
    Q_INVOKABLE virtual void stopTracking       () = 0;

    virtual int         version             () = 0;
    virtual QString     modelName           () = 0;
    virtual QString     vendor              () = 0;
    virtual QString     firmwareVersion     () = 0;
    virtual qreal       focalLength         () = 0;
    virtual QSizeF      sensorSize          () = 0;
    virtual QSize       resolution          () = 0;
    virtual bool        capturesVideo       () = 0;
    virtual bool        capturesPhotos      () = 0;
    virtual bool        hasModes            () = 0;
    virtual bool        hasZoom             () = 0;
    virtual bool        hasFocus            () = 0;
    virtual bool        hasTracking         () = 0;
    virtual bool        hasVideoStream      () = 0;
    virtual bool        photosInVideoMode   () = 0;
    virtual bool        videoInPhotoMode    () = 0;

    virtual int         compID              () = 0;
    virtual bool        isBasic             () = 0;
    virtual VideoCaptureStatus videoCaptureStatus         () = 0;
    virtual PhotoCaptureStatus photoCaptureStatus         () = 0;
    virtual PhotoCaptureMode   photoCaptureMode           () = 0;
    virtual qreal       photoLapse          () = 0;
    virtual int         photoLapseCount     () = 0;
    virtual CameraMode  cameraMode          () = 0;
    virtual StorageStatus storageStatus     () = 0;
    virtual QStringList activeSettings      () = 0;
    virtual quint32     storageFree         () = 0;
    virtual QString     storageFreeStr      () = 0;
    virtual quint32     storageTotal        () = 0;
    virtual int         batteryRemaining    () = 0;
    virtual QString     batteryRemainingStr () = 0;
    virtual bool        paramComplete       () = 0;
    virtual qreal       zoomLevel           () = 0;
    virtual qreal       focusLevel          () = 0;

    virtual QmlObjectListModel* streams     () = 0;
    virtual QGCVideoStreamInfo* currentStreamInstance() = 0;
    virtual QGCVideoStreamInfo* thermalStreamInstance() = 0;
    virtual int          currentStream      () = 0;
    virtual void         setCurrentStream   (int stream) = 0;
    virtual bool         autoStream         () = 0;
    virtual quint32      recordTime         () = 0;
    virtual QString      recordTimeStr      () = 0;

    virtual Fact*       exposureMode        () = 0;
    virtual Fact*       ev                  () = 0;
    virtual Fact*       iso                 () = 0;
    virtual Fact*       shutterSpeed        () = 0;
    virtual Fact*       aperture            () = 0;
    virtual Fact*       wb                  () = 0;
    virtual Fact*       mode                () = 0;

    
    virtual QStringList streamLabels        () = 0; ///< Stream names to show the user (for selection)

    virtual ThermalViewMode thermalMode     () = 0;
    virtual void        setThermalMode      (ThermalViewMode mode) = 0;
    virtual double      thermalOpacity      () = 0;
    virtual void        setThermalOpacity   (double val) = 0;

    virtual void        setZoomLevel        (qreal level) = 0;
    virtual void        setFocusLevel       (qreal level) = 0;
    virtual void        setCameraMode       (CameraMode mode) = 0;
    virtual void        setPhotoCaptureMode (PhotoCaptureMode mode) = 0;
    virtual void        setPhotoLapse       (qreal interval) = 0;
    virtual void        setPhotoLapseCount  (int count) = 0;

    virtual bool        trackingEnabled     () = 0;
    virtual void        setTrackingEnabled  (bool set) = 0;

    virtual TrackingStatus trackingStatus   () = 0;

    virtual bool trackingImageStatus() = 0;
    virtual QRectF trackingImageRect() = 0;

    virtual void        factChanged         (Fact* pFact) = 0;                      ///< Notify controller a parameter has changed
    virtual bool        incomingParameter   (Fact* pFact, QVariant& newValue) = 0;  ///< Allow controller to modify or invalidate incoming parameter
    virtual bool        validateParameter   (Fact* pFact, QVariant& newValue) = 0;  ///< Allow controller to modify or invalidate parameter change

    virtual void        handleSettings      (const mavlink_camera_settings_t& settings) = 0;
    virtual void        handleCaptureStatus (const mavlink_camera_capture_status_t& capStatus) = 0;
    virtual void        handleParamAck      (const mavlink_param_ext_ack_t& ack) = 0;
    virtual void        handleParamValue    (const mavlink_param_ext_value_t& value) = 0;
    virtual void        handleStorageInfo   (const mavlink_storage_information_t& st) = 0;
    virtual void        handleBatteryStatus (const mavlink_battery_status_t& bs) = 0;
    virtual void        handleTrackingImageStatus(const mavlink_camera_tracking_image_status_t *tis) = 0;
    virtual void        handleVideoInfo     (const mavlink_video_stream_information_t *vi) = 0;
    virtual void        handleVideoStatus   (const mavlink_video_stream_status_t *vs) = 0;

    QString cameraModeToStr         (CameraMode mode);
    QString captureImageStatusToStr (uint8_t image_status);
    QString captureVideoStatusToStr (uint8_t video_status);
    QString storageStatusToStr      (uint8_t status);

signals:
    void    infoChanged                 ();
    void    videoCaptureStatusChanged   ();
    void    photoCaptureStatusChanged   ();
    void    photoCaptureModeChanged     ();
    void    photoLapseChanged           ();
    void    photoLapseCountChanged      ();
    void    cameraModeChanged           ();
    void    activeSettingsChanged       ();
    void    storageFreeChanged          ();
    void    storageTotalChanged         ();
    void    batteryRemainingChanged     ();
    void    dataReady                   (QByteArray data);
    void    parametersReady             ();
    void    zoomLevelChanged            ();
    void    focusLevelChanged           ();
    void    streamsChanged              ();
    void    currentStreamChanged        ();
    void    thermalStreamChanged        ();
    void    autoStreamChanged           ();
    void    recordTimeChanged           ();
    void    streamLabelsChanged         ();
    void    trackingEnabledChanged      ();
    void    trackingImageStatusChanged  ();
    void    thermalModeChanged          ();
    void    thermalOpacityChanged       ();
    void    storageStatusChanged        ();

protected slots:
    virtual void _paramDone() = 0;
};
