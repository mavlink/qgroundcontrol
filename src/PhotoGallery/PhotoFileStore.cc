/****************************************************************************
 *
 *   (c) 2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "PhotoFileStore.h"

#include <QDateTime>
#include <QDir>
#include <QDebug>
#include <QtQml>

#include "ExtractJPEGMetadata.h"

PhotoFileStore::PhotoFileStore(QObject * parent)
    : PhotoFileStoreInterface(parent)
{
}

PhotoFileStore::PhotoFileStore(QString location, QObject * parent)
    : PhotoFileStore(parent)
{
    setLocation(std::move(location));
}

PhotoFileStore::~PhotoFileStore()
{
}

void
PhotoFileStore::setLocation(QString location)
{
    if (_location != location) {
        _location = std::move(location);
        rescan();
    }
}
const QString & PhotoFileStore::location() const
{
    return _location;
}

void
PhotoFileStore::setVideoLocation(QString location)
{
    if (_videoLocation != location) {
        _videoLocation = std::move(location);
        rescan();
    }
}

const QString & PhotoFileStore::videoLocation() const
{
    return _videoLocation;
}

const std::set<QString> & PhotoFileStore::ids() const
{
    return _photo_ids;
}

QString PhotoFileStore::add(QString name_hint, QByteArray data)
{
    if (_location.isEmpty()) {
        return {};
    }

    auto metadata = extractJPEGMetadata(data);
    QString base = "";
    auto maybe_flight_uid = metadata.find("PX4:flight_uid");
    auto maybe_vehicle_uuid = metadata.find("PX4:vehicle_uuid");
    if (maybe_flight_uid != metadata.end() && maybe_vehicle_uuid != metadata.end()) {
        base = maybe_vehicle_uuid->second + QDir::separator() + maybe_flight_uid->second;
        QDir(_location).mkpath(base);
        base = base + QDir::separator();
    }

    QString id = base + name_hint;

    while (_photo_ids.find(id) != _photo_ids.end()) {
        QDateTime now = QDateTime::currentDateTime();
        QString suffix = QFileInfo(name_hint).suffix();
        id = base + now.toString("yyyy-MM-dd-HH-mm-ss.z");
        if (!suffix.isEmpty()) {
            id += "." + suffix;
        }
    }
    QFile file(QDir(_location).filePath(id));
    if (!file.open(QIODevice::ReadWrite | QIODevice::NewOnly)) {
        return {};
    }
    if (file.write(data) != data.size()) {
        file.remove();
        return {};
    }
    file.close();

    _photo_ids.insert(id);
    emit added({id});

    return id;
}

void PhotoFileStore::remove(const std::set<QString> & ids)
{
    if (_location.isEmpty()) {
        return;
    }

    QDir base(_location);

    for (const auto & id : ids) {
        std::size_t count = _photo_ids.erase(id);
        if (count == 0) {
            // Tried to erase an image that does not exist. Let's not pass this
            // operation down to the filesystem.
            continue;
        }
        QFile file(base.filePath(id));
        file.remove();

        // Try to remove directories that this file was in.
        QString path = id;
        int sep = path.lastIndexOf('/');
        if (sep == -1) {
            break;
        }
        path.remove(sep, path.size() - sep);
        base.rmpath(path);
    }
    emit removed(ids);
}

QVariant PhotoFileStore::read(const QString & id) const
{
    QString location;
    {
        QMutexLocker guard(&_mutex);
        location = _location;
    }

    if (location.isEmpty()) {
        return {};
    }
    QFile file(QDir(location).filePath(id));
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    // XXX: reading file of arbitrary size into memory -- what could go wrong?
    return file.readAll();
}

void PhotoFileStore::rescan()
{
    emit removed(_photo_ids);
    _photo_ids.clear();
    QDir base(_location);
    QDirIterator iterator(_location, QStringList("*"),  QDir::Files, QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        iterator.next();
        QFileInfo info = iterator.fileInfo();
        QString relpath = base.relativeFilePath(info.absoluteFilePath());
        _photo_ids.insert(relpath);
    }
    emit added(_photo_ids);
}

namespace {

void registerPhotoFileStoreMetaType()
{
    qmlRegisterType<PhotoFileStore>("QGroundControl.Controllers", 1, 0, "PhotoFileStore");
}

}  // namespace

Q_COREAPP_STARTUP_FUNCTION(registerPhotoFileStoreMetaType);
