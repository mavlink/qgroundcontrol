#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QString>

class QLoggingCategory;

/// The start times of one writer's files, which are named by the millisecond they start in. No two are the same, so a
/// file that starts within the millisecond of the one before it gets a name of its own instead of being refused as
/// that earlier file, and the names sort in the order the files started.
class GPSFileStartTimes
{
public:
    /// The start time of the next file: @a now, or the millisecond after the previous one while @a now is not later.
    [[nodiscard]] QDateTime next(const QDateTime& now)
    {
        _last = _last.isValid() && now <= _last ? _last.addMSecs(1) : now;
        return _last;
    }

private:
    QDateTime _last;
};

/// A file that records are appended to, such as a receiver recording or a correction log.
/// The file is written through QFile's buffer, which flush() and close() write out. It keeps the records that fit in
/// maxBytes and then ends, which bounds disk use without a retention policy.
/// Reaching that limit or a file error logs one warning and closes the file; the caller sees it in the result of the
/// call and carries on without it.
class GPSBoundedFile
{
public:
    /// A logging category, as Q_LOGGING_CATEGORY defines one.
    using Category = const QLoggingCategory& (*)();

    /// What ended the file on its own, or kept it from starting.
    enum class End
    {
        /// Nothing did: the file is open, was closed by close(), or was never started.
        None,
        /// The next record did not fit in maxBytes.
        SizeLimit,
        /// The folder of the file could not be created.
        FolderError,
        /// The file could not be created, written or flushed; errorString() has the system's reason.
        FileError,
    };

    /// The warnings are logged in @a category and begin with @a subject, a literal such as "Correction log".
    GPSBoundedFile(Category category, const char* subject, qint64 maxBytes);
    ~GPSBoundedFile();
    Q_DISABLE_COPY_MOVE(GPSBoundedFile)

    /// Starts @a file, creating its folder, in place of the file that was open. A file that exists is an earlier one
    /// and is kept, so the file does not start: a writer names its files with GPSFileStartTimes to keep them apart.
    /// Returns whether it is open.
    bool open(const QString& file);
    /// Appends @a record, whole or not at all. Returns whether the file is still open.
    bool write(const QByteArray& record);
    /// Writes the buffered records out. Returns whether the file is still open.
    bool flush();
    void close();

    [[nodiscard]] bool isOpen() const { return _file.isOpen(); }

    /// The path of the open file; empty while none is open.
    [[nodiscard]] QString file() const { return isOpen() ? _file.fileName() : QString(); }

    /// Why the file is not open since open() was last called, for a caller that tells the user what the warning says.
    [[nodiscard]] End end() const { return _end; }

    /// The system's reason for End::FileError, such as "No space left on device"; empty otherwise.
    [[nodiscard]] QString errorString() const { return _errorString; }

private:
    /// Closes the file after the error @a end, which the warning describes as @a error.
    void _fail(End end, const QString& error);

    const Category _category;
    const char* const _subject;
    const qint64 _maxBytes;
    QFile _file;
    qint64 _bytes = 0;
    End _end = End::None;
    QString _errorString;
};
