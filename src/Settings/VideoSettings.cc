#include "VideoSettings.h"
#include "VideoManager.h"

#include "QGCLoggingCategory.h"
#include "QGCNetworkHelper.h"
#include "SecureMemory.h"
#include <QtCore/QFile>
#include <QtCore/QSettings>
#include <QtCore/QUrl>
#include <QtCore/QUrlQuery>
#include <QtCore/QVariantList>

#if defined(Q_OS_UNIX) && !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

QGC_LOGGING_CATEGORY(VideoSettingsLog, "Settings.VideoSettings")

#ifdef QGC_GST_STREAMING
#include "GStreamer.h"
static constexpr bool kGstEnabled = true;
#else
static constexpr bool kGstEnabled = false;
#endif
#include "UVCReceiver.h"

DECLARE_SETTINGGROUP(Video, "Video")
{
    // Setup enum values for videoSource settings into meta data
    QVariantList videoSourceList;
    videoSourceList.append(videoSourceRTSP);
    videoSourceList.append(videoSourceUDPH264);
    videoSourceList.append(videoSourceUDPH265);
    videoSourceList.append(videoSourceTCP);
    videoSourceList.append(videoSourceMPEGTS);
#ifdef QGC_GST_STREAMING
    videoSourceList.append(videoSourceHTTPMJPEG);
#ifdef QGC_HAS_WEBSOCKET_VIDEO
    videoSourceList.append(videoSourceWebSocketJPEG);
#endif
#endif
    videoSourceList.append(videoSource3DRSolo);
    videoSourceList.append(videoSourceParrotDiscovery);
    videoSourceList.append(videoSourceYuneecMantisG);

#ifdef QGC_HERELINK_AIRUNIT_VIDEO
    videoSourceList.append(videoSourceHerelinkAirUnit);
#else
    videoSourceList.append(videoSourceHerelinkHotspot);
#endif
    QStringList uvcDevices = UVCReceiver::getDeviceNameList();
    for (const QString& device : uvcDevices) {
        videoSourceList.append(device);
    }
    if (videoSourceList.count() == 0) {
        _noVideo = true;
        videoSourceList.append(videoSourceNoVideo);
        setUserVisible(false);
    } else {
        videoSourceList.insert(0, videoDisabled);
    }

    // make translated strings
    QStringList videoSourceCookedList;
    for (const QVariant& videoSource: videoSourceList) {
        videoSourceCookedList.append( VideoSettings::tr(videoSource.toString().toStdString().c_str()) );
    }

    _nameToMetaDataMap[videoSourceName]->setEnumInfo(videoSourceCookedList, videoSourceList);

    _setForceVideoDecodeList();

    // Migrate legacy gpuZeroCopyEnabled (pre-rename) into the new force-CPU semantics.
    {
        QSettings settings;
        settings.beginGroup(settingsGroup);
        const bool hasLegacy = settings.contains(QStringLiteral("gpuZeroCopyEnabled"));
        const bool hasNew    = settings.contains(forceCpuVideoPathName);
        if (hasLegacy) {
            if (!hasNew) {
                const bool gpuZeroCopy = settings.value(QStringLiteral("gpuZeroCopyEnabled")).toBool();
                forceCpuVideoPath()->setRawValue(!gpuZeroCopy);
            }
            settings.remove(QStringLiteral("gpuZeroCopyEnabled"));
        }
        settings.endGroup();
    }

    // Set default value for videoSource
    _setDefaults();
}

VideoSettings::~VideoSettings()
{
    _networkVideoSecret.detach();
    QGC::secureZero(_networkVideoSecret);
}

void VideoSettings::_setDefaults()
{
    if (_noVideo) {
        _nameToMetaDataMap[videoSourceName]->setRawDefaultValue(videoSourceNoVideo);
    } else {
        _nameToMetaDataMap[videoSourceName]->setRawDefaultValue(videoDisabled);
    }
}

