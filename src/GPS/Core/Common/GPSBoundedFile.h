#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QFile>
#include <QtCore/QString>

class QLoggingCategory;

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
    /// and is kept, so the file does not start. Returns whether it is open.
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
