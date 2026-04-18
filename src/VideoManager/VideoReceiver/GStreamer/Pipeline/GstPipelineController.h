#pragma once

#include <glib.h>
#include <gst/gst.h>

#include "GStreamerHelpers.h"
#include "GstObjectPtr.h"
#include "VideoSourceResolver.h"

/// Owns the standard receiver pipeline and the cross-cutting GStreamer plumbing
/// around it: element refs, tee probe lifetime, bus signal wiring, graph dumps,
/// EOS drain, and state transitions.
class GstPipelineController
{
public:
    [[nodiscard]] bool build(const VideoSourceResolver::SourceDescriptor& source, int bufferMs, bool forceSwDecoders);

    [[nodiscard]] bool hasPipeline() const { return static_cast<bool>(_pipeline); }
    [[nodiscard]] GstElement* pipeline() const { return _pipeline.get(); }
    [[nodiscard]] GstElement* source() const { return _source.get(); }
    [[nodiscard]] GstElement* tee() const { return _tee.get(); }
    [[nodiscard]] GstElement* decoderValve() const { return _decoderValve.get(); }
    [[nodiscard]] GstElement* recorderValve() const { return _recorderValve.get(); }

    [[nodiscard]] GstNonFloatingPtr<GstBus> bus() const;
    [[nodiscard]] GstNonFloatingPtr<GstPad> firstSourcePad() const;

    void connectBus(GCallback callback, gpointer data);
    void disconnectBus(gpointer data);

    void installTeeProbe(GstPadProbeCallback callback, gpointer data);
    void removeTeeProbe();

    [[nodiscard]] bool setPlaying();
    void setNull();
    void clear();
    void drainEos(GstBus* bus, gpointer signalData);
    void dumpGraph(const char* name) const;

private:
    GstObjectPtr<GstElement> _decoderValve;
    GstObjectPtr<GstElement> _pipeline;
    GstObjectPtr<GstElement> _recorderValve;
    GstObjectPtr<GstElement> _source;
    GstObjectPtr<GstElement> _tee;
    GstPadProbeGuard _teeProbeGuard;
};
