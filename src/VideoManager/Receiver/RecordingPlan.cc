#include "RecordingPlan.h"

#include <algorithm>

#include <QtCore/QFileInfo>

RecordingPlan::RecordingPlan(const QString& file, VideoReceiver::FileFormat format,
                             std::chrono::milliseconds maxPartDuration)
    : _file(file)
    , _format(format)
    , _maxPartDuration(maxPartDuration)
{}

QString RecordingPlan::insertBeforeExtension(const QString& file, const QString& tag)
{
    const qsizetype dot = file.lastIndexOf(u'.');
    const qsizetype separator = std::max(file.lastIndexOf(u'/'), file.lastIndexOf(u'\\'));
    return (dot > separator) ? (file.left(dot) + tag + file.mid(dot)) : (file + tag);
}

QString RecordingPlan::partFile(const QString& file, int part)
{
    return (part <= 1) ? file : insertBeforeExtension(file, QStringLiteral("_part%1").arg(part));
}

void RecordingPlan::setFormat(VideoReceiver::FileFormat format)
{
    nextPart();
    _format = format;
    const qsizetype dot = _file.lastIndexOf(u'.');
    const qsizetype separator = std::max(_file.lastIndexOf(u'/'), _file.lastIndexOf(u'\\'));
    _file = ((dot > separator) ? _file.left(dot) : _file) + u'.' + VideoReceiver::fileExtension(format);
}

bool RecordingPlan::onFileOpened(const QString& file, Clock::time_point now)
{
    if (file != currentFile()) {
        return false;
    }
    _opened = true;
    _openFile = file;
    _partStartedAt = now;
    if (!_startedAt) {
        _startedAt = now;
    }
    return true;
}

void RecordingPlan::onFileClosed(const QString& file)
{
    if (file == _openFile) {
        _closedBytes += QFileInfo(file).size();
        _openFile.clear();
    }
}

bool RecordingPlan::partDue(Clock::time_point now) const
{
    return _opened && ((now - _partStartedAt) >= _maxPartDuration);
}

void RecordingPlan::nextPart()
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
    return _closedBytes + (_openFile.isEmpty() ? 0 : QFileInfo(_openFile).size());
}
