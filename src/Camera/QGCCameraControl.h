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
public:
    QGCCameraControl(const mavlink_camera_information_t* info, Vehicle* vehicle, int compID, QObject* parent = NULL);
    ~QGCCameraControl();

    //-- cam_mode
    enum CameraMode {
        CAMERA_MODE_UNDEFINED = -1,
        CAMERA_MODE_PHOTO = 0,
        CAMERA_MODE_VIDEO = 1,
    };

    Q_ENUMS(CameraMode)

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

    Q_PROPERTY(QStringList  activeSettings      READ activeSettings                             NOTIFY activeSettingsChanged)
    Q_PROPERTY(CameraMode   cameraMode          READ cameraMode         WRITE   setCameraMode   NOTIFY cameraModeChanged)

    Q_INVOKABLE void setVideoMode   ();
    Q_INVOKABLE void setPhotoMode   ();
    Q_INVOKABLE void toggleMode     ();
    Q_INVOKABLE void takePhoto      ();
    Q_INVOKABLE void startVideo     ();
    Q_INVOKABLE void stopVideo      ();
    Q_INVOKABLE void resetSettings  ();
    Q_INVOKABLE void formatCard     (int id = 1);

    int         version             () { return _version; }
    QString     modelName           () { return _modelName; }
    QString     vendor              () { return _vendor; }
    QString     firmwareVersion     ();
    qreal       focalLength         () { return (qreal)_info.focal_length; }
    QSizeF      sensorSize          () { return QSizeF(_info.sensor_size_h, _info.sensor_size_v); }
    QSize       resolution          () { return QSize(_info.resolution_h, _info.resolution_v); }
    bool        capturesVideo       () { return true  /*_info.flags & CAMERA_CAP_FLAGS_CAPTURE_VIDEO*/ ; }
    bool        capturesPhotos      () { return true  /*_info.flags & CAMERA_CAP_FLAGS_CAPTURE_PHOTO*/ ; }
    bool        hasModes            () { return true  /*_info.flags & CAMERA_CAP_FLAGS_HAS_MODES*/ ; }
    bool        photosInVideoMode   () { return true  /*_info.flags & CAMERA_CAP_FLAGS_CAN_CAPTURE_PHOTO_IN_VIDEO_MODE*/ ; }
    bool        videoInPhotoMode    () { return false /*_info.flags & CAMERA_CAP_FLAGS_CAN_CAPTURE_VIDEO_IN_PHOTO_MODE*/ ; }

    int         compID              () { return _compID; }
    bool        isBasic             () { return _settings.size() == 0; }
    CameraMode  cameraMode          () { return _cameraMode; }
    QStringList activeSettings      () { return _activeSettings; }

    void        setCameraMode       (CameraMode mode);

    void        handleSettings      (const mavlink_camera_settings_t& settings);
    void        handleParamAck      (const mavlink_param_ext_ack_t& ack);
    void        handleParamValue    (const mavlink_param_ext_value_t& value);
    void        factChanged         (Fact* pFact);

signals:
    void    infoChanged                     ();
    void    cameraModeChanged               ();
    void    activeSettingsChanged           ();

private slots:
    void    _requestCameraSettings          ();
    void    _requestAllParameters           ();

private:
    bool    _handleLocalization             (QByteArray& bytes);
    bool    _replaceLocaleStrings           (const QDomNode node, QByteArray& bytes);
    bool    _loadCameraDefinitionFile       (const QString& file);
    bool    _loadConstants                  (const QDomNodeList nodeList);
    bool    _loadSettings                   (const QDomNodeList nodeList);
    void    _processRanges                  ();
    bool    _processCondition               (const QString condition);
    bool    _processConditionTest           (const QString conditionTest);
    bool    _loadNameValue                  (QDomNode option, const QString factName, FactMetaData* metaData, QString& optName, QString& optValue, QVariant& optVariant);
    bool    _loadRanges                     (QDomNode option, const QString factName, QString paramValue);
    void    _updateActiveList               ();
    void    _updateRanges                   (Fact* pFact);

    QStringList     _loadExclusions         (QDomNode option);
    QString         _getParamName           (const char* param_id);

private:
    Vehicle*                            _vehicle;
    int                                 _compID;
    mavlink_camera_information_t        _info;
    int                                 _version;
    QString                             _modelName;
    QString                             _vendor;
    CameraMode                          _cameraMode;
    QStringList                         _activeSettings;
    QStringList                         _settings;
    QList<QGCCameraOptionExclusion*>    _valueExclusions;
    QList<QGCCameraOptionRange*>        _optionRanges;
    QMap<QString, QStringList>          _originalOptNames;
    QMap<QString, QVariantList>         _originalOptValues;
    QMap<QString, QGCCameraParamIO*>    _paramIO;
};
