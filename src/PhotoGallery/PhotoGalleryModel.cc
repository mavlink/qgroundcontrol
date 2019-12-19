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

#include "ExtractJPEGMetadata.h"
#include "PhotoFileStoreInterface.h"

namespace {

constexpr size_t num_loader_threads = 2;
constexpr int limit_memory_image_width = 1280;
constexpr int limit_memory_image_height = 800;

/// Sort two strings representing (relative) file thas in descending order of
/// base names.
///
/// Example: xx/yy/zz/b comes before qqq/a (because b before a in descending
/// order).
class BaseNameComparator {
public:
    bool operator()(const QString & left, const QString & right) const
    {
        int i_left = left.lastIndexOf('/');
        int i_right = right.lastIndexOf('/');
        QStringRef base_left(&left, i_left + 1, left.size() - i_left - 1);
        QStringRef base_right(&right, i_right + 1, right.size() - i_right - 1);
        return base_left > base_right || (base_left == base_right && left > right);
    }
};

}  // namespace

PhotoGalleryModel::~PhotoGalleryModel()
{
    {
        QMutexLocker guard(&_mutex);
        _loader_thread_request_exit = true;
        _queue_condition.notify_all();
    }
    for (const auto& thread : _loader_threads) {
        thread->wait();
    }
}

PhotoGalleryModel::PhotoGalleryModel(PhotoFileStoreInterface * store, QObject * parent)
    : PhotoGalleryModel(parent)
{
    setStore(store);
}

PhotoGalleryModel::PhotoGalleryModel(QObject * parent)
    : QObject(parent), _cache(50)
{
    for (std::size_t n = 0; n < num_loader_threads; ++n) {
        std::unique_ptr<PhotoLoaderThread> thread(new PhotoLoaderThread(this));
        thread->start();
        _loader_threads.push_back(std::move(thread));
    }
}

PhotoFileStoreInterface * PhotoGalleryModel::store() const
{
    return _store;
}

void PhotoGalleryModel::setStore(PhotoFileStoreInterface * store)
{
    clear();
    _store = store;
    if (_store) {
        addedByStore(_store->ids());
        connect(_store, &PhotoFileStoreInterface::added, this, &PhotoGalleryModel::addedByStore);
        connect(_store, &PhotoFileStoreInterface::removed, this, &PhotoGalleryModel::removedByStore);
    }
}

PhotoGalleryModel::Item PhotoGalleryModel::data(PhotoGalleryModelIndex index) const
{
    if (index < _ids.size()) {
        const QString & id = _ids[index];
        Item item;
        item.id = id;

        QMutexLocker guard(&_mutex);
        auto cached_item = _cache.object(id);
        if (cached_item) {
            item.image = cached_item->image;
            item.metadata = cached_item->metadata;
        } else {
            loadAsync(id);
        }
        return item;
    } else {
        return {};
    }
}

PhotoGalleryModel::Item PhotoGalleryModel::dataSync(PhotoGalleryModelIndex index) const
{
    if (index < _ids.size()) {
        const QString & id = _ids[index];
        Item item;
        item.id = id;
        CacheItem* cached_item = nullptr;

        QMutexLocker guard(&_mutex);
        do {
            cached_item = _cache.object(id);
            if (!cached_item) {
                loadAsync(id);
                _completion_condition.wait(&_mutex);
            }
        } while (!cached_item);
        item.image = cached_item->image;
        item.metadata = cached_item->metadata;
        return item;
    } else {
        return {};
    }
}

QString PhotoGalleryModel::filePath(PhotoGalleryModelIndex index) const
{
    if (index < _ids.size()) {
        const QString & id = _ids[index];
        return QDir::cleanPath(store()->location() + QDir::separator() + id);
    }
    return QString();
}

QString PhotoGalleryModel::photosPath() const
{
    return store()->location();
}

