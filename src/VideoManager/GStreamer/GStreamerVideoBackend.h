#pragma once

#include <atomic>
#include <cstdint>

#include "VideoBackend.h"

/// \brief VideoBackend on GStreamer: the only place the app reaches GStreamer's lifecycle from, in the only order it
/// can run: prepareEnvironment() on the GUI thread, before gst_init() reads the environment; initialize() once, on any
/// thread; then selfTest(), once initialized.
///
/// gst_init() can't be undone, so initialize() runs GStreamer's setup once: a later call returns the first one's
/// outcome, and a failed setup isn't retried. The receivers are then Qt Multimedia's. The free functions in
/// GStreamer.h, GStreamerEnvironment.h and GStreamerSelfTest.h do the work.
class GStreamerVideoBackend final : public VideoBackend
{
public:
    enum class State : uint8_t
    {
        Unprepared,
        Prepared,     ///< The environment is set up for gst_init().
        Initialized,  ///< GStreamer runs, with the plugins video needs.
        Failed,       ///< initialize() failed; Qt Multimedia plays the streams.
    };

    State state() const { return _state.load(std::memory_order_acquire); }

    Capabilities capabilities() const override;

    VideoReceiver* createReceiver(QObject* parent) override;
    void configureQtGraphics(bool softwareRendering) override;
    /// Points GStreamer at its plugins, scanner and registry (GStreamer::Environment::prepareEnvironment()).
    EnvPrepResult prepareEnvironment() override;
    /// GStreamer::initialize() the first time; afterwards that call's outcome.
    bool initialize(const InitParams& params) override;
    QString initFailureMessage() const override;
    /// GStreamer::SelfTest::run() once initialized; false, logged, otherwise.
    bool selfTest() override;
    void applyDecoderPriorities(VideoDecoderOption option) override;
    QList<VideoDecoderOption> availableDecoderFamilies() override;
    void setDebugLevel(int level) override;
    void onMainWindowReady(QQuickWindow* window) override;
    void setAndroidJniState(void* javaVm, void* appContext, void* classLoader) override;

private:
    std::atomic<State> _state{State::Unprepared};
};
