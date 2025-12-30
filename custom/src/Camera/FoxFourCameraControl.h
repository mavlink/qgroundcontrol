#pragma once

#include "MavlinkCameraControl.h"
#include "VehicleCameraControl.h"

#include <QtCore/QElapsedTimer>

class FoxFourCameraControl : public VehicleCameraControl
{
    Q_OBJECT
    Q_PROPERTY(bool zoomEnabled READ zoomEnabled NOTIFY zoomEnabledChanged)
public:
    FoxFourCameraControl(const mavlink_camera_information_t* info, Vehicle* vehicle, int compID, QObject* parent = nullptr);
    virtual ~FoxFourCameraControl();

    Q_INVOKABLE virtual bool startVideoRecording    ();
    Q_INVOKABLE virtual bool stopVideoRecording     ();
    Q_INVOKABLE virtual void startTracking          (QRectF rec, QString timestamp, bool zoom);
    Q_INVOKABLE virtual void stopTracking           (uint64_t timestamp=0);
    // Q_INVOKABLE virtual void zoom                   (QRectF rec);
    void setZoomLevel(qreal level);

    virtual void handleSettings (const mavlink_camera_settings_t& settings);

signals:
    void zoomEnabledChanged();
public slots:
    bool zoomEnabled(){return _zoomEnabled;}

protected slots:
    virtual void _processRecordingChanged();

protected:
    QTimer        _videoRecordTimeUpdateTimer;
    QElapsedTimer _videoRecordTimeElapsedTimer;
    bool _zoomEnabled = false;
};
