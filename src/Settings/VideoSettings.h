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
    VideoSettings(QObject* parent = nullptr);

    Q_PROPERTY(Fact* videoSource            READ videoSource            CONSTANT)
    Q_PROPERTY(Fact* udpPort                READ udpPort                CONSTANT)
    Q_PROPERTY(Fact* tcpUrl                 READ tcpUrl                 CONSTANT)
    Q_PROPERTY(Fact* rtspUrl                READ rtspUrl                CONSTANT)
    Q_PROPERTY(Fact* aspectRatio            READ aspectRatio            CONSTANT)
    Q_PROPERTY(Fact* videoFit               READ videoFit               CONSTANT)
    Q_PROPERTY(Fact* gridLines              READ gridLines              CONSTANT)
    Q_PROPERTY(Fact* showRecControl         READ showRecControl         CONSTANT)
    Q_PROPERTY(Fact* recordingFormat        READ recordingFormat        CONSTANT)
    Q_PROPERTY(Fact* maxVideoSize           READ maxVideoSize           CONSTANT)
    Q_PROPERTY(Fact* enableStorageLimit     READ enableStorageLimit     CONSTANT)
    Q_PROPERTY(Fact* rtspTimeout            READ rtspTimeout            CONSTANT)
    Q_PROPERTY(Fact* streamEnabled          READ streamEnabled          CONSTANT)
    Q_PROPERTY(Fact* disableWhenDisarmed    READ disableWhenDisarmed    CONSTANT)
    Q_PROPERTY(bool  streamConfigured       READ streamConfigured       NOTIFY streamConfiguredChanged)

    Fact* videoSource           ();
    Fact* udpPort               ();
    Fact* rtspUrl               ();
    Fact* tcpUrl                ();
    Fact* aspectRatio           ();
    Fact* gridLines             ();
    Fact* videoFit              ();
    Fact* showRecControl        ();
    Fact* recordingFormat       ();
    Fact* maxVideoSize          ();
    Fact* enableStorageLimit    ();
    Fact* rtspTimeout           ();
    Fact* streamEnabled         ();
    Fact* disableWhenDisarmed   ();
    bool  streamConfigured      ();

    static const char* name;
    static const char* settingsGroup;

    static const char* videoSourceName;
    static const char* udpPortName;
    static const char* rtspUrlName;
    static const char* tcpUrlName;
    static const char* videoAspectRatioName;
    static const char* videoFitName;
    static const char* videoGridLinesName;
    static const char* showRecControlName;
    static const char* recordingFormatName;
    static const char* maxVideoSizeName;
    static const char* enableStorageLimitName;
    static const char* rtspTimeoutName;
    static const char* streamEnabledName;
    static const char* disableWhenDisarmedName;

    static const char* videoSourceNoVideo;
    static const char* videoDisabled;
    static const char* videoSourceUDP;
    static const char* videoSourceRTSP;
    static const char* videoSourceTCP;

signals:
    void streamConfiguredChanged    ();

private slots:
    void _configChanged             (QVariant value);

private:
    SettingsFact* _videoSourceFact;
    SettingsFact* _udpPortFact;
    SettingsFact* _tcpUrlFact;
    SettingsFact* _rtspUrlFact;
    SettingsFact* _videoAspectRatioFact;
    SettingsFact* _videoFitFact;
    SettingsFact* _gridLinesFact;
    SettingsFact* _showRecControlFact;
    SettingsFact* _recordingFormatFact;
    SettingsFact* _maxVideoSizeFact;
    SettingsFact* _enableStorageLimitFact;
    SettingsFact* _rtspTimeoutFact;
    SettingsFact* _streamEnabledFact;
    SettingsFact* _disableWhenDisarmedFact;
};

#endif
