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
#include "QmlObjectListModel.h"

class QNetworkAccessManager;
class QDomNode;
class QDomNodeList;

//-----------------------------------------------------------------------------
/// Camera option exclusions
class QGCCameraOptionExclusion : public QObject
{
public:
    QGCCameraOptionExclusion(QObject* parent, QString param_, QString value_, QStringList exclusions_);
    QString param;
    QString value;
    QStringList exclusions;
};

//-----------------------------------------------------------------------------
/// Camera option ranges
class QGCCameraOptionRange : public QObject
{
public:
    QGCCameraOptionRange(QObject* parent, QString param_, QString value_, QString targetParam_, QString condition_, QStringList optNames_, QStringList optValues_);
    QString param;
    QString value;
    QString targetParam;
    QString condition;
    QStringList  optNames;
    QStringList  optValues;
    QVariantList optVariants;
};

//-----------------------------------------------------------------------------
/// MAVLink Camera API controller
class VehicleCameraControl : public MavlinkCameraControl
{
public:
    VehicleCameraControl(const mavlink_camera_information_t* info, Vehicle* vehicle, int compID, QObject* parent = nullptr);
    virtual ~VehicleCameraControl();

    Q_INVOKABLE virtual void setCameraModeVideo     ();
    Q_INVOKABLE virtual void setCameraModePhoto     ();
    Q_INVOKABLE virtual void toggleCameraMode       ();
    Q_INVOKABLE virtual bool takePhoto              ();
    Q_INVOKABLE virtual bool stopTakePhoto          ();
    Q_INVOKABLE virtual bool startVideoRecording    ();
    Q_INVOKABLE virtual bool stopVideoRecording     ();
    Q_INVOKABLE virtual bool toggleVideoRecording   ();
    Q_INVOKABLE virtual void resetSettings          ();
    Q_INVOKABLE virtual void formatCard             (int id = 1);
    Q_INVOKABLE virtual void stepZoom               (int direction);
    Q_INVOKABLE virtual void startZoom              (int direction);
    Q_INVOKABLE virtual void stopZoom               ();
    Q_INVOKABLE virtual void stopStream             ();
    Q_INVOKABLE virtual void resumeStream           ();
    Q_INVOKABLE virtual void startTracking          (QRectF rec);
    Q_INVOKABLE virtual void startTracking          (QPointF point, double radius);
    Q_INVOKABLE virtual void stopTracking           ();

    virtual int         version             () { return _version; }
    virtual QString     modelName           () { return _modelName; }
    virtual QString     vendor              () { return _vendor; }
    virtual QString     firmwareVersion     ();
    virtual qreal       focalLength         () { return static_cast<qreal>(_info.focal_length); }
    virtual QSizeF      sensorSize          () { return QSizeF(static_cast<qreal>(_info.sensor_size_h), static_cast<qreal>(_info.sensor_size_v)); }
    virtual QSize       resolution          () { return QSize(_info.resolution_h, _info.resolution_v); }
    virtual bool        capturesVideo       () { return _info.flags & CAMERA_CAP_FLAGS_CAPTURE_VIDEO; }
    virtual bool        capturesPhotos      () { return _info.flags & CAMERA_CAP_FLAGS_CAPTURE_IMAGE; }
    virtual bool        hasModes            () { return _info.flags & CAMERA_CAP_FLAGS_HAS_MODES; }
    virtual bool        hasZoom             () { return _info.flags & CAMERA_CAP_FLAGS_HAS_BASIC_ZOOM; }
    virtual bool        hasFocus            () { return _info.flags & CAMERA_CAP_FLAGS_HAS_BASIC_FOCUS; }
    virtual bool        hasTracking         () { return _trackingStatus & TRACKING_SUPPORTED; }
    virtual bool        hasVideoStream      () { return _info.flags & CAMERA_CAP_FLAGS_HAS_VIDEO_STREAM; }
    virtual bool        photosInVideoMode   () { return _info.flags & CAMERA_CAP_FLAGS_CAN_CAPTURE_IMAGE_IN_VIDEO_MODE; }
    virtual bool        videoInPhotoMode    () { return _info.flags & CAMERA_CAP_FLAGS_CAN_CAPTURE_VIDEO_IN_IMAGE_MODE; }

