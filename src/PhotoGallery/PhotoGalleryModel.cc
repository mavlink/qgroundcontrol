/****************************************************************************
 *
 *   (c) 2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "PhotoGalleryModel.h"

#include <QImage>
#include <QVariant>
#include <QtQml>

#include "PhotoFileStore.h"

PhotoGalleryModel::~PhotoGalleryModel()
{
}

PhotoGalleryModel::PhotoGalleryModel(PhotoFileStore * store, QObject * parent)
    : PhotoGalleryModel(parent)
{
    setStore(store);
}

PhotoGalleryModel::PhotoGalleryModel(QObject * parent)
    : QObject(parent), _cache(100)
{
}

PhotoFileStore * PhotoGalleryModel::store() const
{
    return _store;
}

void PhotoGalleryModel::setStore(PhotoFileStore * store)
{
    clear();
    _store = store;
    if (_store) {
        addedByStore(_store->ids());
        connect(_store, &PhotoFileStore::added, this, &PhotoGalleryModel::addedByStore);
        connect(_store, &PhotoFileStore::removed, this, &PhotoGalleryModel::removedByStore);
    }
}

PhotoGalleryModel::Item PhotoGalleryModel::data(PhotoGalleryModelIndex index) const
{
    if (index < _ids.size()) {
        const QString & id = _ids[index];
        Item item;
        item.id = id;
        auto cached_item = _cache.object(id);
        if (cached_item) {
            item.image = *cached_item;
        } else {
            auto data = _store->read(id);
            if (data.canConvert<QByteArray>()) {
                auto ptr = std::make_shared<QImage>();
                if (ptr->loadFromData(data.value<QByteArray>())) {
                    _cache.insert(id, new std::shared_ptr<QImage>(ptr));
                    item.image = std::move(ptr);
                }
            }
        }
        return item;
    } else {
        return {};
    }
}

std::size_t PhotoGalleryModel::numPhotos() const
{
    return _ids.size();
}

void PhotoGalleryModel::remove(const std::set<QString> & ids)
{
    if (_store) {
        _store->remove(ids);
    }
}


void PhotoGalleryModel::addedByStore(const std::set<QString> & ids)
{
    std::set<PhotoGalleryModelIndex> indices_added;

    // Yes this looks quite stupid, performing O(n^2) insertions into a vector
    // in the worst case.
    // This is however called in two ways only:
    // - when store is empty: everything is filled in correct sequence, so it
    //   it O(n) then
    // - when taking new photo: in all likelihood, things are sorted by date
    //   already so new image goes to end, meaning O(1). In rare circumstances,
    //   it is O(n) if an image is put into the middle, but this is still not
    //   worth worrying as we are not taking millions of pictures per second.
    for (auto & id : ids) {
        auto i = std::lower_bound(_ids.begin(), _ids.end(), id);
        std::unique_ptr<Item> item(new Item);
        i = _ids.insert(i, std::move(id));
        indices_added.insert(i - _ids.begin());
    }
    emit added(indices_added);
}

void PhotoGalleryModel::removedByStore(const std::set<QString> & ids)
{
    std::set<PhotoGalleryModelIndex> indices_removed;
    for (std::size_t n = _ids.size(); n > 0; --n) {
        std::size_t index = n - 1;
        if (ids.find(_ids[index]) != ids.end()) {
            indices_removed.insert(PhotoGalleryModelIndex(index));
            _cache.remove(_ids[index]);
            _ids.erase(_ids.begin() + index);
        }
    }

    emit removed(indices_removed);
}

void PhotoGalleryModel::clear()
{
    if (_store) {
        disconnect(_store, &PhotoFileStore::added, this, &PhotoGalleryModel::addedByStore);
        disconnect(_store, &PhotoFileStore::removed, this, &PhotoGalleryModel::removedByStore);
    }
    _cache.clear();
    _ids.clear();
    std::set<PhotoGalleryModelIndex> indices_removed;
    for (std::size_t n = 0; n < _ids.size(); ++n) {
        indices_removed.insert(PhotoGalleryModelIndex(n));
    }

    emit removed(indices_removed);
}

namespace {

void registerPhotoFileStoreMetaType()
{
    // XXX: correct namespace
    qmlRegisterType<PhotoGalleryModel>("QGroundControl.Controllers", 1, 0, "PhotoGalleryModel");
}

}  // namespace

Q_COREAPP_STARTUP_FUNCTION(registerPhotoFileStoreMetaType);