DECLARE_SETTINGSFACT(VideoSettings, aspectRatio)
DECLARE_SETTINGSFACT(VideoSettings, videoFit)
DECLARE_SETTINGSFACT(VideoSettings, gridLines)
DECLARE_SETTINGSFACT(VideoSettings, showRecControl)
DECLARE_SETTINGSFACT(VideoSettings, recordingFormat)
DECLARE_SETTINGSFACT(VideoSettings, maxVideoSize)
DECLARE_SETTINGSFACT(VideoSettings, enableStorageLimit)
DECLARE_SETTINGSFACT(VideoSettings, streamEnabled)
DECLARE_SETTINGSFACT(VideoSettings, disableWhenDisarmed)

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, videoSource)
{
    if (!_videoSourceFact) {
        _videoSourceFact = _createSettingsFact(videoSourceName);
        //-- Check for sources no longer available
        if(!_videoSourceFact->enumValues().contains(_videoSourceFact->rawValue().toString())) {
            if (_noVideo) {
                _videoSourceFact->setRawValue(videoSourceNoVideo);
            } else {
                _videoSourceFact->setRawValue(videoDisabled);
            }
        }
        connect(_videoSourceFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _videoSourceFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, forceVideoDecoder)
{
    if (!_forceVideoDecoderFact) {
        _forceVideoDecoderFact = _createSettingsFact(forceVideoDecoderName);

        _forceVideoDecoderFact->setUserVisible(kGstEnabled);

        connect(_forceVideoDecoderFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _forceVideoDecoderFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, lowLatencyMode)
{
    if (!_lowLatencyModeFact) {
        _lowLatencyModeFact = _createSettingsFact(lowLatencyModeName);

        _lowLatencyModeFact->setUserVisible(kGstEnabled);

        connect(_lowLatencyModeFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _lowLatencyModeFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, rtpJitterLatencyMs)
{
    if (!_rtpJitterLatencyMsFact) {
        _rtpJitterLatencyMsFact = _createSettingsFact(rtpJitterLatencyMsName);
        _rtpJitterLatencyMsFact->setUserVisible(kGstEnabled);
        connect(_rtpJitterLatencyMsFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _rtpJitterLatencyMsFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, rtspAutoReconnect)
{
    if (!_rtspAutoReconnectFact) {
        _rtspAutoReconnectFact = _createSettingsFact(rtspAutoReconnectName);
        _rtspAutoReconnectFact->setUserVisible(kGstEnabled);
        connect(_rtspAutoReconnectFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _rtspAutoReconnectFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, forceCpuVideoPath)
{
    if (!_forceCpuVideoPathFact) {
        _forceCpuVideoPathFact = _createSettingsFact(forceCpuVideoPathName);

#if defined(QGC_HAS_ANY_GPU_PATH)
        _forceCpuVideoPathFact->setUserVisible(kGstEnabled);
#else
        _forceCpuVideoPathFact->setUserVisible(false);
#endif
    }
    return _forceCpuVideoPathFact;
}

// videoConversionElement / disablePixelAspectRatio are read by VideoBackend::createSink()
// into a VideoSinkConfig and passed as construct-only bin properties — no env-var indirection.
DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, videoConversionElement)
{
    if (!_videoConversionElementFact) {
        _videoConversionElementFact = _createSettingsFact(videoConversionElementName);
        _videoConversionElementFact->setUserVisible(kGstEnabled);
    }
    return _videoConversionElementFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, disablePixelAspectRatio)
{
    if (!_disablePixelAspectRatioFact) {
        _disablePixelAspectRatioFact = _createSettingsFact(disablePixelAspectRatioName);
        _disablePixelAspectRatioFact->setUserVisible(kGstEnabled);
    }
    return _disablePixelAspectRatioFact;
}


DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, rtspTimeout)
{
    if (!_rtspTimeoutFact) {
        _rtspTimeoutFact = _createSettingsFact(rtspTimeoutName);

        _rtspTimeoutFact->setUserVisible(kGstEnabled);

        connect(_rtspTimeoutFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _rtspTimeoutFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, udpUrl)
{
    if (!_udpUrlFact) {
        _udpUrlFact = _createSettingsFact(udpUrlName);
        connect(_udpUrlFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _udpUrlFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, rtspUrl)
{
    if (!_rtspUrlFact) {
        _rtspUrlFact = _createSettingsFact(rtspUrlName);
        connect(_rtspUrlFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _rtspUrlFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, tcpUrl)
{
    if (!_tcpUrlFact) {
        _tcpUrlFact = _createSettingsFact(tcpUrlName);
        connect(_tcpUrlFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _tcpUrlFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, httpMjpegUrl)
{
    if (!_httpMjpegUrlFact) {
        _httpMjpegUrlFact = _createSettingsFact(httpMjpegUrlName);
        connect(_httpMjpegUrlFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _httpMjpegUrlFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, websocketJpegUrl)
{
    if (!_websocketJpegUrlFact) {
        _websocketJpegUrlFact = _createSettingsFact(websocketJpegUrlName);
        connect(_websocketJpegUrlFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _websocketJpegUrlFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, networkVideoAuthType)
{
    if (!_networkVideoAuthTypeFact) {
        _networkVideoAuthTypeFact = _createSettingsFact(networkVideoAuthTypeName);
        connect(_networkVideoAuthTypeFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _networkVideoAuthTypeFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, networkVideoUsername)
{
    if (!_networkVideoUsernameFact) {
        _networkVideoUsernameFact = _createSettingsFact(networkVideoUsernameName);
        connect(_networkVideoUsernameFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _networkVideoUsernameFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, networkVideoSecretFile)
{
    if (!_networkVideoSecretFileFact) {
        _networkVideoSecretFileFact = _createSettingsFact(networkVideoSecretFileName);
        connect(_networkVideoSecretFileFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _networkVideoSecretFileFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, networkVideoOrigin)
{
    if (!_networkVideoOriginFact) {
        _networkVideoOriginFact = _createSettingsFact(networkVideoOriginName);
        connect(_networkVideoOriginFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _networkVideoOriginFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, networkVideoCaCertificateFile)
{
    if (!_networkVideoCaCertificateFileFact) {
        _networkVideoCaCertificateFileFact = _createSettingsFact(networkVideoCaCertificateFileName);
        connect(_networkVideoCaCertificateFileFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _networkVideoCaCertificateFileFact;
}

bool VideoSettings::validateNetworkVideoUrl(const QString& value, const QStringList& allowedSchemes, QString& error)
{
    const QUrl url(value, QUrl::StrictMode);
    if (value.isEmpty() || !url.isValid() || url.host().isEmpty()) {
        error = tr("Enter a valid network video URL with a host.");
        return false;
    }

    const QString scheme = url.scheme().toLower();
    if (!allowedSchemes.contains(scheme)) {
        error = tr("The selected video source does not support the '%1' URL scheme.").arg(scheme);
        return false;
    }
    if (!url.userInfo().isEmpty()) {
        error = tr("Credentials in video URLs are not supported. Use the security settings instead.");
        return false;
    }
    if (url.hasFragment()) {
        error = tr("Video URLs must not contain a fragment.");
        return false;
    }

    static const QStringList sensitiveQueryKeys = {
        QStringLiteral("access_token"),
        QStringLiteral("api_key"),
        QStringLiteral("apikey"),
        QStringLiteral("auth"),
        QStringLiteral("authorization"),
        QStringLiteral("key"),
        QStringLiteral("passwd"),
        QStringLiteral("password"),
        QStringLiteral("secret"),
        QStringLiteral("token"),
    };
    const QUrlQuery query(url);
    for (const auto& [key, unusedValue] : query.queryItems(QUrl::FullyDecoded)) {
        Q_UNUSED(unusedValue)
        if (sensitiveQueryKeys.contains(key.toLower())) {
            error = tr("Credentials in video URL query parameters are not supported.");
            return false;
        }
    }

    error.clear();
    return true;
}

bool VideoSettings::networkVideoSessionSecretConfigured()
{
    return !_networkVideoSecret.isEmpty();
}

bool VideoSettings::networkVideoCredentialFileSupported() const
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    return true;
#else
    return false;
#endif
}

QString VideoSettings::setNetworkVideoSecret(const QString& secret)
{
    QByteArray encoded = secret.toUtf8();
    if (encoded.isEmpty()) {
        return tr("Credential cannot be empty.");
    }
    if (encoded.size() > 4096) {
        QGC::secureZero(encoded);
        return tr("Credential exceeds the 4096 byte limit.");
    }
    if (encoded.contains('\0') || encoded.contains('\r') || encoded.contains('\n')) {
        QGC::secureZero(encoded);
        return tr("Credential must be a single line without NUL characters.");
    }

    _networkVideoSecret.detach();
    QGC::secureZero(_networkVideoSecret);
    _networkVideoSecret = encoded;
    QGC::secureZero(encoded);
    emit networkVideoSecretChanged();
    emit networkVideoConfigurationErrorChanged();
    emit streamConfiguredChanged(streamConfigured());
    return QString();
}

void VideoSettings::clearNetworkVideoSecret()
{
    if (_networkVideoSecret.isEmpty()) {
        return;
    }

    _networkVideoSecret.detach();
    QGC::secureZero(_networkVideoSecret);
    emit networkVideoSecretChanged();
    emit networkVideoConfigurationErrorChanged();
    emit streamConfiguredChanged(streamConfigured());
}

bool VideoSettings::resolveNetworkVideoSecret(QByteArray& secret, QString& error) const
{
    secret.clear();
    error.clear();

    if (!_networkVideoSecret.isEmpty()) {
        secret = _networkVideoSecret;
        return true;
    }

    const QString filePath = _networkVideoSecretFileFact
                                 ? _networkVideoSecretFileFact->rawValue().toString()
                                 : const_cast<VideoSettings*>(this)->networkVideoSecretFile()->rawValue().toString();
    if (filePath.isEmpty()) {
        error = tr("Enter a session credential or select a credential file.");
        return false;
    }

#if !defined(Q_OS_UNIX) || defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    error = tr("Credential files are supported only on Unix-like systems. Enter a session credential instead.");
    return false;
#else
    int openFlags = O_RDONLY | O_NONBLOCK | O_NOFOLLOW;
#ifdef O_CLOEXEC
    openFlags |= O_CLOEXEC;
#endif
    const QByteArray encodedPath = QFile::encodeName(filePath);
    const int fileDescriptor = ::open(encodedPath.constData(), openFlags);
    if (fileDescriptor < 0) {
        error = tr("Credential file could not be opened as a non-symbolic-link file.");
        return false;
    }

    struct stat fileStatus {};
    if (::fstat(fileDescriptor, &fileStatus) != 0 || !S_ISREG(fileStatus.st_mode)) {
        ::close(fileDescriptor);
        error = tr("Credential file must be a regular file.");
        return false;
    }
    if (fileStatus.st_uid != ::geteuid()) {
        ::close(fileDescriptor);
        error = tr("Credential file must be owned by the QGroundControl process user.");
        return false;
    }
    if ((fileStatus.st_mode & S_IRUSR) == 0 || (fileStatus.st_mode & (S_IRWXG | S_IRWXO)) != 0) {
        ::close(fileDescriptor);
        error = tr("Credential file permissions must allow owner read and deny all group/other access.");
        return false;
    }
    if (fileStatus.st_nlink != 1) {
        ::close(fileDescriptor);
        error = tr("Credential file must not have multiple hard links.");
        return false;
    }

    secret.resize(4097);
    qsizetype bytesRead = 0;
    while (bytesRead < secret.size()) {
        const ssize_t result =
            ::read(fileDescriptor, secret.data() + bytesRead, static_cast<size_t>(secret.size() - bytesRead));
        if (result == 0) {
            break;
        }
        if (result < 0) {
            if (errno == EINTR) {
                continue;
            }
            QGC::secureZero(secret);
            ::close(fileDescriptor);
            error = tr("Credential file could not be read.");
            return false;
        }
        bytesRead += static_cast<qsizetype>(result);
    }
    ::close(fileDescriptor);
    secret.resize(bytesRead);
    if (secret.size() > 4096) {
        QGC::secureZero(secret);
        error = tr("Credential file exceeds the 4096 byte limit.");
        return false;
    }
    if (secret.endsWith("\r\n")) {
        secret.chop(2);
    } else if (secret.endsWith('\n') || secret.endsWith('\r')) {
        secret.chop(1);
    }
    if (secret.isEmpty() || secret.contains('\0') || secret.contains('\r') || secret.contains('\n')) {
        QGC::secureZero(secret);
        error = tr("Credential file must contain exactly one non-empty line.");
        return false;
    }

    return true;
#endif
}

QString VideoSettings::networkVideoConfigurationError()
{
    const QString source = videoSource()->rawValue().toString();
    QString urlValue;
    QStringList schemes;
    if (source == videoSourceHTTPMJPEG) {
        urlValue = httpMjpegUrl()->rawValue().toString();
        schemes = {QStringLiteral("http"), QStringLiteral("https")};
    } else if (source == videoSourceWebSocketJPEG) {
        urlValue = websocketJpegUrl()->rawValue().toString();
        schemes = {QStringLiteral("ws"), QStringLiteral("wss")};
    } else {
        return QString();
    }

    QString error;
    if (!validateNetworkVideoUrl(urlValue, schemes, error)) {
        return error;
    }

    const int authType = networkVideoAuthType()->rawValue().toInt();
    if (authType < NetworkVideoAuthNone || authType > NetworkVideoAuthBearer) {
        return tr("Unsupported network video authentication method.");
    }

    const QUrl url(urlValue, QUrl::StrictMode);
    const bool secureTransport = (url.scheme() == QStringLiteral("https") || url.scheme() == QStringLiteral("wss"));
    if (authType != NetworkVideoAuthNone && !secureTransport) {
        return tr("Credentials require HTTPS or WSS. Anonymous HTTP or WS remains available for local testing.");
    }
    if (authType == NetworkVideoAuthBasic) {
        const QString username = networkVideoUsername()->rawValue().toString();
        if (username.isEmpty()) {
            return tr("Basic authentication requires a username.");
        }
        if (username.contains(QLatin1Char(':')) || username.contains(QChar::Null) ||
            username.contains(QLatin1Char('\r')) || username.contains(QLatin1Char('\n'))) {
            return tr("Basic authentication username must not contain colon, NUL, CR, or LF characters.");
        }
    }

    const QString origin = networkVideoOrigin()->rawValue().toString();
    if (!origin.isEmpty()) {
        const QUrl originUrl(origin, QUrl::StrictMode);
        const QString originScheme = originUrl.scheme().toLower();
        if (!originUrl.isValid() || originUrl.host().isEmpty() ||
            (originScheme != QStringLiteral("http") && originScheme != QStringLiteral("https")) ||
            !originUrl.userInfo().isEmpty() || originUrl.hasQuery() || originUrl.hasFragment() ||
            (!originUrl.path().isEmpty() && originUrl.path() != QStringLiteral("/"))) {
            return tr("Origin must be an HTTP or HTTPS origin containing only scheme, host, and optional port.");
        }
    }
    if (authType != NetworkVideoAuthNone) {
        QByteArray resolvedSecret;
        if (!resolveNetworkVideoSecret(resolvedSecret, error)) {
            return error;
        }
        QGC::secureZero(resolvedSecret);
    }

    const QString caFile = networkVideoCaCertificateFile()->rawValue().toString();
    if (!caFile.isEmpty()) {
        if (!secureTransport) {
            return tr("A custom CA certificate can be used only with HTTPS or WSS.");
        }
        if (QGCNetworkHelper::loadCaCertificates(caFile, &error).isEmpty()) {
            return error;
        }
    }

    return QString();
}

bool VideoSettings::streamConfigured(void)
{
    //-- First, check if it's autoconfigured
    if(VideoManager::instance()->autoStreamConfigured()) {
        qCDebug(VideoSettingsLog) << "Stream auto configured";
        return true;
    }
    //-- Check if it's disabled
    QString vSource = videoSource()->rawValue().toString();
    if(vSource == videoSourceNoVideo || vSource == videoDisabled) {
        return false;
    }
    //-- If UDP, check for URL
    if(vSource == videoSourceUDPH264 || vSource == videoSourceUDPH265) {
        qCDebug(VideoSettingsLog) << "Testing configuration for UDP Stream:" << udpUrl()->rawValue().toString();
        return !udpUrl()->rawValue().toString().isEmpty();
    }
    //-- If RTSP, check for URL
    if(vSource == videoSourceRTSP) {
        qCDebug(VideoSettingsLog) << "Testing configuration for RTSP Stream:"
                                  << QGCNetworkHelper::redactedUrlForLogging(rtspUrl()->rawValue().toString());
        return !rtspUrl()->rawValue().toString().isEmpty();
    }
    //-- If TCP, check for URL
    if(vSource == videoSourceTCP) {
        qCDebug(VideoSettingsLog) << "Testing configuration for TCP Stream:" << tcpUrl()->rawValue().toString();
        return !tcpUrl()->rawValue().toString().isEmpty();
    }
    //-- If MPEG-TS, check for URL
    if(vSource == videoSourceMPEGTS) {
        qCDebug(VideoSettingsLog) << "Testing configuration for MPEG-TS Stream:" << udpUrl()->rawValue().toString();
        return !udpUrl()->rawValue().toString().isEmpty();
    }
    if (vSource == videoSourceHTTPMJPEG || vSource == videoSourceWebSocketJPEG) {
        const QString error = networkVideoConfigurationError();
        if (!error.isEmpty()) {
            qCDebug(VideoSettingsLog) << "Network video configuration is incomplete:" << error;
            return false;
        }
        return true;
    }
    //-- If Herelink Air unit, good to go
    if(vSource == videoSourceHerelinkAirUnit) {
        qCDebug(VideoSettingsLog) << "Stream configured for Herelink Air Unit";
        return true;
    }
    //-- If Herelink Hotspot, good to go
    if(vSource == videoSourceHerelinkHotspot) {
        qCDebug(VideoSettingsLog) << "Stream configured for Herelink Hotspot";
        return true;
    }
    if (UVCReceiver::enabled() && UVCReceiver::deviceExists(vSource)) {
        qCDebug(VideoSettingsLog) << "Stream configured for UVC";
        return true;
    }
    return false;
}

void VideoSettings::_configChanged(QVariant)
{
    emit networkVideoConfigurationErrorChanged();
    emit streamConfiguredChanged(streamConfigured());
}

void VideoSettings::_setForceVideoDecodeList()
{
#ifdef QGC_GST_STREAMING
    static const QList<GStreamer::VideoDecoderOptions> removeForceVideoDecodeList{
#if defined(Q_OS_ANDROID)
    GStreamer::VideoDecoderOptions::ForceVideoDecoderDirectX3D,
    GStreamer::VideoDecoderOptions::ForceVideoDecoderVideoToolbox,
    GStreamer::VideoDecoderOptions::ForceVideoDecoderVAAPI,
    GStreamer::VideoDecoderOptions::ForceVideoDecoderNVIDIA,
    GStreamer::VideoDecoderOptions::ForceVideoDecoderIntel,
#elif defined(Q_OS_LINUX)
    GStreamer::VideoDecoderOptions::ForceVideoDecoderDirectX3D,
    GStreamer::VideoDecoderOptions::ForceVideoDecoderVideoToolbox,
#elif defined(Q_OS_WIN)
    GStreamer::VideoDecoderOptions::ForceVideoDecoderVideoToolbox,
    GStreamer::VideoDecoderOptions::ForceVideoDecoderVulkan,
#elif defined(Q_OS_MACOS)
    GStreamer::VideoDecoderOptions::ForceVideoDecoderDirectX3D,
    GStreamer::VideoDecoderOptions::ForceVideoDecoderVAAPI,
#elif defined(Q_OS_IOS)
    GStreamer::VideoDecoderOptions::ForceVideoDecoderDirectX3D,
    GStreamer::VideoDecoderOptions::ForceVideoDecoderVAAPI,
    GStreamer::VideoDecoderOptions::ForceVideoDecoderNVIDIA,
    GStreamer::VideoDecoderOptions::ForceVideoDecoderIntel,
#endif
    };

    for (const auto &value : removeForceVideoDecodeList) {
        _nameToMetaDataMap[forceVideoDecoderName]->removeEnumInfo(value);
    }
#endif
}

void VideoSettings::pruneUnavailableDecoders()
{
#ifdef QGC_GST_STREAMING
    static const QList<GStreamer::VideoDecoderOptions> hardwareFamilies{
        GStreamer::VideoDecoderOptions::ForceVideoDecoderNVIDIA,
        GStreamer::VideoDecoderOptions::ForceVideoDecoderVAAPI,
        GStreamer::VideoDecoderOptions::ForceVideoDecoderDirectX3D,
        GStreamer::VideoDecoderOptions::ForceVideoDecoderVideoToolbox,
        GStreamer::VideoDecoderOptions::ForceVideoDecoderIntel,
        GStreamer::VideoDecoderOptions::ForceVideoDecoderVulkan,
    };

    const QList<GStreamer::VideoDecoderOptions> available = GStreamer::availableDecoderFamilies();
    const auto metaIt = _nameToMetaDataMap.constFind(forceVideoDecoderName);
    if (metaIt == _nameToMetaDataMap.constEnd() || !metaIt.value()) {
        return;
    }
    FactMetaData* const metaData = metaIt.value();
    bool pruned = false;
    for (const auto family : hardwareFamilies) {
        // removeEnumInfo() qWarns on an absent value, so skip families not in the enum; values are
        // stored as QVariant(int), so match that representation.
        const QVariant familyValue = static_cast<int>(family);
        if (!available.contains(family) && metaData->enumValues().contains(familyValue)) {
            metaData->removeEnumInfo(familyValue);
            pruned = true;
        }
    }

    Fact* const fact = forceVideoDecoder();
    if (pruned) {
        // Backend init is async — refresh any live FactComboBox bound to this fact.
        emit fact->enumsChanged();
    }
    if (!metaData->enumValues().contains(fact->rawValue())) {
        fact->setRawValue(GStreamer::VideoDecoderOptions::ForceVideoDecoderDefault);
    }
#endif
}
