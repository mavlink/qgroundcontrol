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

const char* VideoSettings::videoSettingsGroupName = "Video";

const char* VideoSettings::videoSourceName =        "VideoSource";
const char* VideoSettings::udpPortName =            "VideoUDPPort";
const char* VideoSettings::rtspUrlName =            "VideoRTSPUrl";
const char* VideoSettings::videoSavePathName =      "VideoSavePath";
const char* VideoSettings::videoAspectRatioName =   "VideoAspectRatio";
const char* VideoSettings::videoGridLinesName =     "VideoGridLines";

const char* VideoSettings::videoSourceNoVideo = "No Video Available";
const char* VideoSettings::videoSourceUDP =     "UDP Video Stream";
const char* VideoSettings::videoSourceRTSP =    "RTSP Video Stream";

VideoSettings::VideoSettings(QObject* parent)
    : SettingsGroup(videoSettingsGroupName, QString() /* root settings group */, parent)
    , _videoSourceFact(NULL)
    , _udpPortFact(NULL)
    , _rtspUrlFact(NULL)
    , _videoSavePathFact(NULL)
    , _videoAspectRatioFact(NULL)
    , _gridLinesFact(NULL)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    qmlRegisterUncreatableType<VideoSettings>("QGroundControl.SettingsManager", 1, 0, "VideoSettings", "Reference only");

    // Setup enum values for videoSource settings into meta data
    QStringList videoSourceList;
#ifdef QGC_GST_STREAMING
#ifndef NO_UDP_VIDEO
    videoSourceList.append(videoSourceUDP);
#endif
    videoSourceList.append(videoSourceRTSP);
#endif
#ifndef QGC_DISABLE_UVC
    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    foreach (const QCameraInfo &cameraInfo, cameras) {
        videoSourceList.append(cameraInfo.description());
    }
#endif
    if (videoSourceList.count() == 0) {
        videoSourceList.append(videoSourceNoVideo);
    }
    QVariantList videoSourceVarList;
    foreach (const QString& videoSource, videoSourceList) {
        videoSourceVarList.append(QVariant::fromValue(videoSource));
    }
    _nameToMetaDataMap[videoSourceName]->setEnumInfo(videoSourceList, videoSourceVarList);

    // Set default value for videoSource
#if defined(NO_UDP_VIDEO)
    _nameToMetaDataMap[videoSourceName]->setRawDefaultValue(videoSourceRTSP);
#else
    _nameToMetaDataMap[videoSourceName]->setRawDefaultValue(videoSourceUDP);
#endif
}

Fact* VideoSettings::videoSource(void)
{
    if (!_videoSourceFact) {
        _videoSourceFact = _createSettingsFact(videoSourceName);
    }

    return _videoSourceFact;
}

Fact* VideoSettings::udpPort(void)
{
    if (!_udpPortFact) {
        _udpPortFact = _createSettingsFact(udpPortName);
    }

    return _udpPortFact;
}

Fact* VideoSettings::rtspUrl(void)
{
    if (!_rtspUrlFact) {
        _rtspUrlFact = _createSettingsFact(rtspUrlName);
    }

    return _rtspUrlFact;
}

Fact* VideoSettings::videoSavePath(void)
{
    if (!_videoSavePathFact) {
        _videoSavePathFact = _createSettingsFact(videoSavePathName);
    }

    return _videoSavePathFact;
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
