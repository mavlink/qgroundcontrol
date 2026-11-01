#pragma once

#include "GStreamerRuntime.h"
#include "VideoBackend.h"

/// VideoBackend on GStreamer: the only place the app reaches GStreamer's lifecycle from.
class GStreamerVideoBackend final : public VideoBackend
{
public:
    Capabilities capabilities() const override;

    VideoReceiver* createReceiver(QObject* parent) override;
    void configureQtGraphics(bool softwareRendering) override;
    EnvPrepResult prepareEnvironment() override;
    bool initialize(const InitParams& params) override;
    bool selfTest() override;
    void applyDecoderPriorities(int rawOption) override;
    QList<VideoDecoder::Options> availableDecoderFamilies() override;
    void setDebugLevel(int level) override;
    void onMainWindowReady(QQuickWindow* window) override;
    void setAndroidJniState(void* javaVm, void* appContext, void* classLoader) override;

private:
    GStreamer::Runtime _runtime;
};
