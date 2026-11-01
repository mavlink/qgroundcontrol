#include "RecordingPlan.h"

#include <algorithm>

#include <QtCore/QFileInfo>

RecordingPlan::RecordingPlan(const QString& file, VideoReceiver::FileFormat format)
    : _file(file)
    , _format(format)
{}

QString RecordingPlan::partFile(const QString& file, int part)
{
    if (part <= 1) {
        return file;
    }
    const QString suffix = QStringLiteral("_part%1").arg(part);
    const qsizetype dot = file.lastIndexOf(u'.');
    const qsizetype separator = std::max(file.lastIndexOf(u'/'), file.lastIndexOf(u'\\'));
    return (dot > separator) ? (file.left(dot) + suffix + file.mid(dot)) : (file + suffix);
}

bool RecordingPlan::onFileOpened(const QString& file, Clock::time_point now)
{
    if (file != currentFile()) {
        return false;
    }
    _opened = true;
    if (!_startedAt) {
        _startedAt = now;
    }
    return true;
}

void RecordingPlan::onSessionEnded()
{
    if (_opened) {
        ++_part;
        _opened = false;
    }
}

qint64 RecordingPlan::recordingMs(Clock::time_point now) const
{
    return _startedAt ? std::chrono::duration_cast<std::chrono::milliseconds>(now - *_startedAt).count() : -1;
}

qint64 RecordingPlan::bytesWritten() const
{
    qint64 bytes = 0;
    for (int part = 1; part <= _part; ++part) {
        bytes += QFileInfo(partFile(_file, part)).size();
    }
    return bytes;
}
