/*!
 * @file
 *   @brief Camera Controller
 *   @author Gus Grubba <gus@auterion.com>
 *
 */

/// @file
/// @brief  MAVLink Camera API
/// @author Gus Grubba <gus@auterion.com>

#pragma once

#include "QGCApplication.h"
#include <QLoggingCategory>

class QDomNode;
class QDomNodeList;
class QGCCameraParamIO;

Q_DECLARE_LOGGING_CATEGORY(CameraControlLog)
Q_DECLARE_LOGGING_CATEGORY(CameraControlVerboseLog)

//-----------------------------------------------------------------------------
/// Video Stream Info
/// Encapsulates the contents of a [VIDEO_STREAM_INFORMATION](https://mavlink.io/en/messages/common.html#VIDEO_STREAM_INFORMATION) message
class QGCVideoStreamInfo : public QObject
{
    Q_OBJECT
public:
    QGCVideoStreamInfo(QObject* parent, const mavlink_video_stream_information_t* si);

    Q_PROPERTY(QString      uri                 READ uri                NOTIFY infoChanged)
    Q_PROPERTY(QString      name                READ name               NOTIFY infoChanged)
    Q_PROPERTY(int          streamID            READ streamID           NOTIFY infoChanged)
    Q_PROPERTY(int          type                READ type               NOTIFY infoChanged)
    Q_PROPERTY(qreal        aspectRatio         READ aspectRatio        NOTIFY infoChanged)
    Q_PROPERTY(qreal        hfov                READ hfov               NOTIFY infoChanged)
    Q_PROPERTY(bool         isThermal           READ isThermal          NOTIFY infoChanged)

    QString uri             () { return QString(_streamInfo.uri);  }
    QString name            () { return QString(_streamInfo.name); }
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
class QGCCameraControl : public FactGroup
{
    Q_OBJECT
    friend class QGCCameraParamIO;
public:
    QGCCameraControl(const mavlink_camera_information_t* info, Vehicle* vehicle, int compID, QObject* parent = nullptr);
    virtual ~QGCCameraControl();

    //-- cam_mode
    enum CameraMode {
        CAM_MODE_UNDEFINED = -1,
        CAM_MODE_PHOTO  = 0,
        CAM_MODE_VIDEO  = 1,
        CAM_MODE_SURVEY = 2,
    };

    //-- Video Capture Status
    enum VideoStatus {
        VIDEO_CAPTURE_STATUS_STOPPED = 0,
        VIDEO_CAPTURE_STATUS_RUNNING,
        VIDEO_CAPTURE_STATUS_LAST,
        VIDEO_CAPTURE_STATUS_UNDEFINED = 255
    };

    //-- Photo Capture Status
    enum PhotoStatus {
        PHOTO_CAPTURE_IDLE = 0,
        PHOTO_CAPTURE_IN_PROGRESS,
        PHOTO_CAPTURE_INTERVAL_IDLE,
        PHOTO_CAPTURE_INTERVAL_IN_PROGRESS,
        PHOTO_CAPTURE_LAST,
        PHOTO_CAPTURE_STATUS_UNDEFINED = 255
    };

    //-- Photo Capture Modes
    enum PhotoMode {
        PHOTO_CAPTURE_SINGLE = 0,
        PHOTO_CAPTURE_TIMELAPSE,
    };

    //-- Storage Status
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

    Q_ENUM(CameraMode)
    Q_ENUM(VideoStatus)
    Q_ENUM(PhotoStatus)
    Q_ENUM(PhotoMode)
    Q_ENUM(StorageStatus)
    Q_ENUM(ThermalViewMode)

