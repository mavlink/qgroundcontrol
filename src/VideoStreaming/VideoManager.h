/****************************************************************************
 *
 *   (c) 2009-2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef VideoManager_H
#define VideoManager_H

#include <QObject>
#include <QTimer>
#include <QTime>
#include <QUrl>

#include "QGCMAVLink.h"
#include "QGCLoggingCategory.h"
#include "VideoReceiver.h"
#include "QGCToolbox.h"
#include "SubtitleWriter.h"

Q_DECLARE_LOGGING_CATEGORY(VideoManagerLog)

class VideoSettings;
class Vehicle;
class Joystick;

class VideoManager : public QGCTool
{
    Q_OBJECT

public:
    VideoManager    (QGCApplication* app, QGCToolbox* toolbox);
    virtual ~VideoManager   ();

    Q_PROPERTY(bool             hasVideo                READ    hasVideo                                    NOTIFY hasVideoChanged)
    Q_PROPERTY(bool             isGStreamer             READ    isGStreamer                                 NOTIFY isGStreamerChanged)
    Q_PROPERTY(bool             isTaisync               READ    isTaisync       WRITE   setIsTaisync        NOTIFY isTaisyncChanged)
    Q_PROPERTY(QString          videoSourceID           READ    videoSourceID                               NOTIFY videoSourceIDChanged)
    Q_PROPERTY(bool             uvcEnabled              READ    uvcEnabled                                  CONSTANT)
    Q_PROPERTY(bool             fullScreen              READ    fullScreen      WRITE   setfullScreen       NOTIFY fullScreenChanged)
    Q_PROPERTY(double           aspectRatio             READ    aspectRatio                                 NOTIFY aspectRatioChanged)
    Q_PROPERTY(double           thermalAspectRatio      READ    thermalAspectRatio                          NOTIFY aspectRatioChanged)
    Q_PROPERTY(double           hfov                    READ    hfov                                        NOTIFY aspectRatioChanged)
    Q_PROPERTY(double           thermalHfov             READ    thermalHfov                                 NOTIFY aspectRatioChanged)
    Q_PROPERTY(bool             autoStreamConfigured    READ    autoStreamConfigured                        NOTIFY autoStreamConfiguredChanged)
    Q_PROPERTY(bool             hasThermal              READ    hasThermal                                  NOTIFY aspectRatioChanged)
    Q_PROPERTY(VideoReceiver* videoReceiver MEMBER _videoReceiver CONSTANT)

    virtual bool        hasVideo            ();
    virtual bool        isGStreamer         ();
    virtual bool        isTaisync           () { return _isTaisync; }
    virtual bool        fullScreen          () { return _fullScreen; }
    virtual QString     videoSourceID       () { return _videoSourceID; }
    virtual double      aspectRatio         ();
    virtual double      thermalAspectRatio  ();
    virtual double      hfov                ();
    virtual double      thermalHfov         ();
    virtual bool        autoStreamConfigured();
    virtual bool        hasThermal          ();

#if defined(QGC_DISABLE_UVC)
    virtual bool        uvcEnabled          () { return false; }
#else
    virtual bool        uvcEnabled          ();
#endif

    virtual void        setfullScreen       (bool f);
    virtual void        setIsTaisync        (bool t) { _isTaisync = t;  emit isTaisyncChanged(); }

    // Override from QGCTool
    virtual void        setToolbox          (QGCToolbox *toolbox);

signals:
    void hasVideoChanged            ();
    void isGStreamerChanged         ();
    void videoSourceIDChanged       ();
    void fullScreenChanged          ();
    void isAutoStreamChanged        ();
    void isTaisyncChanged           ();
    void aspectRatioChanged         ();
    void autoStreamConfiguredChanged();

protected slots:
    void _videoSourceChanged        ();
    void _updateUVC                 ();
    void _setActiveVehicle          (Vehicle* vehicle);
    void _aspectRatioChanged        ();
    void _connectionLostChanged     (bool connectionLost);

protected:
    void _updateSettings            ();

protected:
    SubtitleWriter  _subtitleWriter;
    bool            _isTaisync              = false;
    VideoSettings*  _videoSettings          = nullptr;
    VideoReceiver* _videoReceiver = nullptr;
    QString         _videoSourceID;
    bool            _fullScreen             = false;
    Vehicle*        _activeVehicle          = nullptr;
};

#endif
