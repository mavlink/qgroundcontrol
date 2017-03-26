/*!
 * @file
 *   @brief Camera Controller
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#pragma once

#include <QObject>
#include <QTimer>
#include <QSoundEffect>

#include "Vehicle.h"

Q_DECLARE_LOGGING_CATEGORY(YuneecCameraLog)

class QNetworkRequest;
class QNetworkAccessManager;

//-----------------------------------------------------------------------------
// Ambarella Camera Status
typedef struct {
    int         rval;
    int         msg_id;
    int         cam_mode;
    QString     status;
    uint32_t    sdfree;
    uint32_t    sdtotal;
    uint32_t    record_time;
    uint32_t    white_balance;
    int         ae_enable;
    uint32_t    iq_type;
    QString     exposure_value;
    QString     video_mode;
    uint32_t    awb_lock;
    bool        audio_switch;
    QString     shutter_time;
    QString     iso_value;
    QString     photo_format;
    QString     rtsp_res;
    int         photo_mode;
    int         photo_num;
    int         photo_times;
    QString     ev_step;
    int         interval_ms;
    int         cam_scene;
    bool        audio_enable;
    int         left_time;
    uint32_t    metering_mode;
    float       x_ratio;
    float       y_ratio;
    int         layers;
    int         pitch;
    int         yaw;
    int         timer_photo_sta;
} amb_camera_status_t;

//-----------------------------------------------------------------------------
// Video Resolution Options
typedef struct {
    const char* description;
    const char* video_mode;
    float       aspectRatio;
} video_res_t;

//-----------------------------------------------------------------------------
// Color Mode
typedef struct {
    const char* description;
    uint32_t    mode;
} iq_mode_t;

//-----------------------------------------------------------------------------
// Metering Mode
typedef struct {
    const char* description;
    uint32_t    mode;
} metering_mode_t;

//-----------------------------------------------------------------------------
// Photo Format
typedef struct {
    const char* description;
    const char* mode;
} photo_format_t;

//-----------------------------------------------------------------------------
// White Balance
typedef struct {
    const char* description;
    uint32_t    mode;
} white_balance_t;

//-----------------------------------------------------------------------------
// ISO Values
typedef struct {
    const char* description;
} iso_values_t;

//-----------------------------------------------------------------------------
// White Balance Values
typedef struct {
    const char* description;
} wb_values_t;

//-----------------------------------------------------------------------------
// Shutter Speeds
typedef struct {
    const char* description;
    const char* value;
} shutter_speeds_t;

//-----------------------------------------------------------------------------
// Exposure Compensation
typedef struct {
    const char* description;
    const char* value;
} exposure_compsensation_t;

//-----------------------------------------------------------------------------
class CameraControl : public QObject
{
    Q_OBJECT
public:
    CameraControl(QObject* parent = NULL);
    ~CameraControl();

    //-- Camera Control
    enum VideoStatus {
        VIDEO_CAPTURE_STATUS_STOPPED = 0,
        VIDEO_CAPTURE_STATUS_RUNNING,
        VIDEO_CAPTURE_STATUS_CAPTURE,
        VIDEO_CAPTURE_STATUS_UNDEFINED
    };

    //-- cam_mode
    enum CameraMode {
        CAMERA_MODE_UNDEFINED = 0,
        CAMERA_MODE_VIDEO,
        CAMERA_MODE_PHOTO,
    };

    //-- Auto Exposure
    enum AEModes {
        AE_MODE_MANUAL = 0,
        AE_MODE_AUTO,
        AE_MODE_UNDEFINED
    };

    #define DEFAULT_VALUE -1.0f

    Q_ENUMS(VideoStatus)
    Q_ENUMS(CameraMode)
    Q_ENUMS(AEModes)

    Q_PROPERTY(VideoStatus  videoStatus     READ    videoStatus                                 NOTIFY videoStatusChanged)
    Q_PROPERTY(CameraMode   cameraMode      READ    cameraMode      WRITE   setCameraMode       NOTIFY cameraModeChanged)
    Q_PROPERTY(AEModes      aeMode          READ    aeMode          WRITE   setAeMode           NOTIFY aeModeChanged)
    Q_PROPERTY(quint32      recordTime      READ    recordTime                                  NOTIFY recordTimeChanged)
    Q_PROPERTY(quint32      sdFree          READ    sdFree                                      NOTIFY sdFreeChanged)
    Q_PROPERTY(QString      sdFreeStr       READ    sdFreeStr                                   NOTIFY sdFreeChanged)
    Q_PROPERTY(quint32      sdTotal         READ    sdTotal                                     NOTIFY sdTotalChanged)
    Q_PROPERTY(QString      recordTimeStr   READ    recordTimeStr                               NOTIFY recordTimeChanged)
    Q_PROPERTY(QStringList  videoResList    READ    videoResList                                CONSTANT)
    Q_PROPERTY(QStringList  iqModeList      READ    iqModeList                                  CONSTANT)
    Q_PROPERTY(QStringList  wbList          READ    wbList                                      CONSTANT)
    Q_PROPERTY(QStringList  isoList         READ    isoList                                     CONSTANT)
    Q_PROPERTY(QStringList  shutterList     READ    shutterList                                 CONSTANT)
    Q_PROPERTY(QStringList  meteringList    READ    meteringList                                CONSTANT)
    Q_PROPERTY(QStringList  photoFormatList READ    photoFormatList                             CONSTANT)
    Q_PROPERTY(QStringList  evList          READ    evList                                      CONSTANT)

    Q_PROPERTY(quint32      currentVideoRes READ    currentVideoRes WRITE setCurrentVideoRes    NOTIFY currentVideoResChanged)
    Q_PROPERTY(quint32      currentWB       READ    currentWB       WRITE setCurrentWB          NOTIFY currentWBChanged)
    Q_PROPERTY(quint32      currentIso      READ    currentIso      WRITE setCurrentIso         NOTIFY currentIsoChanged)
    Q_PROPERTY(quint32      currentShutter  READ    currentShutter  WRITE setCurrentShutter     NOTIFY currentShutterChanged)
    Q_PROPERTY(quint32      currentIQ       READ    currentIQ       WRITE setCurrentIQ          NOTIFY currentIQChanged)
    Q_PROPERTY(quint32      currentPhotoFmt READ    currentPhotoFmt WRITE setCurrentPhotoFmt    NOTIFY currentPhotoFmtChanged)
    Q_PROPERTY(quint32      currentMetering READ    currentMetering WRITE setCurrentMetering    NOTIFY currentMeteringChanged)
    Q_PROPERTY(quint32      currentEV       READ    currentEV       WRITE setCurrentEV          NOTIFY currentEVChanged)

    Q_INVOKABLE void setVideoMode   ();
    Q_INVOKABLE void setPhotoMode   ();
    Q_INVOKABLE void startVideo     ();
    Q_INVOKABLE void stopVideo      ();
    Q_INVOKABLE void takePhoto      ();
    Q_INVOKABLE void toggleMode     ();
    Q_INVOKABLE void toggleVideo    ();
    Q_INVOKABLE void resetSettings  ();
    Q_INVOKABLE void formatCard     ();

    VideoStatus videoStatus         ();
    CameraMode  cameraMode          ();
    AEModes     aeMode              ();
    quint32     recordTime          ();
    quint32     sdFree              () { return _ambarellaStatus.sdfree;  }
    QString     sdFreeStr           ();
    quint32     sdTotal             () { return _ambarellaStatus.sdtotal; }
    QString     recordTimeStr       ();
    QStringList videoResList        ();
    QStringList iqModeList          ();
    QStringList wbList              ();
    QStringList isoList             ();
    QStringList shutterList         ();
    QStringList meteringList        ();
    QStringList photoFormatList     ();
    QStringList evList              ();

    quint32     currentVideoRes     () { return _currentVideoResIndex; }
    quint32     currentWB           () { return _currentWB; }
    quint32     currentIso          () { return _currentIso; }
    quint32     currentShutter      () { return _currentShutter; }
    quint32     currentIQ           () { return _ambarellaStatus.iq_type; }
    quint32     currentPhotoFmt     () { return _currentPhotoFmt; }
    quint32     currentMetering     () { return _ambarellaStatus.metering_mode; }
    quint32     currentEV           () { return _currentEV; }

    void        setCameraMode       (CameraMode mode);
    void        setAeMode           (AEModes mode);
    void        setVehicle          (Vehicle* vehicle);

    void        setCurrentVideoRes  (quint32 index);
    void        setCurrentWB        (quint32 index);
    void        setCurrentIso       (quint32 index);
    void        setCurrentShutter   (quint32 index);
    void        setCurrentIQ        (quint32 index);
    void        setCurrentPhotoFmt  (quint32 index);
    void        setCurrentMetering  (quint32 index);
    void        setCurrentEV        (quint32 index);

    QNetworkAccessManager*  networkManager  ();

private slots:
    void    _httpFinished           ();
    void    _getCameraStatus        ();

signals:
    void    videoStatusChanged      ();
    void    cameraModeChanged       ();
    void    aeModeChanged           ();
    void    recordTimeChanged       ();
    void    sdFreeChanged           ();
    void    sdTotalChanged          ();
    void    currentVideoResChanged  ();
    void    currentWBChanged        ();
    void    currentIsoChanged       ();
    void    currentShutterChanged   ();
    void    currentIQChanged        ();
    void    currentPhotoFmtChanged  ();
    void    currentMeteringChanged  ();
    void    currentEVChanged        ();

private:
    void    _updateAspectRatio      ();
    void    _initStreaming          ();
    void    _handleVideoResStatus   ();
    void    _handleShutterStatus    ();
    void    _handleISOStatus        ();
    void    _sendAmbRequest         (QNetworkRequest request);
    void    _setCamMode             (const char* mode);
    void    _handleCameraStatus     (int http_code, QByteArray data);
    void    _handleTakePhotoStatus  (int http_code, QByteArray data);
    void    _resetCameraValues      ();

private:
    Vehicle*                _vehicle;
    QTimer                  _updateTimer;
    QStringList             _videoResList;
    QStringList             _iqModeList;
    QStringList             _wbList;
    QStringList             _isoList;
    QStringList             _shutterList;
    QStringList             _meteringList;
    QStringList             _photoFormatList;
    QStringList             _evList;
    QSoundEffect            _cameraSound;
    QSoundEffect            _videoSound;
    QSoundEffect            _errorSound;

    enum {
        CAMERA_SUPPORT_UNDEFINED,
        CAMERA_SUPPORT_YES,
        CAMERA_SUPPORT_NO
    };

    int                     _cameraSupported;
    int                     _httpErrorCount;
    quint32                 _currentVideoResIndex;
    quint32                 _currentWB;
    quint32                 _currentIso;
    quint32                 _currentShutter;
    quint32                 _currentPhotoFmt;
    quint32                 _currentEV;

    QNetworkAccessManager*  _networkManager;
    amb_camera_status_t     _ambarellaStatus;
    QTimer                  _statusTimer;
};
