#pragma once

#include "VideoBackend.h"

/// VideoBackend on GStreamer: the only place the app reaches GStreamer's lifecycle from.
class GStreamerVideoBackend final : public VideoBackend
{
public:
    bool hasStreaming() const override { return true; }

    bool hasGpuVideoPath() const override;

    bool needsAsyncInit() const override { return true; }

    VideoReceiver* createReceiver(QObject* parent) override;
    EnvPrepResult prepareEnvironment() override;
    bool initialize(const QStringList& arguments, const EnvPrepResult& envResult,
                    std::optional<int> debugLevel) override;
    bool selfTest() override;
    void applyDecoderPriorities(int rawOption) override;
    QList<VideoDecoder::Options> availableDecoderFamilies() override;
    void setDebugLevel(int level) override;
    void onMainWindowReady(QQuickWindow* window) override;
    void setAndroidJniState(void* javaVm, void* appContext, void* classLoader) override;
};