    virtual int                 compID              () { return _compID; }
    virtual bool                isBasic             () { return _settings.size() == 0; }
    virtual VideoCaptureStatus  videoCaptureStatus  ();
    virtual PhotoCaptureStatus  photoCaptureStatus  ();
    virtual PhotoCaptureMode    photoCaptureMode    () { return _photoMode; }
    virtual qreal               photoLapse          () { return _photoLapse; }
    virtual int                 photoLapseCount     () { return _photoLapseCount; }
    virtual CameraMode          cameraMode          () { return _cameraMode; }
    virtual StorageStatus       storageStatus       () { return _storageStatus; }
    virtual QStringList         activeSettings      ();
    virtual quint32             storageFree         () { return _storageFree;  }
    virtual QString             storageFreeStr      ();
    virtual quint32             storageTotal        () { return _storageTotal; }
    virtual int                 batteryRemaining    () { return _batteryRemaining; }
    virtual QString             batteryRemainingStr ();
    virtual bool                paramComplete       () { return _paramComplete; }
    virtual qreal               zoomLevel           () { return _zoomLevel; }
    virtual qreal               focusLevel          () { return _focusLevel; }

    virtual QmlObjectListModel* streams             () { return &_streams; }
    virtual QGCVideoStreamInfo* currentStreamInstance();
    virtual QGCVideoStreamInfo* thermalStreamInstance();
    virtual int                 currentStream       () { return _currentStream; }
    virtual void                setCurrentStream    (int stream);
    virtual bool                autoStream          ();
    virtual quint32             recordTime          () { return _recordTime; }
    virtual QString             recordTimeStr       ();

    virtual QStringList streamLabels        () { return _streamLabels; }

    virtual ThermalViewMode thermalMode     () { return _thermalMode; }
    virtual void        setThermalMode      (ThermalViewMode mode);
    virtual double      thermalOpacity      () { return _thermalOpacity; }
    virtual void        setThermalOpacity   (double val);

    virtual void        setZoomLevel        (qreal level);
    virtual void        setFocusLevel       (qreal level);
    virtual void        setCameraMode       (CameraMode mode);
    virtual void        setPhotoCaptureMode        (PhotoCaptureMode mode);
    virtual void        setPhotoLapse       (qreal interval);
    virtual void        setPhotoLapseCount  (int count);

    virtual void        handleSettings      (const mavlink_camera_settings_t& settings);
    virtual void        handleCaptureStatus (const mavlink_camera_capture_status_t& capStatus);
    virtual void        handleParamAck      (const mavlink_param_ext_ack_t& ack);
    virtual void        handleParamValue    (const mavlink_param_ext_value_t& value);
    virtual void        handleStorageInfo   (const mavlink_storage_information_t& st);
    virtual void        handleBatteryStatus (const mavlink_battery_status_t& bs);
    virtual void        handleTrackingImageStatus(const mavlink_camera_tracking_image_status_t *tis);
    virtual void        handleVideoInfo     (const mavlink_video_stream_information_t *vi);
    virtual void        handleVideoStatus   (const mavlink_video_stream_status_t *vs);

    virtual bool        trackingEnabled     () { return _trackingStatus & TRACKING_ENABLED; }
    virtual void        setTrackingEnabled  (bool set);

    virtual TrackingStatus trackingStatus   () { return _trackingStatus; }

    virtual bool trackingImageStatus() { return _trackingImageStatus.tracking_status == 1; }
    virtual QRectF trackingImageRect() { return _trackingImageRect; }

    virtual Fact*   exposureMode        ();
    virtual Fact*   ev                  ();
    virtual Fact*   iso                 ();
    virtual Fact*   shutterSpeed        ();
    virtual Fact*   aperture            ();
    virtual Fact*   wb                  ();
    virtual Fact*   mode                ();   
    virtual void    factChanged         (Fact* pFact);
    virtual bool    incomingParameter   (Fact* pFact, QVariant& newValue);
    virtual bool    validateParameter   (Fact* pFact, QVariant& newValue);

