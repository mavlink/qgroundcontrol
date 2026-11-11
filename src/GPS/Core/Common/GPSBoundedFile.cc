#include "GPSBoundedFile.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QLoggingCategory>

GPSBoundedFile::GPSBoundedFile(Category category, const char* subject, qint64 maxBytes)
    : _category(category)
    , _subject(subject)
    , _maxBytes(maxBytes)
{}

GPSBoundedFile::~GPSBoundedFile()
{
    close();
}

bool GPSBoundedFile::open(const QString& file)
{
    close();
    _file.setFileName(file);
    _bytes = 0;
    const QString directory = QFileInfo(_file).absolutePath();
    if (!QDir().mkpath(directory)) {
        _fail(QStringLiteral("Cannot create the folder %1").arg(directory));
    } else if (!_file.open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
        _fail(_file.errorString());
    }
    return isOpen();
}

bool GPSBoundedFile::write(const QByteArray& record)
{
    if (!isOpen()) {
        return false;
    }
    // Whole records only, so a file that reached its limit still ends with a complete record.
    if (_bytes + record.size() > _maxBytes) {
        qCWarning(_category) << _subject << "reached its size limit of" << _maxBytes
                             << "bytes and stopped:" << _file.fileName();
        close();
    } else if (_file.write(record) != record.size()) {
        _fail(_file.errorString());
    } else {
        _bytes += record.size();
    }
    return isOpen();
}

bool GPSBoundedFile::flush()
{
    if (isOpen() && !_file.flush()) {
        _fail(_file.errorString());
    }
    return isOpen();
}

void GPSBoundedFile::close()
{
    if (flush()) {
        _file.close();
    }
}

void GPSBoundedFile::_fail(const QString& error)
{
    qCWarning(_category) << _subject << "stopped after a file error:" << _file.fileName() << error;
    _file.close();
}
