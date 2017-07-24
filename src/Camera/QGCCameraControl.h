/*!
 * @file
 *   @brief Camera Controller
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#pragma once

#include "QGCApplication.h"
#include <QLoggingCategory>

class QDomNode;
class QDomNodeList;
class QGCCameraParamIO;

Q_DECLARE_LOGGING_CATEGORY(CameraControlLog)
Q_DECLARE_LOGGING_CATEGORY(CameraControlLogVerbose)

//-----------------------------------------------------------------------------
class QGCCameraOptionExclusion : public QObject
{
public:
    QGCCameraOptionExclusion(QObject* parent, QString param_, QString value_, QStringList exclusions_)
        : QObject(parent)
        , param(param_)
        , value(value_)
        , exclusions(exclusions_)
    {
    }
    QString param;
    QString value;
    QStringList exclusions;
};

//-----------------------------------------------------------------------------
class QGCCameraOptionRange : public QObject
{
public:
    QGCCameraOptionRange(QObject* parent, QString param_, QString value_, QString targetParam_, QString condition_, QStringList optNames_, QStringList optValues_)
        : QObject(parent)
        , param(param_)
        , value(value_)
        , targetParam(targetParam_)
        , condition(condition_)
        , optNames(optNames_)
        , optValues(optValues_)
    {
    }
    QString param;
    QString value;
    QString targetParam;
    QString condition;
    QStringList  optNames;
    QStringList  optValues;
    QVariantList optVariants;
};

//-----------------------------------------------------------------------------
class QGCCameraControl : public FactGroup
{
    Q_OBJECT
    friend class QGCCameraParamIO;
public:
    QGCCameraControl(const mavlink_camera_information_t* info, Vehicle* vehicle, int compID, QObject* parent = NULL);
    virtual ~QGCCameraControl();

    //-- cam_mode
    enum CameraMode {
        CAMERA_MODE_UNDEFINED = -1,
        CAMERA_MODE_PHOTO = 0,
        CAMERA_MODE_VIDEO = 1,
    };

    //-- Video Capture Status
    enum VideoStatus {
        VIDEO_CAPTURE_STATUS_STOPPED = 0,
        VIDEO_CAPTURE_STATUS_RUNNING,
        VIDEO_CAPTURE_STATUS_UNDEFINED
    };

    Q_ENUMS(CameraMode)
    Q_ENUMS(VideoStatus)

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
    Q_PROPERTY(bool         photosInVideoMode   READ photosInVideoMode  NOTIFY infoChanged)
    Q_PROPERTY(bool         videoInPhotoMode    READ videoInPhotoMode   NOTIFY infoChanged)
    Q_PROPERTY(bool         isBasic             READ isBasic            NOTIFY infoChanged)
    Q_PROPERTY(quint32      storageFree         READ storageFree        NOTIFY storageFreeChanged)
    Q_PROPERTY(QString      storageFreeStr      READ storageFreeStr     NOTIFY storageFreeChanged)
    Q_PROPERTY(quint32      storageTotal        READ storageTotal       NOTIFY storageTotalChanged)

    Q_PROPERTY(QStringList  activeSettings      READ activeSettings                             NOTIFY activeSettingsChanged)
    Q_PROPERTY(VideoStatus  videoStatus         READ videoStatus                                NOTIFY videoStatusChanged)
    Q_PROPERTY(CameraMode   cameraMode          READ cameraMode         WRITE   setCameraMode   NOTIFY cameraModeChanged)

    Q_INVOKABLE virtual void setVideoMode   ();
    Q_INVOKABLE virtual void setPhotoMode   ();
    Q_INVOKABLE virtual void toggleMode     ();
    Q_INVOKABLE virtual bool takePhoto      ();
    Q_INVOKABLE virtual bool startVideo     ();
    Q_INVOKABLE virtual bool stopVideo      ();
    Q_INVOKABLE virtual bool toggleVideo    ();
    Q_INVOKABLE virtual void resetSettings  ();
    Q_INVOKABLE virtual void formatCard     (int id = 1);

    virtual int         version             () { return _version; }
    virtual QString     modelName           () { return _modelName; }
    virtual QString     vendor              () { return _vendor; }
    virtual QString     firmwareVersion     ();
    virtual qreal       focalLength         () { return (qreal)_info.focal_length; }
    virtual QSizeF      sensorSize          () { return QSizeF(_info.sensor_size_h, _info.sensor_size_v); }
    virtual QSize       resolution          () { return QSize(_info.resolution_h, _info.resolution_v); }
    virtual bool        capturesVideo       () { return _info.flags & CAMERA_CAP_FLAGS_CAPTURE_VIDEO; }
    virtual bool        capturesPhotos      () { return _info.flags & CAMERA_CAP_FLAGS_CAPTURE_IMAGE; }
    virtual bool        hasModes            () { return _info.flags & CAMERA_CAP_FLAGS_HAS_MODES; }
    virtual bool        photosInVideoMode   () { return _info.flags & CAMERA_CAP_FLAGS_CAN_CAPTURE_IMAGE_IN_VIDEO_MODE; }
    virtual bool        videoInPhotoMode    () { return _info.flags & CAMERA_CAP_FLAGS_CAN_CAPTURE_VIDEO_IN_IMAGE_MODE; }

    virtual int         compID              () { return _compID; }
    virtual bool        isBasic             () { return _settings.size() == 0; }
    virtual VideoStatus videoStatus         ();
    virtual CameraMode  cameraMode          () { return _cameraMode; }
    virtual QStringList activeSettings      () { return _activeSettings; }
    virtual quint32     storageFree         () { return _storageFree;  }
    virtual QString     storageFreeStr      ();
    virtual quint32     storageTotal        () { return _storageTotal; }

    virtual void        setCameraMode       (CameraMode mode);

    virtual void        handleSettings      (const mavlink_camera_settings_t& settings);
    virtual void        handleCaptureStatus (const mavlink_camera_capture_status_t& capStatus);
    virtual void        handleParamAck      (const mavlink_param_ext_ack_t& ack);
    virtual void        handleParamValue    (const mavlink_param_ext_value_t& value);
    virtual void        handleStorageInfo   (const mavlink_storage_information_t& st);
    virtual void        factChanged         (Fact* pFact);

signals:
    void    infoChanged                     ();
    void    videoStatusChanged              ();
    void    cameraModeChanged               ();
    void    activeSettingsChanged           ();
    void    storageFreeChanged              ();
    void    storageTotalChanged             ();
    void    dataReady                       (QByteArray data);
    void    parametersReady                 ();

protected:
    virtual void    _setVideoStatus         (VideoStatus status);
    virtual void    _setCameraMode          (CameraMode mode);

private slots:
    void    _initWhenReady                  ();
    void    _requestCameraSettings          ();
    void    _requestAllParameters           ();
    void    _requestCaptureStatus           ();
    void    _requestStorageInfo             ();
    void    _downloadFinished               ();
    void    _mavCommandResult               (int vehicleId, int component, int command, int result, bool noReponseFromVehicle);
    void    _dataReady                      (QByteArray data);
    void    _paramDone                      ();

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
    QString         _getParamName           (const char* param_id);

protected:
    Vehicle*                            _vehicle;
    int                                 _compID;
    mavlink_camera_information_t        _info;
    int                                 _version;
    bool                                _cached;
    uint32_t                            _storageFree;
    uint32_t                            _storageTotal;
    QNetworkAccessManager*              _netManager;
    QString                             _modelName;
    QString                             _vendor;
    QString                             _cacheFile;
    CameraMode                          _cameraMode;
    VideoStatus                         _video_status;
    QStringList                         _activeSettings;
    QStringList                         _settings;
    QTimer                              _captureStatusTimer;
    QList<QGCCameraOptionExclusion*>    _valueExclusions;
    QList<QGCCameraOptionRange*>        _optionRanges;
    QMap<QString, QStringList>          _originalOptNames;
    QMap<QString, QVariantList>         _originalOptValues;
    QMap<QString, QGCCameraParamIO*>    _paramIO;
};