    static constexpr const char* kCondition       = "condition";
    static constexpr const char* kControl         = "control";
    static constexpr const char* kDefault         = "default";
    static constexpr const char* kDefnition       = "definition";
    static constexpr const char* kDescription     = "description";
    static constexpr const char* kExclusion       = "exclude";
    static constexpr const char* kExclusions      = "exclusions";
    static constexpr const char* kLocale          = "locale";
    static constexpr const char* kLocalization    = "localization";
    static constexpr const char* kMax             = "max";
    static constexpr const char* kMin             = "min";
    static constexpr const char* kModel           = "model";
    static constexpr const char* kName            = "name";
    static constexpr const char* kOption          = "option";
    static constexpr const char* kOptions         = "options";
    static constexpr const char* kOriginal        = "original";
    static constexpr const char* kParameter       = "parameter";
    static constexpr const char* kParameterrange  = "parameterrange";
    static constexpr const char* kParameterranges = "parameterranges";
    static constexpr const char* kParameters      = "parameters";
    static constexpr const char* kReadOnly        = "readonly";
    static constexpr const char* kWriteOnly       = "writeonly";
    static constexpr const char* kRoption         = "roption";
    static constexpr const char* kStep            = "step";
    static constexpr const char* kDecimalPlaces   = "decimalPlaces";
    static constexpr const char* kStrings         = "strings";
    static constexpr const char* kTranslated      = "translated";
    static constexpr const char* kType            = "type";
    static constexpr const char* kUnit            = "unit";
    static constexpr const char* kUpdate          = "update";
    static constexpr const char* kUpdates         = "updates";
    static constexpr const char* kValue           = "value";
    static constexpr const char* kVendor          = "vendor";
    static constexpr const char* kVersion         = "version";

    static constexpr const char* kPhotoMode       = "PhotoCaptureMode";
    static constexpr const char* kPhotoLapse      = "PhotoLapse";
    static constexpr const char* kPhotoLapseCount = "PhotoLapseCount";
    static constexpr const char* kThermalOpacity  = "ThermalOpacity";
    static constexpr const char* kThermalMode     = "ThermalMode";

    //-----------------------------------------------------------------------------
    // Known Parameters
    static constexpr const char* kCAM_EV          = "CAM_EV";
    static constexpr const char* kCAM_EXPMODE     = "CAM_EXPMODE";
    static constexpr const char* kCAM_ISO         = "CAM_ISO";
    static constexpr const char* kCAM_SHUTTERSPD  = "CAM_SHUTTERSPD";
    static constexpr const char* kCAM_APERTURE    = "CAM_APERTURE";
    static constexpr const char* kCAM_WBMODE      = "CAM_WBMODE";
    static constexpr const char* kCAM_MODE        = "CAM_MODE";

protected:
    virtual void    _setVideoStatus         (VideoCaptureStatus status);
    virtual void    _setPhotoStatus         (PhotoCaptureStatus status);
    virtual void    _setCameraMode          (CameraMode mode);
    virtual void    _requestStreamInfo      (uint8_t streamID);
    virtual void    _requestStreamStatus    (uint8_t streamID);
    virtual void    _requestTrackingStatus  ();
    virtual QGCVideoStreamInfo* _findStream (uint8_t streamID, bool report = true);
    virtual QGCVideoStreamInfo* _findStream (const QString name);

protected slots:
    virtual void    _initWhenReady          ();
    virtual void    _requestCameraSettings  ();
    virtual void    _requestAllParameters   ();
    virtual void    _requestParamUpdates    ();
    virtual void    _requestCaptureStatus   ();
    virtual void    _requestStorageInfo     ();
    virtual void    _downloadFinished       ();
    virtual void    _mavCommandResult       (int vehicleId, int component, int command, int result, bool noReponseFromVehicle);
    virtual void    _dataReady              (QByteArray data);
    virtual void    _paramDone              ();
    virtual void    _streamInfoTimeout      ();
    virtual void    _streamStatusTimeout    ();
    virtual void    _recTimerHandler        ();
    virtual void    _checkForVideoStreams   ();

private:
    bool    _handleLocalization             (QByteArray& bytes);
    bool    _replaceLocaleStrings           (const QDomNode node, QByteArray& bytes);
    bool    _loadCameraDefinitionFile       (QByteArray& bytes);
    bool    _loadConstants                  (const QDomNodeList nodeList);
    bool    _loadSettings                   (const QDomNodeList nodeList);
    void    _processRanges                  ();
    bool    _processCondition               (const QString condition);
    bool    _processConditionTest           (const QString conditionTest);
    bool    _loadNameValue                  (QDomNode option, const QString factName, FactMetaData* metaData, QString& optName, QString& optValue, QVariant& optVariant);
    bool    _loadRanges                     (QDomNode option, const QString factName, QString paramValue);
    void    _updateActiveList               ();
    void    _updateRanges                   (Fact* pFact);
    void    _httpRequest                    (const QString& url);
    void    _handleDefinitionFile           (const QString& url);
    void    _ftpDownloadComplete            (const QString& fileName, const QString& errorMsg);

