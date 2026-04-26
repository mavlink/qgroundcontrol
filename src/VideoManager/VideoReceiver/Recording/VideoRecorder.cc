#include "VideoRecorder.h"

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(VideoRecorderLog, "Video.VideoRecorder")

VideoRecorder::VideoRecorder(QObject* parent) : QObject(parent) {}

void VideoRecorder::setState(State s)
{
    if (s == _state)
        return;
    _state = s;
    emit stateChanged(s);
}
