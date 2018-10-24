/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VideoSettings.h"

#include <QQmlEngine>
#include <QtQml>
#include <QVariantList>

#ifndef QGC_DISABLE_UVC
#include <QCameraInfo>
#endif

const char* VideoSettings::name =                   "Video";
const char* VideoSettings::settingsGroup =          ""; // settings are in root group

const char* VideoSettings::videoSourceName =        "VideoSource";
const char* VideoSettings::udpPortName =            "VideoUDPPort";
const char* VideoSettings::rtspUrlName =            "VideoRTSPUrl";
const char* VideoSettings::tcpUrlName =             "VideoTCPUrl";
const char* VideoSettings::videoAspectRatioName =   "VideoAspectRatio";
const char* VideoSettings::videoGridLinesName =     "VideoGridLines";
const char* VideoSettings::showRecControlName =     "ShowRecControl";
const char* VideoSettings::recordingFormatName =    "RecordingFormat";
const char* VideoSettings::maxVideoSizeName =       "MaxVideoSize";
const char* VideoSettings::enableStorageLimitName = "EnableStorageLimit";
const char* VideoSettings::rtspTimeoutName =        "RtspTimeout";
const char* VideoSettings::streamEnabledName =      "StreamEnabled";
const char* VideoSettings::disableWhenDisarmedName ="DisableWhenDisarmed";

const char* VideoSettings::videoSourceNoVideo =     "No Video Available";
const char* VideoSettings::videoDisabled =          "Video Stream Disabled";
const char* VideoSettings::videoSourceUDP =         "UDP Video Stream";
const char* VideoSettings::videoSourceRTSP =        "RTSP Video Stream";
const char* VideoSettings::videoSourceTCP =         "TCP-MPEG2 Video Stream";

VideoSettings::VideoSettings(QObject* parent)
    : SettingsGroup(name, settingsGroup, parent)
    , _videoSourceFact(NULL)
    , _udpPortFact(NULL)
    , _tcpUrlFact(NULL)
    , _rtspUrlFact(NULL)
    , _videoAspectRatioFact(NULL)
    , _gridLinesFact(NULL)
    , _showRecControlFact(NULL)
    , _recordingFormatFact(NULL)
    , _maxVideoSizeFact(NULL)
    , _enableStorageLimitFact(NULL)
    , _rtspTimeoutFact(NULL)
    , _streamEnabledFact(NULL)
    , _disableWhenDisarmedFact(NULL)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    qmlRegisterUncreatableType<VideoSettings>("QGroundControl.SettingsManager", 1, 0, "VideoSettings", "Reference only");

    // Setup enum values for videoSource settings into meta data
    bool noVideo = false;
    QStringList videoSourceList;
#ifdef QGC_GST_STREAMING
#ifndef NO_UDP_VIDEO
    videoSourceList.append(videoSourceUDP);
#endif
    videoSourceList.append(videoSourceRTSP);
    videoSourceList.append(videoSourceTCP);
#endif
#ifndef QGC_DISABLE_UVC
    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    for (const QCameraInfo &cameraInfo: cameras) {
        videoSourceList.append(cameraInfo.description());
    }
#endif
    if (videoSourceList.count() == 0) {
        noVideo = true;
        videoSourceList.append(videoSourceNoVideo);
    } else {
        videoSourceList.insert(0, videoDisabled);
    }
    QVariantList videoSourceVarList;
    for (const QString& videoSource: videoSourceList) {
        videoSourceVarList.append(QVariant::fromValue(videoSource));
    }
    _nameToMetaDataMap[videoSourceName]->setEnumInfo(videoSourceList, videoSourceVarList);

    // Set default value for videoSource
    if (noVideo) {
        _nameToMetaDataMap[videoSourceName]->setRawDefaultValue(videoSourceNoVideo);
    } else {
        _nameToMetaDataMap[videoSourceName]->setRawDefaultValue(videoDisabled);
    }
}

Fact* VideoSettings::videoSource(void)
{
    if (!_videoSourceFact) {
        _videoSourceFact = _createSettingsFact(videoSourceName);
        connect(_videoSourceFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _videoSourceFact;
}

Fact* VideoSettings::udpPort(void)
{
    if (!_udpPortFact) {
        _udpPortFact = _createSettingsFact(udpPortName);
        connect(_udpPortFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _udpPortFact;
}

Fact* VideoSettings::rtspUrl(void)
{
    if (!_rtspUrlFact) {
        _rtspUrlFact = _createSettingsFact(rtspUrlName);
        connect(_rtspUrlFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _rtspUrlFact;
}

Fact* VideoSettings::tcpUrl(void)
{
    if (!_tcpUrlFact) {
        _tcpUrlFact = _createSettingsFact(tcpUrlName);
        connect(_tcpUrlFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _tcpUrlFact;
}

Fact* VideoSettings::aspectRatio(void)
{
    if (!_videoAspectRatioFact) {
        _videoAspectRatioFact = _createSettingsFact(videoAspectRatioName);
    }
    return _videoAspectRatioFact;
}

Fact* VideoSettings::gridLines(void)
{
    if (!_gridLinesFact) {
        _gridLinesFact = _createSettingsFact(videoGridLinesName);
    }
    return _gridLinesFact;
}

Fact* VideoSettings::showRecControl(void)
{
    if (!_showRecControlFact) {
        _showRecControlFact = _createSettingsFact(showRecControlName);
    }
    return _showRecControlFact;
}

Fact* VideoSettings::recordingFormat(void)
{
    if (!_recordingFormatFact) {
        _recordingFormatFact = _createSettingsFact(recordingFormatName);
    }
    return _recordingFormatFact;
}

Fact* VideoSettings::maxVideoSize(void)
{
    if (!_maxVideoSizeFact) {
        _maxVideoSizeFact = _createSettingsFact(maxVideoSizeName);
    }
    return _maxVideoSizeFact;
}

Fact* VideoSettings::enableStorageLimit(void)
{
    if (!_enableStorageLimitFact) {
        _enableStorageLimitFact = _createSettingsFact(enableStorageLimitName);
    }
    return _enableStorageLimitFact;
}

Fact* VideoSettings::rtspTimeout(void)
{
    if (!_rtspTimeoutFact) {
        _rtspTimeoutFact = _createSettingsFact(rtspTimeoutName);
    }
    return _rtspTimeoutFact;
}

Fact* VideoSettings::streamEnabled(void)
{
    if (!_streamEnabledFact) {
        _streamEnabledFact = _createSettingsFact(streamEnabledName);
    }
    return _streamEnabledFact;
}

Fact* VideoSettings::disableWhenDisarmed(void)
{
    if (!_disableWhenDisarmedFact) {
        _disableWhenDisarmedFact = _createSettingsFact(disableWhenDisarmedName);
    }
    return _disableWhenDisarmedFact;
}

bool VideoSettings::streamConfigured(void)
{
#if !defined(QGC_GST_STREAMING)
    return false;
#endif
    QString vSource = videoSource()->rawValue().toString();
    if(vSource == videoSourceNoVideo || vSource == videoDisabled) {
        return false;
    }
    //-- If UDP, check if port is set
    if(vSource == videoSourceUDP) {
        return udpPort()->rawValue().toInt() != 0;
    }
    //-- If RTSP, check for URL
    if(vSource == videoSourceRTSP) {
        return !rtspUrl()->rawValue().toString().isEmpty();
    }
    //-- If TCP, check for URL
    if(vSource == videoSourceTCP) {
        return !tcpUrl()->rawValue().toString().isEmpty();
    }
    return false;
}

void VideoSettings::_configChanged(QVariant)
{
    emit streamConfiguredChanged();
}
