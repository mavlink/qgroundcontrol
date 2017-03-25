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
    int         white_balance;
    bool        ae_enabled;
    int         iq_type;
    float       exposure_value;
    QString     video_mode;
    int         awb_lock;
    bool        audio_switch;
    int         shutter_time;
    QString     iso_value;
    QString     photo_format;
    QString     rtsp_res;
    int         photo_mode;
    int         photo_num;
    int         photo_times;
    float       ev_step;
    int         interval_ms;
    int         cam_scene;
    bool        audio_enable;
    int         left_time;
    int         metering_mode;
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
    int         mode;
} color_mode_t;

//-----------------------------------------------------------------------------
// Metering Mode
typedef struct {
    const char* description;
    int         mode;
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
    int         mode;
} white_balance_t;

//-----------------------------------------------------------------------------
// ISO Values
typedef struct {
    const char* description;
    const char* value;
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
        VIDEO_CAPTURE_STATUS_UNDEFINED
    };

    //-- cam_mode
    enum CameraMode {
        CAMERA_MODE_UNDEFINED = 0,
        CAMERA_MODE_VIDEO,
        CAMERA_MODE_PHOTO,
    };

    #define DEFAULT_VALUE -1.0f

    Q_ENUMS(VideoStatus)
    Q_ENUMS(CameraMode)

    Q_PROPERTY(VideoStatus  videoStatus     READ    videoStatus                                 NOTIFY videoStatusChanged)
    Q_PROPERTY(CameraMode   cameraMode      READ    cameraMode      WRITE   setCameraMode       NOTIFY cameraModeChanged)
    Q_PROPERTY(quint32      recordTime      READ    recordTime                                  NOTIFY recordTimeChanged)
    Q_PROPERTY(QString      recordTimeStr   READ    recordTimeStr                               NOTIFY recordTimeChanged)
    Q_PROPERTY(QStringList  videoResList    READ    videoResList                                CONSTANT)
    Q_PROPERTY(QStringList  colorModeList   READ    colorModeList                               CONSTANT)
    Q_PROPERTY(QStringList  wbList          READ    wbList                                      CONSTANT)
    Q_PROPERTY(QStringList  isoList         READ    isoList                                     CONSTANT)
    Q_PROPERTY(QStringList  shutterList     READ    shutterList                                 CONSTANT)

    Q_PROPERTY(quint32      currentVideoRes READ    currentVideoRes WRITE setCurrentVideoRes    NOTIFY currentVideoResChanged)
    Q_PROPERTY(quint32      currentWb       READ    currentWb       WRITE setCurrentWb          NOTIFY currentWbChanged)
    Q_PROPERTY(quint32      currentIso      READ    currentIso      WRITE setCurrentIso         NOTIFY currentIsoChanged)
    Q_PROPERTY(quint32      currentShutter  READ    currentShutter  WRITE setCurrentShutter     NOTIFY currentShutterChanged)

    Q_INVOKABLE void setVideoMode   ();
    Q_INVOKABLE void setPhotoMode   ();
    Q_INVOKABLE void startVideo     ();
    Q_INVOKABLE void stopVideo      ();
    Q_INVOKABLE void takePhoto      ();
    Q_INVOKABLE void toggleMode     ();
    Q_INVOKABLE void toggleVideo    ();

    VideoStatus videoStatus         ();
    CameraMode  cameraMode          ();
    quint32     recordTime          ();
    QString     recordTimeStr       ();
    QStringList videoResList        ();
    QStringList colorModeList       ();
    QStringList wbList              ();
    QStringList isoList             ();
    QStringList shutterList         ();

    quint32     currentVideoRes     () { return _currentVideoResIndex; }
    quint32     currentWb           () { return _currentWb; }
    quint32     currentIso          () { return _currentIso; }
    quint32     currentShutter      () { return _currentShutter; }

    void        setCameraMode       (CameraMode mode);
    void        setVehicle          (Vehicle* vehicle);

    void        setCurrentVideoRes  (quint32 index);
    void        setCurrentWb        (quint32 index);
    void        setCurrentIso       (quint32 index);
    void        setCurrentShutter   (quint32 index);

    QNetworkAccessManager*  networkManager  ();

private slots:
    void    _requestCameraSettings  ();
    void    _requestCaptureStatus   ();
    void    _httpFinished           ();
    void    _getCameraStatus        ();

signals:
    void    videoStatusChanged      ();
    void    cameraModeChanged       ();
    void    recordTimeChanged       ();
    void    currentVideoResChanged  ();
    void    currentWbChanged        ();
    void    currentIsoChanged       ();
    void    currentShutterChanged   ();

private:
    void    _updateAspectRatio      ();
    void    _updateVideoRes         (int w, int h, float f);
    int     _findVideoRes           (int w, int h, float f);
    void    _updateIso              (int iso, int locked);
    void    _updateShutter          (int speed, int locked);
    void    _setSettings_1          (float p1 = DEFAULT_VALUE, float p2 = DEFAULT_VALUE, float p3 = DEFAULT_VALUE, float p4 = DEFAULT_VALUE, float p5 = DEFAULT_VALUE, float p6 = DEFAULT_VALUE, float p7 = DEFAULT_VALUE);
    void    _setSettings_2          (float p1 = DEFAULT_VALUE, float p2 = DEFAULT_VALUE, float p3 = DEFAULT_VALUE, float p4 = DEFAULT_VALUE, float p5 = DEFAULT_VALUE, float p6 = DEFAULT_VALUE);
    void    _initStreaming          ();
    void    _handleVideoResStatus   ();
    void    _sendAmbRequest         (QNetworkRequest request);
    void    _setCamMode             (const char* mode);
    void    _handleCameraStatus     (int http_code, QByteArray data);
    void    _handleTakePhotoStatus  (int http_code, QByteArray data);

private:
    Vehicle*                _vehicle;
    QTimer                  _updateTimer;
    QStringList             _videoResList;
    QStringList             _colorModeList;
    QStringList             _wbList;
    QStringList             _isoList;
    QStringList             _shutterList;
    QSoundEffect            _cameraSound;
    QSoundEffect            _videoSound;
    QSoundEffect            _errorSound;
    camera_capture_status_t _cameraStatus;
    camera_settings_t       _cameraSettings;

    enum {
        CAMERA_SUPPORT_UNDEFINED,
        CAMERA_SUPPORT_YES,
        CAMERA_SUPPORT_NO
    };

    bool                    _waitingShutter;
    int                     _cameraSupported;
    int                     _httpErrorCount;
    quint32                 _currentVideoResIndex;


    quint32                 _currentWb;
    quint32                 _currentIso;
    quint32                 _currentShutter;

    //-- Direct Ambarella Interface
    QNetworkAccessManager*  _networkManager;
    amb_camera_status_t     _amb_cam_status;
};