    Q_PROPERTY(int          version             READ version            NOTIFY infoChanged)
    Q_PROPERTY(QString      modelName           READ modelName          NOTIFY infoChanged)
    Q_PROPERTY(QString      vendor              READ vendor             NOTIFY infoChanged)
    Q_PROPERTY(QString      firmwareVersion     READ firmwareVersion    NOTIFY infoChanged)
    Q_PROPERTY(qreal        focalLength         READ focalLength        NOTIFY infoChanged)
    Q_PROPERTY(QSizeF       sensorSize          READ sensorSize         NOTIFY infoChanged)
    Q_PROPERTY(QSize        resolution          READ resolution         NOTIFY infoChanged)
    Q_PROPERTY(bool         capturesVideo       READ capturesVideo      NOTIFY infoChanged)
    Q_PROPERTY(bool         capturesPhotos      READ capturesPhotos     NOTIFY infoChanged)
    Q_PROPERTY(bool         hasModes            READ hasModes           NOTIFY infoChanged)
    Q_PROPERTY(bool         hasZoom             READ hasZoom            NOTIFY infoChanged)
    Q_PROPERTY(bool         hasFocus            READ hasFocus           NOTIFY infoChanged)
    Q_PROPERTY(bool         hasVideoStream      READ hasVideoStream     NOTIFY infoChanged)
    Q_PROPERTY(bool         photosInVideoMode   READ photosInVideoMode  NOTIFY infoChanged)
    Q_PROPERTY(bool         videoInPhotoMode    READ videoInPhotoMode   NOTIFY infoChanged)
    Q_PROPERTY(bool         isBasic             READ isBasic            NOTIFY infoChanged)
    Q_PROPERTY(quint32      storageFree         READ storageFree        NOTIFY storageFreeChanged)
    Q_PROPERTY(QString      storageFreeStr      READ storageFreeStr     NOTIFY storageFreeChanged)
    Q_PROPERTY(quint32      storageTotal        READ storageTotal       NOTIFY storageTotalChanged)
    Q_PROPERTY(int          batteryRemaining    READ batteryRemaining       NOTIFY batteryRemainingChanged)
    Q_PROPERTY(QString      batteryRemainingStr READ batteryRemainingStr    NOTIFY batteryRemainingChanged)
    Q_PROPERTY(bool         paramComplete       READ paramComplete      NOTIFY parametersReady)

    Q_PROPERTY(qreal        zoomLevel           READ zoomLevel          WRITE  setZoomLevel         NOTIFY zoomLevelChanged)
    Q_PROPERTY(qreal        focusLevel          READ focusLevel         WRITE  setFocusLevel        NOTIFY focusLevelChanged)

    Q_PROPERTY(Fact*        exposureMode        READ exposureMode       NOTIFY parametersReady)
    Q_PROPERTY(Fact*        ev                  READ ev                 NOTIFY parametersReady)
    Q_PROPERTY(Fact*        iso                 READ iso                NOTIFY parametersReady)
    Q_PROPERTY(Fact*        shutterSpeed        READ shutterSpeed       NOTIFY parametersReady)
    Q_PROPERTY(Fact*        aperture            READ aperture           NOTIFY parametersReady)
    Q_PROPERTY(Fact*        wb                  READ wb                 NOTIFY parametersReady)
    Q_PROPERTY(Fact*        mode                READ mode               NOTIFY parametersReady)

    Q_PROPERTY(QStringList  activeSettings      READ activeSettings                                 NOTIFY activeSettingsChanged)
    Q_PROPERTY(VideoStatus  videoStatus         READ videoStatus                                    NOTIFY videoStatusChanged)
    Q_PROPERTY(PhotoStatus  photoStatus         READ photoStatus                                    NOTIFY photoStatusChanged)
    Q_PROPERTY(CameraMode   cameraMode          READ cameraMode         WRITE   setCameraMode       NOTIFY cameraModeChanged)
    Q_PROPERTY(StorageStatus storageStatus      READ storageStatus                                  NOTIFY storageStatusChanged)
    Q_PROPERTY(qreal        photoLapse          READ photoLapse         WRITE   setPhotoLapse       NOTIFY photoLapseChanged)
    Q_PROPERTY(int          photoLapseCount     READ photoLapseCount    WRITE   setPhotoLapseCount  NOTIFY photoLapseCountChanged)
    Q_PROPERTY(PhotoMode    photoMode           READ photoMode          WRITE   setPhotoMode        NOTIFY photoModeChanged)
    Q_PROPERTY(int          currentStream       READ currentStream      WRITE   setCurrentStream    NOTIFY currentStreamChanged)
    Q_PROPERTY(bool         autoStream          READ autoStream                                     NOTIFY autoStreamChanged)
    Q_PROPERTY(QmlObjectListModel* streams      READ streams                                        NOTIFY streamsChanged)
    Q_PROPERTY(QGCVideoStreamInfo* currentStreamInstance READ currentStreamInstance                 NOTIFY currentStreamChanged)
    Q_PROPERTY(QGCVideoStreamInfo* thermalStreamInstance READ thermalStreamInstance                 NOTIFY thermalStreamChanged)
    Q_PROPERTY(quint32      recordTime          READ recordTime                                     NOTIFY recordTimeChanged)
    Q_PROPERTY(QString      recordTimeStr       READ recordTimeStr                                  NOTIFY recordTimeChanged)
    Q_PROPERTY(QStringList  streamLabels        READ streamLabels                                   NOTIFY streamLabelsChanged)
    Q_PROPERTY(ThermalViewMode thermalMode      READ thermalMode        WRITE  setThermalMode       NOTIFY thermalModeChanged)
    Q_PROPERTY(double       thermalOpacity      READ thermalOpacity     WRITE  setThermalOpacity    NOTIFY thermalOpacityChanged)

