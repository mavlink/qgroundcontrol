#include "RecordingFinalizer.h"

#include <gst/gst.h>
#include <utility>

#include <QtCore/QMutexLocker>

void RecordingFinalizer::arm(GstElement* fileSink)
{
    QMutexLocker lock(&_mutex);
    _fileSink = GStreamer::adoptElement(GST_ELEMENT(gst_object_ref(fileSink)));
    _outcome = Outcome::Pending;
}

RecordingFinalizer::Outcome RecordingFinalizer::disarm()
{
    QMutexLocker lock(&_mutex);
    _fileSink.reset();
    return std::exchange(_outcome, Outcome::Pending);
}

RecordingFinalizer::Outcome RecordingFinalizer::outcome() const
{
    QMutexLocker lock(&_mutex);
    return _outcome;
}

bool RecordingFinalizer::settle(Outcome outcome)
{
    QMutexLocker lock(&_mutex);
    return _settleLocked(outcome);
}

bool RecordingFinalizer::settleFrom(GstObject* source, Outcome outcome, bool* belongs)
{
    QMutexLocker lock(&_mutex);
    const bool fromRecording =
        _fileSink && source && gst_object_has_as_ancestor(source, GST_OBJECT_CAST(_fileSink.get()));
    if (belongs) {
        *belongs = fromRecording;
    }
    return fromRecording && _settleLocked(outcome);
}

RecordingFinalizer::Outcome RecordingFinalizer::wait(QDeadlineTimer deadline)
{
    QMutexLocker lock(&_mutex);
    while (_outcome == Outcome::Pending) {
        if (!_settled.wait(&_mutex, deadline)) {
            break;
        }
    }
    return _outcome;
}

bool RecordingFinalizer::_settleLocked(Outcome outcome)
{
    if (!_fileSink || (_outcome != Outcome::Pending)) {
        return false;
    }
    _outcome = outcome;
    _settled.wakeAll();
    return true;
}
