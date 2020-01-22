/****************************************************************************
 *
 * (c) 2009-2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 * @file
 *   @brief Camera Controller
 *   @author Gus Grubba <gus@auterion.com>
 *
 */

#pragma once

#include "QGCCameraControl.h"

#include <QSize>
#include <QPoint>

Q_DECLARE_LOGGING_CATEGORY(CustomCameraLog)
Q_DECLARE_LOGGING_CATEGORY(CustomCameraVerboseLog)

//-----------------------------------------------------------------------------
class CustomCameraControl : public QGCCameraControl
{
    Q_OBJECT
public:
    CustomCameraControl(const mavlink_camera_information_t* info, Vehicle* vehicle, int compID, QObject* parent = nullptr);

    Q_PROPERTY(Fact*        irPalette       READ    irPalette           NOTIFY parametersReady)

    Fact*       irPalette           ();

    bool        takePhoto           () override;
    bool        stopTakePhoto       () override;
    bool        startVideo          () override;
    bool        stopVideo           () override;
    void        setVideoMode        () override;
    void        setPhotoMode        () override;
    void        handleCaptureStatus (const mavlink_camera_capture_status_t& capStatus) override;
    void        setThermalMode      (ThermalViewMode mode) override;

protected:
    void    _setVideoStatus         (VideoStatus status) override;

};