    Q_INVOKABLE virtual void setVideoMode   ();
    Q_INVOKABLE virtual void setPhotoMode   ();
    Q_INVOKABLE virtual void toggleMode     ();
    Q_INVOKABLE virtual bool takePhoto      ();
    Q_INVOKABLE virtual bool stopTakePhoto  ();
    Q_INVOKABLE virtual bool startVideo     ();
    Q_INVOKABLE virtual bool stopVideo      ();
    Q_INVOKABLE virtual bool toggleVideo    ();
    Q_INVOKABLE virtual void resetSettings  ();
    Q_INVOKABLE virtual void formatCard     (int id = 1);
    Q_INVOKABLE virtual void stepZoom       (int direction);
    Q_INVOKABLE virtual void startZoom      (int direction);
    Q_INVOKABLE virtual void stopZoom       ();
    Q_INVOKABLE virtual void stopStream     ();
    Q_INVOKABLE virtual void resumeStream   ();

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
    virtual bool        hasVideoStream      () { return _info.flags & CAMERA_CAP_FLAGS_HAS_VIDEO_STREAM; }
    virtual bool        photosInVideoMode   () { return _info.flags & CAMERA_CAP_FLAGS_CAN_CAPTURE_IMAGE_IN_VIDEO_MODE; }
    virtual bool        videoInPhotoMode    () { return _info.flags & CAMERA_CAP_FLAGS_CAN_CAPTURE_VIDEO_IN_IMAGE_MODE; }

    virtual int         compID              () { return _compID; }
    virtual bool        isBasic             () { return _settings.size() == 0; }
    virtual VideoStatus videoStatus         ();
    virtual PhotoStatus photoStatus         ();
    virtual PhotoMode   photoMode           () { return _photoMode; }
    virtual qreal       photoLapse          () { return _photoLapse; }
    virtual int         photoLapseCount     () { return _photoLapseCount; }
    virtual CameraMode  cameraMode          () { return _cameraMode; }
    virtual StorageStatus storageStatus     () { return _storageStatus; }
    virtual QStringList activeSettings      ();
    virtual quint32     storageFree         () { return _storageFree;  }
    virtual QString     storageFreeStr      ();
    virtual quint32     storageTotal        () { return _storageTotal; }
    virtual int         batteryRemaining    () { return _batteryRemaining; }
    virtual QString     batteryRemainingStr ();
    virtual bool        paramComplete       () { return _paramComplete; }
    virtual qreal       zoomLevel           () { return _zoomLevel; }
    virtual qreal       focusLevel          () { return _focusLevel; }

    virtual QmlObjectListModel* streams     () { return &_streams; }
    virtual QGCVideoStreamInfo* currentStreamInstance();
    virtual QGCVideoStreamInfo* thermalStreamInstance();
    virtual int          currentStream      () { return _currentStream; }
    virtual void         setCurrentStream   (int stream);
    virtual bool         autoStream         ();
    virtual quint32      recordTime         () { return _recordTime; }
    virtual QString      recordTimeStr      ();

    virtual Fact*       exposureMode        ();
    virtual Fact*       ev                  ();
    virtual Fact*       iso                 ();
    virtual Fact*       shutterSpeed        ();
    virtual Fact*       aperture            ();
    virtual Fact*       wb                  ();
    virtual Fact*       mode                ();

    /// Stream names to show the user (for selection)
    virtual QStringList streamLabels        () { return _streamLabels; }

    virtual ThermalViewMode thermalMode     () { return _thermalMode; }
    virtual void        setThermalMode      (ThermalViewMode mode);
    virtual double      thermalOpacity      () { return _thermalOpacity; }
    virtual void        setThermalOpacity   (double val);

