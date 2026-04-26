#pragma once

#include "MavlinkCameraControlInterface.h"
#include "QmlObjectListModel.h"

class QGCVideoStreamInfo;
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

/// MAVLink Camera API controller - connected to a real mavlink v2 camera
class VehicleCameraControl : public MavlinkCameraControlInterface
{
public:
    VehicleCameraControl(const mavlink_camera_information_t* info, Vehicle* vehicle, int compID, QObject* parent = nullptr);
    ~VehicleCameraControl() override;

    Q_INVOKABLE void setCameraModeVideo() override;
    Q_INVOKABLE void setCameraModePhoto() override;
    Q_INVOKABLE void toggleCameraMode       () override;
    Q_INVOKABLE bool takePhoto              () override;
    Q_INVOKABLE bool stopTakePhoto          () override;
    Q_INVOKABLE bool startVideoRecording    () override;
    Q_INVOKABLE bool stopVideoRecording     () override;
    Q_INVOKABLE bool toggleVideoRecording   () override;
    Q_INVOKABLE void resetSettings          () override;
    Q_INVOKABLE void formatCard             (int id = 1) override;
    Q_INVOKABLE void stepZoom               (int direction) override;
    Q_INVOKABLE void startZoom              (int direction) override;
    Q_INVOKABLE void stopZoom               () override;
    Q_INVOKABLE void stepFocus              (int direction) override;
    Q_INVOKABLE void startFocus             (int direction) override;
    Q_INVOKABLE void stopFocus              () override;
    Q_INVOKABLE void stopStream             () override;
    Q_INVOKABLE void resumeStream           () override;
    Q_INVOKABLE void startTrackingRect      (QRectF rec) override;
    Q_INVOKABLE void startTrackingPoint     (QPointF point, double radius) override;
    Q_INVOKABLE void stopTracking           () override;

    int         version             () const override { return _version; }
    QString     modelName           () const override { return _modelName; }
    QString     vendor              () const override { return _vendor; }
    QString     firmwareVersion     () const override;
    qreal       focalLength         () const override { return static_cast<qreal>(_mavlinkCameraInfo.focal_length); }
    QSizeF      sensorSize          () const override { return QSizeF(static_cast<qreal>(_mavlinkCameraInfo.sensor_size_h), static_cast<qreal>(_mavlinkCameraInfo.sensor_size_v)); }
    QSize       resolution          () const override { return QSize(_mavlinkCameraInfo.resolution_h, _mavlinkCameraInfo.resolution_v); }
    bool        capturesVideo       () const override;
    bool        capturesPhotos      () const override;
    bool        hasModes            () const override { return _mavlinkCameraInfo.flags & CAMERA_CAP_FLAGS_HAS_MODES; }
    bool        hasZoom             () const override { return _mavlinkCameraInfo.flags & CAMERA_CAP_FLAGS_HAS_BASIC_ZOOM; }
    bool        hasFocus            () const override { return _mavlinkCameraInfo.flags & CAMERA_CAP_FLAGS_HAS_BASIC_FOCUS; }
    bool        hasTracking         () const override { return _hasTrackingRectCapability || _hasTrackingPointCapability; }
    bool        supportsTrackingPoint() const override { return _hasTrackingPointCapability; }
    bool        supportsTrackingRect () const override { return _hasTrackingRectCapability; }
    bool        hasVideoStream      () const override { return _mavlinkCameraInfo.flags & CAMERA_CAP_FLAGS_HAS_VIDEO_STREAM; }
    bool        photosInVideoMode   () const override { return _mavlinkCameraInfo.flags & CAMERA_CAP_FLAGS_CAN_CAPTURE_IMAGE_IN_VIDEO_MODE; }
    bool        videoInPhotoMode    () const override { return _mavlinkCameraInfo.flags & CAMERA_CAP_FLAGS_CAN_CAPTURE_VIDEO_IN_IMAGE_MODE; }
    CaptureVideoState captureVideoState() const override;
    CapturePhotosState capturePhotosState() const override;

    int                 compID              () const override { return _compID; }
    bool                isBasic             () const override { return _settings.size() == 0; }
    StorageStatus       storageStatus       () const override { return _storageStatus; }
    QStringList         activeSettings      () const override;
    quint32             storageFree         () const override { return _storageFree;  }
    QString             storageFreeStr      () const override;
    quint32             storageTotal        () const override { return _storageTotal; }
    int                 batteryRemaining    () const override { return _batteryRemaining; }
    QString             batteryRemainingStr () const override;
    bool                paramComplete       () const override { return _paramComplete; }
    qreal               zoomLevel           () const override { return _zoomLevel; }
    qreal               focusLevel          () const override { return _focusLevel; }

    QmlObjectListModel* streams             () override { return &_streams; }
    QGCVideoStreamInfo* currentStreamInstance() override;
    QGCVideoStreamInfo* thermalStreamInstance() override;
    int                 currentStream       () const override { return _currentStream; }
    void                setCurrentStream    (int stream) override;
    bool                autoStream          () const override;
    quint32             recordTime          () const override { return _recordTime; }
    QString             recordTimeStr       () const override;

    QStringList streamLabels        () const override { return _streamLabels; }

    ThermalViewMode thermalMode     () const override { return _thermalMode; }
    void        setThermalMode      (ThermalViewMode mode) override;
    double      thermalOpacity      () const override { return _thermalOpacity; }
    void        setThermalOpacity   (double val) override;