QString PhotoGalleryModel::videosPath() const
{
    return store()->videoLocation();
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

    // The order of ids given by store does not match our sorting criteria:
    // We want "newest" image first, independent of vehicle/flight assignment.
    // Produce correct order for insertion.
    BaseNameComparator comparator;
    std::vector<QString> ids_sorted(ids.begin(), ids.end());
    std::sort(ids_sorted.begin(), ids_sorted.end(), comparator);

    // Yes this looks quite stupid, performing O(n^2) insertions into a vector
    // in the worst case.
    // This is however called in two ways only:
    // - when store is empty: everything is filled in correct sequence, so it
    //   is O(n) then
    // - when taking new photo: in all likelihood, things are sorted by date
    //   already so new image goes to end, meaning O(1). In rare circumstances,
    //   it is O(n) if an image is put into the middle, but this is still not
    //   worth worrying as we are not taking millions of pictures per second.
    for (const auto & id : ids_sorted) {
        auto i = std::lower_bound(_ids.begin(), _ids.end(), id, comparator);
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

void PhotoGalleryModel::loadAsync(const QString& id) const
{
    if (_load_items.find(id) != _load_items.end()) {
        return;
    }
    _load_items.insert(id);
    _load_queue.push_back(id);
    _queue_condition.notify_one();
}

void PhotoGalleryModel::clear()
{
    {
        // Make sure that asychronous load operations are quiet before resetting
        // store to a different value. See concurrency notes on the "_store"
        // member.
        QMutexLocker guard(&_mutex);
        _load_queue.clear();
        _load_items.clear();
        while (_loader_threads_idle != _loader_threads.size()) {
            _idle_condition.wait(&_mutex);
        }
        _cache.clear();
    }

    if (_store) {
        disconnect(_store, &PhotoFileStoreInterface::added, this, &PhotoGalleryModel::addedByStore);
        disconnect(_store, &PhotoFileStoreInterface::removed, this, &PhotoGalleryModel::removedByStore);
    }
    _ids.clear();
    std::set<PhotoGalleryModelIndex> indices_removed;
    for (std::size_t n = 0; n < _ids.size(); ++n) {
        indices_removed.insert(PhotoGalleryModelIndex(n));
    }

    emit removed(indices_removed);
}

void PhotoGalleryModel::loaderFunction()
{
    for (;;) {
        QString id;
        {
            QMutexLocker guard(&_mutex);
            while (_load_queue.empty() && !_loader_thread_request_exit) {
                ++_loader_threads_idle;
                _idle_condition.notify_one();
                _queue_condition.wait(&_mutex);
                --_loader_threads_idle;
            }
            if (_loader_thread_request_exit) {
                break;
            }
            id = _load_queue.front();
            _load_queue.pop_front();
        }

        auto data = _store->read(id);
        CacheItem* item = new CacheItem;
        if (data.canConvert<QByteArray>()) {
            const auto& bytes = data.value<QByteArray>();
            item->image = std::make_shared<QImage>();
            item->metadata = std::make_shared<std::map<QString, QString>>();

            if (item->image->loadFromData(bytes)) {
                if (item->image->width() > limit_memory_image_width
                    || item->image->height() > limit_memory_image_height) {
                    *item->image = item->image->scaled(
                        QSize(limit_memory_image_width, limit_memory_image_height),
                        Qt::KeepAspectRatio, Qt::SmoothTransformation);
                }
                *item->metadata = extractJPEGMetadata(bytes);
            }
        }

        {
            QMutexLocker guard(&_mutex);
            _cache.insert(id, item);
            _load_items.erase(id);
            _completion_condition.notify_all();
        }

        emit loaded();
    }
}

PhotoLoaderThread::PhotoLoaderThread(PhotoGalleryModel* model)
    : QThread(model), _model(model)
{
}

PhotoLoaderThread::~PhotoLoaderThread()
{
}

void PhotoLoaderThread::run()
{
    _model->loaderFunction();
}


namespace {

void registerPhotoGalleryModelMetaType()
{
    // XXX: correct namespace
    qmlRegisterType<PhotoGalleryModel>("QGroundControl.Controllers", 1, 0, "PhotoGalleryModel");
}

}  // namespace

Q_COREAPP_STARTUP_FUNCTION(registerPhotoGalleryModelMetaType);
