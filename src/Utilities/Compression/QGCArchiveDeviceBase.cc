#include "QGCArchiveDeviceBase.h"

#include <archive.h>
#include <cstring>

#include <QtCore/QFile>

#include "QGCFileHelper.h"
#include "QGCLoggingCategory.h"
#include "QGClibarchive.h"

QGC_LOGGING_CATEGORY(QGCArchiveDeviceBaseLog, "Utilities.QGCArchiveDeviceBase")

// ============================================================================
// Constructors / Destructor
// ============================================================================

QGCArchiveDeviceBase::QGCArchiveDeviceBase(const QString &filePath, QObject *parent)
    : QIODevice(parent)
    , _filePath(filePath)
{
}

QGCArchiveDeviceBase::QGCArchiveDeviceBase(QIODevice *source, QObject *parent)
    : QIODevice(parent)
    , _sourceDevice(source)
{
}

QGCArchiveDeviceBase::~QGCArchiveDeviceBase()
{
    close();
}

// ============================================================================
// QIODevice Interface
// ============================================================================

bool QGCArchiveDeviceBase::open(OpenMode mode)
{
    // A second opening starts as the first: without the buffer, the end and the reader of the one before
    if (isOpen()) {
        close();
    }
    _errorString.clear();

    if (mode != ReadOnly) {
        setError(QLatin1StringView(metaObject()->className()) + QStringLiteral(" only supports ReadOnly mode"));
        return false;
    }

    if (!initArchive() || !prepareForReading()) {
        return false;
    }

    return QIODevice::open(mode);
}

void QGCArchiveDeviceBase::close()
{
    _reader.reset();

    resetState();

    if (_ownedSource) {
        _ownedSource.reset();
        _sourceDevice = nullptr;
    }

    QIODevice::close();
}

qint64 QGCArchiveDeviceBase::bytesAvailable() const
{
    return _buffer.size() + QIODevice::bytesAvailable();
}

bool QGCArchiveDeviceBase::atEnd() const
{
    if (!isOpen()) {
        return true;
    }
    if (bytesAvailable() > 0) {
        return false;
    }

    // QIODevice goes by what is buffered, which says nothing about what is still compressed
    const_cast<QGCArchiveDeviceBase*>(this)->fillBuffer();
    return _buffer.isEmpty();
}

QString QGCArchiveDeviceBase::errorString() const
{
    return _errorString;
}

QString QGCArchiveDeviceBase::formatName() const
{
    return _reader ? _reader->formatName() : QString();
}

QString QGCArchiveDeviceBase::filterName() const
{
    return _reader ? _reader->filterName() : QString();
}

// ============================================================================
// Protected QIODevice Methods
// ============================================================================

qint64 QGCArchiveDeviceBase::readData(char *data, qint64 maxSize)
{
    if (_eof && _buffer.isEmpty()) {
        return 0;  // EOF
    }

    // Fill buffer if needed
    while (_buffer.size() < maxSize && !_eof) {
        if (!fillBuffer()) {
            if (_buffer.isEmpty()) {
                return _eof ? 0 : -1;
            }
            break;
        }
    }

    // Return data from buffer
    const qint64 bytesToCopy = qMin(static_cast<qint64>(_buffer.size()), maxSize);
    if (bytesToCopy > 0) {
        std::memcpy(data, _buffer.constData(), static_cast<size_t>(bytesToCopy));
        _buffer.remove(0, static_cast<int>(bytesToCopy));
    }

    return bytesToCopy;
}

qint64 QGCArchiveDeviceBase::writeData(const char *, qint64)
{
    return -1;
}

// ============================================================================
// Protected Implementation Methods
// ============================================================================

bool QGCArchiveDeviceBase::initSourceFromPath()
{
    // QFile reads a Qt resource as it reads a file
    auto file = std::make_unique<QFile>(_filePath);
    if (!file->open(QIODevice::ReadOnly)) {
        const bool isResource = QGCFileHelper::isQtResource(_filePath);
        setError((isResource ? QStringLiteral("Failed to open resource: ") : QStringLiteral("Failed to open file: ")) +
                 _filePath);
        return false;
    }
    _ownedSource = std::move(file);
    _sourceDevice = _ownedSource.get();

    return true;
}

bool QGCArchiveDeviceBase::openArchive(bool allFormats)
{
    if (!_filePath.isEmpty() && !initSourceFromPath()) {
        return false;
    }

    if (!_sourceDevice || !_sourceDevice->isOpen() || !_sourceDevice->isReadable()) {
        setError(QStringLiteral("Source device is not ready"));
        return false;
    }

    _reader = std::make_unique<QGClibarchive::ArchiveReader>();
    const QGClibarchive::ReaderMode mode =
        allFormats ? QGClibarchive::ReaderMode::AllFormats : QGClibarchive::ReaderMode::RawFormat;
    QString reason;
    if (!_reader->open(_sourceDevice, mode, reason)) {
        setError(reason);
        _reader.reset();
        return false;
    }

    return true;
}

bool QGCArchiveDeviceBase::fillBuffer()
{
    if (_eof || !_reader || !_reader->isOpen()) {
        return false;
    }

    char readBuffer[QGCFileHelper::kBufferSizeMax];
    const la_ssize_t bytesRead = archive_read_data(_reader->handle(), readBuffer, sizeof(readBuffer));

    if (bytesRead < 0) {
        setError(QString::fromUtf8(archive_error_string(_reader->handle())));
        qCWarning(QGCArchiveDeviceBaseLog) << "Read error:" << _errorString;
        // libarchive has no more data to give, and would report the error again for every read
        _reader->close();
        return false;
    }

    if (bytesRead == 0) {
        _eof = true;
        return false;
    }

    _buffer.append(readBuffer, static_cast<int>(bytesRead));
    return true;
}

void QGCArchiveDeviceBase::setError(const QString& reason)
{
    // libarchive has no text for some failures, and an empty one would say that nothing failed
    _errorString = reason.isEmpty() ? QStringLiteral("Unknown error") : reason;
    setErrorString(_errorString);
}

void QGCArchiveDeviceBase::resetState()
{
    _buffer.clear();
    _eof = false;
}
