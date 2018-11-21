/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef VideoSettings_H
#define VideoSettings_H

#include "SettingsGroup.h"

#ifdef QGC_GST_TAISYNC_USB
#include "TaisyncVideoReceiver.h"
#endif

class VideoSettings : public SettingsGroup
{
    Q_OBJECT

public:
    VideoSettings(QObject* parent = nullptr);
    DEFINE_SETTING_NAME_GROUP()

    DEFINE_SETTINGFACT(videoSource)
    DEFINE_SETTINGFACT(udpPort)
    DEFINE_SETTINGFACT(tcpUrl)
    DEFINE_SETTINGFACT(rtspUrl)
    DEFINE_SETTINGFACT(aspectRatio)
    DEFINE_SETTINGFACT(videoFit)
    DEFINE_SETTINGFACT(gridLines)
    DEFINE_SETTINGFACT(showRecControl)
    DEFINE_SETTINGFACT(recordingFormat)
    DEFINE_SETTINGFACT(maxVideoSize)
    DEFINE_SETTINGFACT(enableStorageLimit)
    DEFINE_SETTINGFACT(rtspTimeout)
    DEFINE_SETTINGFACT(streamEnabled)
    DEFINE_SETTINGFACT(disableWhenDisarmed)

    Q_PROPERTY(bool  streamConfigured       READ streamConfigured       NOTIFY streamConfiguredChanged)

    bool  streamConfigured          ();

    static const char* videoSourceNoVideo;
    static const char* videoDisabled;
    static const char* videoSourceUDP;
    static const char* videoSourceRTSP;
    static const char* videoSourceTCP;
#ifdef QGC_GST_TAISYNC_USB
    static const char* videoSourceTaiSyncUSB;
#endif

signals:
    void streamConfiguredChanged    ();

private slots:
    void _configChanged             (QVariant value);

private:
#ifdef QGC_GST_TAISYNC_USB
    TaisyncVideoReceiver*              _taiSync = nullptr;
#endif
};

#endif
