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

PhotoFileStore::PhotoFileStore(QObject * parent)
    : QObject(parent)
{
}

PhotoFileStore::PhotoFileStore(QString location, QObject * parent)
    : PhotoFileStore(parent)
{
    setLocation(std::move(location));
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

const std::set<QString> & PhotoFileStore::ids() const
{
    return _photo_ids;
}

QString PhotoFileStore::add(QString name_hint, QByteArray data)
{
    if (_location.isEmpty()) {
        return {};
    }
    QString id = name_hint;
    while (_photo_ids.find(id) != _photo_ids.end()) {
        QDateTime now = QDateTime::currentDateTime();
        QString suffix = QFileInfo(name_hint).suffix();
        id = now.toString("yyyy-MM-dd-HH-mm-ss.z");
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
    for (const auto & id : ids) {
        _photo_ids.erase(id);
        QFile file(QDir(_location).filePath(id));
        file.remove();
    }
    emit removed(ids);
}

QVariant PhotoFileStore::read(const QString & id) const
{
    if (_location.isEmpty()) {
        return {};
    }
    QFile file(QDir(_location).filePath(id));
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
    auto entries = QDir(_location).entryInfoList();
    for (const auto & entry : entries) {
        if (!entry.isFile()) {
            continue;
        }
        _photo_ids.insert(entry.fileName());
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
