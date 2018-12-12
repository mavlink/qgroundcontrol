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

Q_DECLARE_LOGGING_CATEGORY(AuterionCameraLog)
Q_DECLARE_LOGGING_CATEGORY(AuterionCameraLogVerbose)

//-----------------------------------------------------------------------------
class AuterionCameraControl : public QGCCameraControl
{
    Q_OBJECT
public:
    AuterionCameraControl(const mavlink_camera_information_t* info, Vehicle* vehicle, int compID, QObject* parent = nullptr);

    Q_PROPERTY(qreal        gimbalRoll      READ    gimbalRoll          NOTIFY gimbalRollChanged)
    Q_PROPERTY(qreal        gimbalPitch     READ    gimbalPitch         NOTIFY gimbalPitchChanged)
    Q_PROPERTY(qreal        gimbalYaw       READ    gimbalYaw           NOTIFY gimbalYawChanged)
    Q_PROPERTY(bool         gimbalData      READ    gimbalData          NOTIFY gimbalDataChanged)
    Q_PROPERTY(quint32      recordTime      READ    recordTime          NOTIFY recordTimeChanged)
    Q_PROPERTY(QString      recordTimeStr   READ    recordTimeStr       NOTIFY recordTimeChanged)

    bool        takePhoto           () override;
    bool        stopTakePhoto       () override;
    bool        startVideo          () override;
    bool        stopVideo           () override;
    void        setVideoMode        () override;
    void        setPhotoMode        () override;
    void        handleCaptureStatus (const mavlink_camera_capture_status_t& capStatus) override;

    qreal       gimbalRoll          () { return static_cast<qreal>(_gimbalRoll);}
    qreal       gimbalPitch         () { return static_cast<qreal>(_gimbalPitch); }
    qreal       gimbalYaw           () { return static_cast<qreal>(_gimbalYaw); }
    bool        gimbalData          () { return _gimbalData; }
    quint32     recordTime          () { return _recordTime; }
    QString     recordTimeStr       ();

private slots:
    void    _recTimerHandler        ();
    void    _mavlinkMessageReceived (const mavlink_message_t& message);

signals:
    void    recordTimeChanged       ();
    void    gimbalRollChanged       ();
    void    gimbalPitchChanged      ();
    void    gimbalYawChanged        ();
    void    gimbalDataChanged       ();

protected:
    void    _setVideoStatus         (VideoStatus status) override;

private:
    void    _handleGimbalOrientation(const mavlink_message_t& message);

private:
    QSoundEffect            _cameraSound;
    QSoundEffect            _videoSound;
    QSoundEffect            _errorSound;
    QTimer                  _recTimer;
    QTime                   _recTime;
    float                   _gimbalRoll         = 0.0;
    float                   _gimbalPitch        = 0.0;
    float                   _gimbalYaw          = 0.0;
    uint32_t                _recordTime         = 0;
    bool                    _firstPhotoLapse    = false;
    bool                    _gimbalData         = false;
};
