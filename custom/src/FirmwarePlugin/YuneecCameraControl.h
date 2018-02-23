/*!
 * @file
 *   @brief Camera Controller
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#pragma once

#include "QGCCameraControl.h"

#include <QSoundEffect>
#include <QSize>
#include <QPoint>

#if defined(__androidx86__)
#include "m4lib.h" // for button states
#endif

Q_DECLARE_LOGGING_CATEGORY(YuneecCameraLog)
Q_DECLARE_LOGGING_CATEGORY(YuneecCameraLogVerbose)

//-- Temperature Status (CGOET/E10T)
typedef struct
{
    int32_t center_val;
    int32_t max_val;
    int32_t min_val;
    int32_t avg_val;
} udp_ctrl_cam_area_temp_t;

typedef struct
{
    int32_t  locked_max_temp;
    int32_t  locked_min_temp;
    udp_ctrl_cam_area_temp_t all_area;
    udp_ctrl_cam_area_temp_t custom_area;
} udp_ctrl_cam_lepton_area_temp_t;

typedef struct
{
    const char* name;
    float       temp_min;
    float       temp_max;
    uint32_t    palette;
} cgoet_presets_t;

//-----------------------------------------------------------------------------
class YuneecCameraControl : public QGCCameraControl
{
    Q_OBJECT
public:
    YuneecCameraControl(const mavlink_camera_information_t* info, Vehicle* vehicle, int compID, QObject* parent = NULL);

    Q_PROPERTY(QString      gimbalVersion   READ    gimbalVersion       NOTIFY gimbalVersionChanged)
    Q_PROPERTY(bool         gimbalCalOn     READ    gimbalCalOn         NOTIFY gimbalCalOnChanged)
    Q_PROPERTY(int          gimbalProgress  READ    gimbalProgress      NOTIFY gimbalProgressChanged)
    Q_PROPERTY(qreal        gimbalRoll      READ    gimbalRoll          NOTIFY gimbalRollChanged)
    Q_PROPERTY(qreal        gimbalPitch     READ    gimbalPitch         NOTIFY gimbalPitchChanged)
    Q_PROPERTY(qreal        gimbalYaw       READ    gimbalYaw           NOTIFY gimbalYawChanged)
    Q_PROPERTY(bool         gimbalData      READ    gimbalData          NOTIFY gimbalDataChanged)
    Q_PROPERTY(quint32      recordTime      READ    recordTime          NOTIFY recordTimeChanged)
    Q_PROPERTY(QString      recordTimeStr   READ    recordTimeStr       NOTIFY recordTimeChanged)
    Q_PROPERTY(Fact*        exposureMode    READ    exposureMode        NOTIFY factsLoaded)
    Q_PROPERTY(Fact*        ev              READ    ev                  NOTIFY factsLoaded)
    Q_PROPERTY(Fact*        iso             READ    iso                 NOTIFY factsLoaded)
    Q_PROPERTY(Fact*        shutterSpeed    READ    shutterSpeed        NOTIFY factsLoaded)
    Q_PROPERTY(Fact*        wb              READ    wb                  NOTIFY factsLoaded)
    Q_PROPERTY(Fact*        meteringMode    READ    meteringMode        NOTIFY factsLoaded)
    Q_PROPERTY(Fact*        videoRes        READ    videoRes            NOTIFY factsLoaded)
    Q_PROPERTY(Fact*        aspectRatio     READ    aspectRatio         NOTIFY factsLoaded)
    Q_PROPERTY(Fact*        irPalette       READ    irPalette           NOTIFY factsLoaded)
    Q_PROPERTY(Fact*        irROI           READ    irROI               NOTIFY factsLoaded)
    Q_PROPERTY(Fact*        irPresets       READ    irPresets           NOTIFY factsLoaded)
    Q_PROPERTY(QPoint       spotArea        READ    spotArea            WRITE  setSpotArea      NOTIFY spotAreaChanged)
    Q_PROPERTY(QSize        videoSize       READ    videoSize           WRITE  setVideoSize     NOTIFY videoSizeChanged)
    Q_PROPERTY(bool         isCGOET         READ    isCGOET             NOTIFY isCGOETChanged)
    Q_PROPERTY(bool         isE10T          READ    isE10T              NOTIFY isE10TChanged)
    Q_PROPERTY(bool         isThermal       READ    isThermal           NOTIFY isThermalChanged)
    Q_PROPERTY(bool         isE90           READ    isE90               NOTIFY isE90Changed)
    Q_PROPERTY(bool         paramComplete   READ    paramComplete       NOTIFY factsLoaded)

    Q_PROPERTY(qreal        irCenterTemp    READ    irCenterTemp        NOTIFY irTempChanged)
    Q_PROPERTY(qreal        irAverageTemp   READ    irAverageTemp       NOTIFY irTempChanged)
    Q_PROPERTY(qreal        irMinTemp       READ    irMinTemp           NOTIFY irTempChanged)
    Q_PROPERTY(qreal        irMaxTemp       READ    irMaxTemp           NOTIFY irTempChanged)
    Q_PROPERTY(QUrl         palettetBar     READ    palettetBar         NOTIFY palettetBarChanged)
    Q_PROPERTY(bool         irValid         READ    irValid             NOTIFY irTempChanged)

    Q_INVOKABLE void calibrateGimbal();

    bool        takePhoto           () override;
    bool        stopTakePhoto       () override;
    bool        startVideo          () override;
    bool        stopVideo           () override;
    QString     firmwareVersion     () override;
    void        factChanged         (Fact* pFact) override;
    void        setVideoMode        () override;
    void        setPhotoMode        () override;
    bool        incomingParameter   (Fact* pFact, QVariant& newValue) override;
    bool        validateParameter   (Fact* pFact, QVariant& newValue) override;
    void        handleCaptureStatus (const mavlink_camera_capture_status_t& capStatus) override;
    void        resetSettings       () override;

    QString     gimbalVersion       () { return _gimbalVersion; }
    bool        gimbalCalOn         () { return _gimbalCalOn; }
    int         gimbalProgress      () { return _gimbalProgress; }
    quint32     recordTime          () { return _recordTime; }
    QString     recordTimeStr       ();
    qreal       gimbalRoll          () { return _gimbalRoll;}
    qreal       gimbalPitch         () { return _gimbalPitch; }
    qreal       gimbalYaw           () { return _gimbalYaw; }
    bool        gimbalData          () { return _gimbalData; }
    Fact*       exposureMode        ();
    Fact*       ev                  ();
    Fact*       iso                 ();
    Fact*       shutterSpeed        ();
    Fact*       wb                  ();
    Fact*       meteringMode        ();
    Fact*       videoRes            ();
    Fact*       aspectRatio         ();
    Fact*       irPalette           ();
    Fact*       irROI               ();
    Fact*       irPresets           ();
    QPoint      spotArea            ();
    void        setSpotArea         (QPoint mouse);

    QSize       videoSize           ();
    void        setVideoSize        (QSize s);
    bool        isVideoRecording    () { return _videoRecording; }

    bool        isCGOET             () { return _isCGOET; }
    bool        isE10T              () { return _isE10T; }
    bool        isThermal           () { return _isCGOET || _isE10T; }
    bool        isE90               () { return _isE90; }
    bool        paramComplete       () { return _paramComplete; }

    qreal       irCenterTemp        ();
    qreal       irAverageTemp       ();
    qreal       irMinTemp           ();
    qreal       irMaxTemp           ();
    bool        irValid             () { return _irValid; }
    QUrl        palettetBar         ();

private slots:
    void    _recTimerHandler        ();
    void    _setCameraTime          ();
    void    _mavlinkMessageReceived (const mavlink_message_t& message);
#if defined(__androidx86__)
    void    _buttonStateChanged     (M4Lib::ButtonId buttonId, M4Lib::ButtonState buttonState);
#endif
    void    _parametersReady        ();
    void    _sendUpdates            ();
    void    _delayedStartVideo      ();
    void    _delayedTakePhoto       ();
    void    _gimbalCalTimeout       ();
    void    _irStatusTimeout        ();
    void    _resumeIrStatus         ();
    void    _presetChanged          (QVariant value);

signals:
    void    gimbalVersionChanged    ();
    void    gimbalProgressChanged   ();
    void    gimbalCalOnChanged      ();
    void    recordTimeChanged       ();
    void    factsLoaded             ();
    void    gimbalRollChanged       ();
    void    gimbalPitchChanged      ();
    void    gimbalYawChanged        ();
    void    gimbalDataChanged       ();
    void    spotAreaChanged         ();
    void    videoSizeChanged        ();
    void    isCGOETChanged          ();
    void    isE10TChanged           ();
    void    isThermalChanged        ();
    void    isE90Changed            ();
    void    irTempChanged           ();
    void    palettetBarChanged      ();
    void    irSpotROIChanged        ();
    void    isVideoRecordingChanged ();

protected:
    void    _setVideoStatus         (VideoStatus status) override;

private:
    void    _handleHeartBeat        (const mavlink_message_t& message);
    void    _handleCommandAck       (const mavlink_message_t& message);
    void    _handleHardwareVersion    (const mavlink_message_t& message);
    void    _handleGimbalOrientation(const mavlink_message_t& message);
    void    _handleGimbalResult     (uint16_t result, uint8_t progress);

    QVariant _validateShutterSpeed  (Fact* pFact, QVariant& newValue);
    QVariant _validateISO           (Fact* pFact, QVariant& newValue);

private:
    Vehicle*                _vehicle;
    bool                    _gimbalCalOn;
    int                     _gimbalProgress;
    float                   _gimbalRoll;
    float                   _gimbalPitch;
    float                   _gimbalYaw;
    bool                    _gimbalData;
    QString                 _gimbalVersion;
    QSoundEffect            _cameraSound;
    QSoundEffect            _videoSound;
    QSoundEffect            _errorSound;
    QTimer                  _setTimeTimer;
    QTimer                  _irStatusTimer;
    QTimer                  _gimbalTimer;
    QTimer                  _recTimer;
    QTime                   _recTime;
    uint32_t                _recordTime;
    bool                    _paramComplete;
    QString                 _version;
    QSize                   _videoSize;
    bool                    _isE90;
    bool                    _isCGOET;
    bool                    _isE10T;
    QStringList             _updatesToSend;
    bool                    _inMissionMode;
    bool                    _irValid;
    bool                    _firstPhotoLapse;
    SettingsFact*           _irROI;
    SettingsFact*           _irPresets;
    udp_ctrl_cam_lepton_area_temp_t _cgoetTempStatus;
    bool                    _videoRecording;
};
