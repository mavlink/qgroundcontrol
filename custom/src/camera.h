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

//-----------------------------------------------------------------------------
// Video Resolution Options
typedef struct {
    const char* description;
    int         with;
    int         height;
    int         fps;
    float       aspectRatio;
} video_res_t;

//-----------------------------------------------------------------------------
// Color Mode
typedef struct {
    const char* description;
    int         mode;
} color_mode_t;

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
    float       speed;
} shutter_speeds_t;

//-----------------------------------------------------------------------------
// Camera Capture Status
typedef struct {
    bool        data_ready;             /*< Data received from camera*/
    float       image_interval;         /*< Image capture interval in seconds*/
    float       video_framerate;        /*< Video frame rate in Hz*/
    uint32_t    recording_time_ms;      /*< Time in milliseconds since recording started*/
    float       available_capacity;     /*< Available storage capacity in MiB*/
    uint16_t    image_resolution_h;     /*< Image resolution in pixels horizontal*/
    uint16_t    image_resolution_v;     /*< Image resolution in pixels vertical*/
    uint16_t    video_resolution_h;     /*< Video resolution in pixels horizontal*/
    uint16_t    video_resolution_v;     /*< Video resolution in pixels vertical*/
    uint8_t     camera_id;              /*< Camera ID if there are multiple*/
    uint8_t     image_status;           /*< Current status of image capturing (0: not running, 1: interval capture in progress)*/
    uint8_t     video_status;           /*< Current status of video capturing (0: not running, 1: capture in progress)*/
} camera_capture_status_t;

//-----------------------------------------------------------------------------
// Camera Settings
typedef struct {
    bool        data_ready;             /*< Data received from camera*/
    float       aperture;               /*< Aperture is 1/value*/
    float       shutter_speed;          /*< Shutter speed in s*/
    float       iso_sensitivity;        /*< ISO sensitivity*/
    float       white_balance;          /*< Color temperature in K*/
    uint8_t     camera_id;              /*< Camera ID if there are multiple*/
    uint8_t     aperture_locked;        /*< Aperture locked (0: auto, 1: locked)*/
    uint8_t     shutter_speed_locked;   /*< Shutter speed locked (0: auto, 1: locked)*/
    uint8_t     iso_sensitivity_locked; /*< ISO sensitivity locked (0: auto, 1: locked)*/
    uint8_t     white_balance_locked;   /*< Color temperature locked (0: auto, 1: locked)*/
    uint8_t     mode_id;                /*< Reserved for a camera mode ID*/
    uint8_t     color_mode_id;          /*< Reserved for a color mode ID*/
    uint8_t     image_format_id;        /*< Reserved for image format ID*/
} camera_settings_t;

//-----------------------------------------------------------------------------
class CameraControl : public QObject
{
    Q_OBJECT
public:
    CameraControl(QObject* parent = NULL);
    ~CameraControl() {}

    //-- Camera Control
    enum VideoStatus {
        VIDEO_CAPTURE_STATUS_STOPPED = 0,
        VIDEO_CAPTURE_STATUS_RUNNING,
        VIDEO_CAPTURE_STATUS_UNDEFINED
    };

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

    quint32     currentVideoRes     () { return _currentVideoRes; }
    quint32     currentWb           () { return _currentWb; }
    quint32     currentIso          () { return _currentIso; }
    quint32     currentShutter      () { return _currentShutter; }

    void        setCameraMode       (CameraMode mode);
    void        setVehicle          (Vehicle* vehicle);

    void        setCurrentVideoRes  (quint32 index);
    void        setCurrentWb        (quint32 index);
    void        setCurrentIso       (quint32 index);
    void        setCurrentShutter   (quint32 index);

private slots:
    void    _mavlinkMessageReceived (const mavlink_message_t& message);
    void    _mavCommandResult       (int vehicleId, int component, int command, int result, bool noReponseFromVehicle);
    void    _requestCameraSettings  ();
    void    _requestCaptureStatus   ();

signals:
    void    videoStatusChanged      ();
    void    cameraModeChanged       ();
    void    recordTimeChanged       ();
    void    currentVideoResChanged  ();
    void    currentWbChanged        ();
    void    currentIsoChanged       ();
    void    currentShutterChanged   ();

private:
    void    _handleCaptureStatus    (const mavlink_message_t& message);
    void    _handleCameraSettings   (const mavlink_message_t& message);
    void    _updateAspectRatio      ();
    void    _updateVideoRes         (int w, int h, float f);
    int     _findVideoRes           (int w, int h, float f);
    void    _updateIso              (int iso, int locked);
    void    _updateShutter          (int speed, int locked);
    void    _setSettings_1          (float p1 = DEFAULT_VALUE, float p2 = DEFAULT_VALUE, float p3 = DEFAULT_VALUE, float p4 = DEFAULT_VALUE, float p5 = DEFAULT_VALUE, float p6 = DEFAULT_VALUE, float p7 = DEFAULT_VALUE);
    void    _setSettings_2          (float p1 = DEFAULT_VALUE, float p2 = DEFAULT_VALUE, float p3 = DEFAULT_VALUE, float p4 = DEFAULT_VALUE, float p5 = DEFAULT_VALUE, float p6 = DEFAULT_VALUE);

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

    int                     _cameraSupported;
    quint32                 _currentVideoRes;
    quint32                 _currentWb;
    quint32                 _currentIso;
    quint32                 _currentShutter;

    //-- This should come from the camera. In the mean time, we keep track of it here.
    QTime                   _recordTime;
};
