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

class VideoSettings : public SettingsGroup
{
    Q_OBJECT

public:
    VideoSettings(QObject* parent = NULL);

    Q_PROPERTY(Fact* videoSource    READ videoSource    CONSTANT)
    Q_PROPERTY(Fact* udpPort        READ udpPort        CONSTANT)
    Q_PROPERTY(Fact* rtspUrl        READ rtspUrl        CONSTANT)
    Q_PROPERTY(Fact* videoSavePath  READ videoSavePath  CONSTANT)
    Q_PROPERTY(Fact* aspectRatio    READ aspectRatio    CONSTANT)

    Fact* videoSource   (void);
    Fact* udpPort       (void);
    Fact* rtspUrl       (void);
    Fact* videoSavePath (void);
    Fact* aspectRatio   (void);

    static const char* videoSettingsGroupName;

    static const char* videoSourceName;
    static const char* udpPortName;
    static const char* rtspUrlName;
    static const char* videoSavePathName;
    static const char* videoAspectRatioName;

    static const char* videoSourceNoVideo;
    static const char* videoSourceUDP;
    static const char* videoSourceRTSP;

private:
    SettingsFact* _videoSourceFact;
    SettingsFact* _udpPortFact;
    SettingsFact* _rtspUrlFact;
    SettingsFact* _videoSavePathFact;
    SettingsFact* _videoAspectRatioFact;
};

#endif
