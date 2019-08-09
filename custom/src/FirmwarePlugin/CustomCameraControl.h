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
Q_DECLARE_LOGGING_CATEGORY(AuterionCameraVerboseLog)

//-----------------------------------------------------------------------------
class AuterionCameraControl : public QGCCameraControl
{
    Q_OBJECT
public:
    AuterionCameraControl(const mavlink_camera_information_t* info, Vehicle* vehicle, int compID, QObject* parent = nullptr);

    Q_PROPERTY(Fact*        videoRes        READ    videoRes            NOTIFY parametersReady)
    Q_PROPERTY(Fact*        irPalette       READ    irPalette           NOTIFY parametersReady)

    bool        takePhoto           () override;
    bool        stopTakePhoto       () override;
    bool        startVideo          () override;
    bool        stopVideo           () override;
    void        setVideoMode        () override;
    void        setPhotoMode        () override;
    void        handleCaptureStatus (const mavlink_camera_capture_status_t& capStatus) override;

    Fact*       videoRes            ();
    Fact*       irPalette           ();

protected:
    void    _setVideoStatus         (VideoStatus status) override;

private:
    QSoundEffect            _cameraSound;
    QSoundEffect            _videoSound;
    QSoundEffect            _errorSound;
    bool                    _firstPhotoLapse    = false;
};
