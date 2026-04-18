#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QString>
#include <QtMultimedia/QMediaFormat>
#include <functional>
#include <gst/gstelement.h>

#include "GstObjectPtr.h"
#include "GstTeeBranch.h"
#include "VideoReceiver.h"

Q_DECLARE_LOGGING_CATEGORY(GstRecordingBranchLog)

/// Manages the recording tee branch (filesink + mux + keyframe probe).
///
/// Not a QObject. Takes pipeline elements as explicit parameters to avoid
/// back-pointers into GstVideoReceiver. Signal emissions are the caller's
/// responsibility.
class GstRecordingBranch : public GstTeeBranch
{
public:
    GstRecordingBranch() = default;

    /// Build and link the filesink branch into the pipeline. Install keyframe probe.
    /// Returns STATUS_OK on success. Does NOT emit signals -- caller does that.
    VideoReceiver::STATUS start(GstElement* pipeline, GstElement* recorderValve, GstElement* tee,
                                const QString& videoFile, QMediaFormat::FileFormat format,
                                std::function<void(const QString&)> onKeyframe);

    /// Close valve and unlink branch. Async -- completion via pending callback.
    /// @param unlinkFn wraps GstVideoReceiver::_unlinkBranch (passed to avoid coupling).
    /// @return STATUS_OK if EOS sent, STATUS_FAIL otherwise.
    VideoReceiver::STATUS stop(GstElement* recorderValve, std::function<bool(GstElement*)> unlinkFn);

    /// Tear down the recording branch (called when EOS arrives or on pipeline stop).
    void shutdown(GstElement* pipeline);

    [[nodiscard]] bool hasFileSink() const { return !!_fileSink; }

    /// Context passed as user_data to the keyframe probe.
    struct KeyframeProbeCtx
    {
        std::function<void(const QString&)> onKeyframe;  ///< emits recordingStarted
        QString recordingOutput;
    };

    /// Keyframe-aligned recording probe -- drops buffers until first keyframe,
    /// sets timestamp offset, then removes itself.
    static GstPadProbeReturn keyframeWatch(GstPad* pad, GstPadProbeInfo* info, gpointer user_data);

private:
    GstElement* makeFileSink(const QString& videoFile, QMediaFormat::FileFormat format, GstElement* tee);
    const char* selectMuxForStream(GstElement* tee, QMediaFormat::FileFormat format) const;

    GstObjectPtr<GstElement> _fileSink;
    /// Guards the keyframe probe; removes it on shutdown so the probe
    /// callback cannot fire into freed state after teardown.
    GstPadProbeGuard _keyframeProbe;
};
