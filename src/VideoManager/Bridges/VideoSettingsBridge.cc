#include "VideoSettingsBridge.h"

#include "Fact.h"
#include "QGCLoggingCategory.h"
#include "VideoSettings.h"

QGC_LOGGING_CATEGORY(VideoSettingsBridgeLog, "Video.VideoSettingsBridge")

VideoSettingsBridge::VideoSettingsBridge(VideoSettings* settings, QObject* parent)
    : QObject(parent),
      _settings(settings)
{
}

VideoSettingsBridge::~VideoSettingsBridge() = default;

void VideoSettingsBridge::subscribe()
{
    if (_subscribed)
        return;
    _subscribed = true;

    // Fact::rawValueChanged carries a QVariant; Qt drops the arg when
    // forwarding to a zero-arg signal.
    (void)connect(_settings, &VideoSettings::streamConfiguredChanged, this,
                  &VideoSettingsBridge::sourceChanged);
    (void)connect(_settings->videoSource(), &Fact::rawValueChanged, this,
                  &VideoSettingsBridge::sourceChanged);
    (void)connect(_settings->udpUrl(), &Fact::rawValueChanged, this,
                  &VideoSettingsBridge::sourceChanged);
    (void)connect(_settings->rtspUrl(), &Fact::rawValueChanged, this,
                  &VideoSettingsBridge::sourceChanged);
    (void)connect(_settings->tcpUrl(), &Fact::rawValueChanged, this,
                  &VideoSettingsBridge::sourceChanged);
    (void)connect(_settings->lowLatencyMode(), &Fact::rawValueChanged, this,
                  &VideoSettingsBridge::lowLatencyChanged);
}
