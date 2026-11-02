#include "RecordingFinalizer.h"

#include <gst/gst.h>
#include <utility>

void RecordingFinalizer::arm(GstElement* fileSink)
{
    _fileSink = GStreamer::adoptElement(GST_ELEMENT(gst_object_ref(fileSink)));
    _outcome = Outcome::Pending;
}

RecordingFinalizer::Outcome RecordingFinalizer::disarm()
{
    _fileSink.reset();
    return std::exchange(_outcome, Outcome::Pending);
}

bool RecordingFinalizer::settle(Outcome outcome)
{
    if (!_fileSink || (_outcome != Outcome::Pending)) {
        return false;
    }
    _outcome = outcome;
    return true;
}

bool RecordingFinalizer::settleFrom(GstObject* source, Outcome outcome, bool* belongs)
{
    const bool fromRecording =
        _fileSink && source && gst_object_has_as_ancestor(source, GST_OBJECT_CAST(_fileSink.get()));
    if (belongs) {
        *belongs = fromRecording;
    }
    return fromRecording && settle(outcome);
}