    QStringList     _loadExclusions         (QDomNode option);
    QStringList     _loadUpdates            (QDomNode option);
    QString         _getParamName           (const char* param_id);

protected:
    Vehicle*                            _vehicle            = nullptr;
    int                                 _compID             = 0;
    mavlink_camera_information_t        _info;
    int                                 _version            = 0;
    bool                                _cached             = false;
    bool                                _paramComplete      = false;
    qreal                               _zoomLevel          = 0.0;
    qreal                               _focusLevel         = 0.0;
    uint32_t                            _storageFree        = 0;
    uint32_t                            _storageTotal       = 0;
    int                                 _batteryRemaining   = -1;
    QNetworkAccessManager*              _netManager         = nullptr;
    QString                             _modelName;
    QString                             _vendor;
    QString                             _cacheFile;
    CameraMode                          _cameraMode         = CAM_MODE_UNDEFINED;
    StorageStatus                       _storageStatus      = STORAGE_NOT_SUPPORTED;
    PhotoCaptureMode                    _photoMode          = PHOTO_CAPTURE_SINGLE;
    qreal                               _photoLapse         = 1.0;
    int                                 _photoLapseCount    = 0;
    VideoCaptureStatus                  _video_status       = VIDEO_CAPTURE_STATUS_UNDEFINED;
    PhotoCaptureStatus                  _photo_status       = PHOTO_CAPTURE_STATUS_UNDEFINED;
    QStringList                         _activeSettings;
    QStringList                         _settings;
    QTimer                              _captureStatusTimer;
    QList<QGCCameraOptionExclusion*>    _valueExclusions;
    QList<QGCCameraOptionRange*>        _optionRanges;
    QMap<QString, QStringList>          _originalOptNames;
    QMap<QString, QVariantList>         _originalOptValues;
    QMap<QString, QGCCameraParamIO*>    _paramIO;
    int                                 _cameraSettingsRetries = 0;
    int                                 _storageInfoRetries = 0;
    int                                 _captureInfoRetries = 0;
    bool                                _resetting          = false;
    QTimer                              _recTimer;
    QTime                               _recTime;
    uint32_t                            _recordTime         = 0;
    //-- Parameters that require a full update
    QMap<QString, QStringList>          _requestUpdates;
    QStringList                         _updatesToRequest;
    //-- Video Streams
    int                                 _videoStreamInfoRetries = 0;
    int                                 _videoStreamStatusRetries = 0;
    int                                 _currentStream      = 0;
    int                                 _expectedCount      = 1;
    QTimer                              _streamInfoTimer;
    QTimer                              _streamStatusTimer;
    QmlObjectListModel                  _streams;
    QStringList                         _streamLabels;
    ThermalViewMode                     _thermalMode        = THERMAL_BLEND;
    double                              _thermalOpacity     = 85.0;
    TrackingStatus                      _trackingStatus     = TRACKING_UNKNOWN;
    QRectF                              _trackingMarquee;
    QPointF                             _trackingPoint;
    double                              _trackingRadius     = 0.0;
    mavlink_camera_tracking_image_status_t  _trackingImageStatus;
    QRectF                                  _trackingImageRect;
};