    virtual void        setZoomLevel        (qreal level);
    virtual void        setFocusLevel       (qreal level);
    virtual void        setCameraMode       (CameraMode mode);
    virtual void        setPhotoMode        (PhotoMode mode);
    virtual void        setPhotoLapse       (qreal interval);
    virtual void        setPhotoLapseCount  (int count);

    virtual void        handleSettings      (const mavlink_camera_settings_t& settings);
    virtual void        handleCaptureStatus (const mavlink_camera_capture_status_t& capStatus);
    virtual void        handleParamAck      (const mavlink_param_ext_ack_t& ack);
    virtual void        handleParamValue    (const mavlink_param_ext_value_t& value);
    virtual void        handleStorageInfo   (const mavlink_storage_information_t& st);
    virtual void        handleBatteryStatus (const mavlink_battery_status_t& bs);
    virtual void        handleVideoInfo     (const mavlink_video_stream_information_t *vi);
    virtual void        handleVideoStatus   (const mavlink_video_stream_status_t *vs);

    /// Notify controller a parameter has changed
    virtual void        factChanged         (Fact* pFact);
    /// Allow controller to modify or invalidate incoming parameter
    virtual bool        incomingParameter   (Fact* pFact, QVariant& newValue);
    /// Allow controller to modify or invalidate parameter change
    virtual bool        validateParameter   (Fact* pFact, QVariant& newValue);

    // Known Parameters
    static const char* kCAM_EV;
    static const char* kCAM_EXPMODE;
    static const char* kCAM_ISO;
    static const char* kCAM_SHUTTERSPD;
    static const char* kCAM_APERTURE;
    static const char* kCAM_WBMODE;
    static const char* kCAM_MODE;

signals:
    void    infoChanged                     ();
    void    videoStatusChanged              ();
    void    photoStatusChanged              ();
    void    photoModeChanged                ();
    void    photoLapseChanged               ();
    void    photoLapseCountChanged          ();
    void    cameraModeChanged               ();
    void    activeSettingsChanged           ();
    void    storageFreeChanged              ();
    void    storageTotalChanged             ();
    void    batteryRemainingChanged         ();
    void    dataReady                       (QByteArray data);
    void    parametersReady                 ();
    void    zoomLevelChanged                ();
    void    focusLevelChanged               ();
    void    streamsChanged                  ();
    void    currentStreamChanged            ();
    void    thermalStreamChanged            ();
    void    autoStreamChanged               ();
    void    recordTimeChanged               ();
    void    streamLabelsChanged             ();
    void    thermalModeChanged              ();
    void    thermalOpacityChanged           ();
    void    storageStatusChanged            ();

protected:
    virtual void    _setVideoStatus         (VideoStatus status);
    virtual void    _setPhotoStatus         (PhotoStatus status);
    virtual void    _setCameraMode          (CameraMode mode);
    virtual void    _requestStreamInfo      (uint8_t streamID);
    virtual void    _requestStreamStatus    (uint8_t streamID);
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
    virtual void    _streamTimeout          ();
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
    PhotoMode                           _photoMode          = PHOTO_CAPTURE_SINGLE;
    qreal                               _photoLapse         = 1.0;
    int                                 _photoLapseCount    = 0;
    VideoStatus                         _video_status       = VIDEO_CAPTURE_STATUS_UNDEFINED;
    PhotoStatus                         _photo_status       = PHOTO_CAPTURE_STATUS_UNDEFINED;
    QStringList                         _activeSettings;
    QStringList                         _settings;
    QTimer                              _captureStatusTimer;
    QList<QGCCameraOptionExclusion*>    _valueExclusions;
    QList<QGCCameraOptionRange*>        _optionRanges;
    QMap<QString, QStringList>          _originalOptNames;
    QMap<QString, QVariantList>         _originalOptValues;
    QMap<QString, QGCCameraParamIO*>    _paramIO;
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
    int                                 _requestCount       = 0;
    int                                 _currentStream      = 0;
    int                                 _expectedCount      = 1;
    QTimer                              _streamInfoTimer;
    QTimer                              _streamStatusTimer;
    QmlObjectListModel                  _streams;
    QStringList                         _streamLabels;
    ThermalViewMode                     _thermalMode        = THERMAL_BLEND;
    double                              _thermalOpacity     = 85.0;
};