    void        setZoomLevel        (qreal level) override;
    void        setFocusLevel       (qreal level) override;
    void        setCameraMode(CameraMode cameraMode) override;
    void        setPhotoCaptureMode        (PhotoCaptureMode mode) override;
    void        setPhotoLapse       (qreal interval) override;
    void        setPhotoLapseCount  (int count) override;

    void        handleCameraSettings(const mavlink_camera_settings_t& settings) override;
    void        handleCameraCaptureStatus(const mavlink_camera_capture_status_t& cameraCaptureStatus) override;
    void        handleParamExtAck   (const mavlink_param_ext_ack_t& paramExtAck) override;
    void        handleParamExtValue (const mavlink_param_ext_value_t& paramExtValue) override;
    void        handleStorageInformation(const mavlink_storage_information_t& storageInformation) override;
    void        handleBatteryStatus (const mavlink_battery_status_t& bs) override;
    void        handleTrackingImageStatus(const mavlink_camera_tracking_image_status_t &trackingImageStatus) override;
    void        handleVideoStreamInformation(const mavlink_video_stream_information_t &videoStreamInformation) override;
    void        handleVideoStreamStatus(const mavlink_video_stream_status_t &videoStreamStatus) override;

    bool        trackingEnabled     () const override { return _trackingEnabled; }
    void        setTrackingEnabled  (bool set) override;

    bool trackingImageIsActive() const override { return _trackingImageIsActive; }
    bool trackingImageIsPoint() const override { return _trackingImageIsPoint; }
    QRectF trackingImageRect() const override { return _trackingImageRect; }
    QPointF trackingImagePoint() const override { return _trackingImagePoint; }
    qreal trackingImageRadius() const override { return _trackingImageRadius; }

    Fact*   exposureMode        () override;
    Fact*   ev                  () override;
    Fact*   iso                 () override;
    Fact*   shutterSpeed        () override;
    Fact*   aperture            () override;
    Fact*   wb                  () override;
    Fact*   mode                () override;
    void    factChanged         (Fact* pFact) override;
    bool    incomingParameter   (Fact* pFact, QVariant& newValue) override;
    bool    validateParameter   (Fact* pFact, QVariant& newValue) override;

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
    virtual void    _setVideoCaptureStatus  (VideoCaptureStatus captureStatus);
    virtual void    _setPhotoCaptureStatus  (PhotoCaptureStatus captureStatus);
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
    virtual void    _mavCommandResult       (int vehicleId, int component, int command, int result, int failureCode);
    virtual void    _dataReady              (QByteArray data);
    virtual void    _streamInfoTimeout      ();
    virtual void    _streamStatusTimeout    ();
    virtual void    _cameraSettingsTimeout  ();
    virtual void    _storageInfoTimeout     ();
    virtual void    _recTimerHandler        ();
    virtual void    _checkForVideoStreams   ();
    virtual void    _onVideoManagerRecordingChanged  (bool recording);
    void            _paramDone              () override;

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
    int                                 _compID             = 0;
    mavlink_camera_information_t        _mavlinkCameraInfo;
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
    StorageStatus                       _storageStatus      = STORAGE_NOT_SUPPORTED;
    QStringList                         _activeSettings;
    QStringList                         _settings;
    QTimer                              _captureStatusTimer;
    QList<QGCCameraOptionExclusion*>    _valueExclusions;
    QList<QGCCameraOptionRange*>        _optionRanges;
    QMap<QString, QStringList>          _originalOptNames;
    QMap<QString, QVariantList>         _originalOptValues;
    QMap<QString, QGCCameraParamIO*>    _paramIO;
    int                                 _cameraSettingsRetries = 0;
    int                                 _cameraCaptureStatusRetries = 0;
    int                                 _storageInfoRetries = 0;
    int                                 _captureInfoRetries = 0;
    bool                                _resetting          = false;
    QTime                               _recTime;
    uint32_t                            _recordTime         = 0;
    //-- Parameters that require a full update
    QMap<QString, QStringList>          _requestUpdates;
    QStringList                         _updatesToRequest;
    //-- Video Streams
    int                                 _videoStreamInfoRetries   = 0;
    int                                 _videoStreamStatusRetries = 0;
    int                                 _requestCount       = 0;
    int                                 _currentStream      = 0;
    int                                 _expectedCount      = 1;
    QTimer                              _streamInfoTimer;
    QTimer                              _streamStatusTimer;
    QTimer                              _cameraSettingsTimer;
    QTimer                              _storageInfoTimer;
    QmlObjectListModel                  _streams;
    QStringList                         _streamLabels;
    ThermalViewMode                     _thermalMode        = THERMAL_BLEND;
    double                              _thermalOpacity     = 85.0;
    bool                                _hasTrackingRectCapability = false;
    bool                                _hasTrackingPointCapability = false;
    bool                                _trackingEnabled      = false;
    bool                                    _trackingImageIsActive = false;
    bool                                    _trackingImageIsPoint = false;
    mavlink_camera_tracking_image_status_t  _trackingImageStatus{};
    QRectF                                  _trackingImageRect;
    QPointF                                 _trackingImagePoint;
    qreal                                   _trackingImageRadius = 0.0;
};
