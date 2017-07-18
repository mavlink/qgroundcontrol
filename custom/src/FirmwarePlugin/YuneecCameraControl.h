/*!
 * @file
 *   @brief Camera Controller
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#pragma once

#include "QGCCameraControl.h"

#include <QSoundEffect>

Q_DECLARE_LOGGING_CATEGORY(YuneecCameraLog)
Q_DECLARE_LOGGING_CATEGORY(YuneecCameraLogVerbose)

//-----------------------------------------------------------------------------
class YuneecCameraControl : public QGCCameraControl
{
    Q_OBJECT
public:
    YuneecCameraControl(const mavlink_camera_information_t* info, Vehicle* vehicle, int compID, QObject* parent = NULL);

    Q_PROPERTY(QString      gimbalVersion   READ    gimbalVersion                               NOTIFY gimbalVersionChanged)
    Q_PROPERTY(bool         gimbalCalOn     READ    gimbalCalOn                                 NOTIFY gimbalCalOnChanged)
    Q_PROPERTY(int          gimbalProgress  READ    gimbalProgress                              NOTIFY gimbalProgressChanged)
    Q_PROPERTY(quint32      recordTime      READ    recordTime                                  NOTIFY recordTimeChanged)
    Q_PROPERTY(QString      recordTimeStr   READ    recordTimeStr                               NOTIFY recordTimeChanged)

    Q_INVOKABLE void calibrateGimbal();

    bool        takePhoto           () override;
    bool        startVideo          () override;
    bool        stopVideo           () override;

    QString     gimbalVersion       () { return _gimbalVersion; }
    bool        gimbalCalOn         () { return _gimbalCalOn; }
    int         gimbalProgress      () { return _gimbalProgress; }
    quint32     recordTime          () { return _recordTime; }
    QString     recordTimeStr       ();

private slots:
    void    _recTimerHandler        ();
    void    _mavlinkMessageReceived (const mavlink_message_t& message);
    void    _switchStateChanged     (int swId, int oldState, int newState);

signals:
    void    gimbalVersionChanged    ();
    void    gimbalProgressChanged   ();
    void    gimbalCalOnChanged      ();
    void    recordTimeChanged       ();

protected:
    void    _setVideoStatus         (VideoStatus status) override;
    void    _setCameraMode          (CameraMode mode) override;

private:
    void    _handleCommandAck       (const mavlink_message_t& message);
    void    _handleGimbalVersion    (const mavlink_message_t& message);
    void    _handleGimbalResult     (uint16_t result, uint8_t progress);

private:
    Vehicle*                _vehicle;
    bool                    _gimbalCalOn;
    int                     _gimbalProgress;
    QString                 _gimbalVersion;
    QSoundEffect            _cameraSound;
    QSoundEffect            _videoSound;
    QSoundEffect            _errorSound;
    QTimer                  _recTimer;
    QTime                   _recTime;
    uint32_t                _recordTime;
};
