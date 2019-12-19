/****************************************************************************
 *
 *   (c) 2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <deque>
#include <map>
#include <memory>
#include <set>
#include <vector>

#include <QCache>
#include <QObject>
#include <QMutex>
#include <QThread>
#include <QWaitCondition>

class PhotoLoaderThread;
class PhotoFileStoreInterface;

/// Index into photo PhotoGalleryModel
///
/// Currently, photo gallery exposes a "flat" view of a list of images. This
/// may change when introducing tags to group images by.
///
using PhotoGalleryModelIndex = std::size_t;

/// Represent photo gallery data.
///
/// Represent all photos held in the gallery in a form suitable for manipulation
/// through user interface. The fact that photos are stored as files and are
/// binary data is abstracted away in this representations -- we are dealing
/// with actual images, and the images also have a well-defined order.
class PhotoGalleryModel : public QObject {
    Q_OBJECT

    Q_PROPERTY(PhotoFileStoreInterface* store READ store WRITE setStore);

public:
    struct Item {
        /// Id of the image, to be used as handle for interaction with storage.
        QString id;
        /// Decompressed image corresponding to this id. It is possible that
        /// this may not be available at a given point in time -- decompression
        /// is handled asynchronously.
        std::shared_ptr<const QImage> image;
    };

    ~PhotoGalleryModel() override;

    explicit PhotoGalleryModel(PhotoFileStoreInterface * store, QObject * parent = nullptr);
    explicit PhotoGalleryModel(QObject * parent = nullptr);

    PhotoFileStoreInterface * store() const;
    void setStore(PhotoFileStoreInterface * store);

    /// Get item at index.
    ///
    /// \param index Index of element to access.
    ///
    /// Returns the item held for the given index, if any. Caller is expected
    /// to check whether id is empty -- this indicates that no item of given
    /// index exists.
    ///
    /// It is possible that the image + metadata returned are null -- this
    /// indicates that the data is not in memory yet. UI should query again
    /// later, based on the "loaded" signal below.
    Item data(PhotoGalleryModelIndex index) const;

    /// Get item at index.
    ///
    /// This behaves almost exactly like "data" above, but it synchronously
    /// waits for data to be in memory. This method should not be used for
    /// normal operation but is convenient for testing.
    Item dataSync(PhotoGalleryModelIndex index) const;

    /// Get item file path.
    ///
    /// \param index Index of element to access.
    ///
    /// Returns the full path of the item represented by index
    /// or empty string if failed
    QString filePath(PhotoGalleryModelIndex index) const;

    /// Get photo storage path.
    QString photosPath() const;
    /// Get videos storage path.
    QString videosPath() const;

    /// Number of entries held.
    std::size_t numPhotos() const;

    /// Remove images by their ids.
    void remove(const std::set<QString> & ids);

signals:
    /// Notify that indices are added.
    void added(const std::set<PhotoGalleryModelIndex> & indices);
    /// Notify that indices are removed.
    void removed(const std::set<PhotoGalleryModelIndex> & indices);
    /// Notify that one or more images has been loaded into memory. UI should
    /// use this to replace "placeholder" images representing asynchronous
    /// load state with real images on screen.
    void loaded() const;

private slots:
    /// Notification from data store that new photos have been added.
    void addedByStore(const std::set<QString> & ids);

    /// Notification from data store that photos have been deleted.
    void removedByStore(const std::set<QString> & ids);

private:
    struct CacheItem {
        std::shared_ptr<QImage> image;
    };

    /// Delete all data in model.
    void clear();

    // Starts to asynchronously load the image given by id. This will do
    // nothing if a load operation is in progress already, but caller should
    // ensure that item is not in cache already.
    void loadAsync(const QString& id) const;  // EXCLUSIVE_LOCKS_HELD(_mutex);

    // Data store of images.
    //
    // Notes on concurrency: This member variable is written to only by its
    // property accessor (from main Qt thread). It is also accessed by
    // concurrent thread(s) responsible for asynchronous loading of images.
    // This is synchronized in the following way:
    // - Asynchronous load requests may only be *initiated* from main Qt thread
    //   (same as possibly writing this member).
    // - For any async load request initiated after writing this variable, we
    //   therefore have "happens-before" relation between writing this variable
    //   and accessing it in another thread.
    // - For any async load request initiated before writing this variable,
    //   we wait for its completion in the "clear" method (called as part
    //   of writing a different value). This establishes "happens-before"
    //   between async loader thread and writing a new value.
    // We really need to block and wait for completion of all load requests
    // before changing this value -- after all, as soon as the property setter
    // returns the PhotoFileStoreInterface instance might go out of scope and be
    // destroyed. It is not an option to just lock-guard this data member
    // alone and let some load operations "linger" asynchronously.
    PhotoFileStoreInterface * _store = nullptr;

    /// Actual loader thread function.
    void loaderFunction();

    std::vector<QString> _ids;

    /// Cache of images.
    ///
    /// Keep a bounded number of images in memory (avoid consuming all
    /// memory in case we have a large number of images stored).
    mutable QCache<QString, CacheItem> _cache;

    /// Threads handling image loading. Explicitly controlling number of
    /// threads for now, to avoid overwhelming low-powered systems.
    std::vector<std::unique_ptr<QThread>> _loader_threads;

    /// Items pending to be loaded. The first represents the queue in the order
    /// of items requested. The second allows to quickly determine if an item
    /// is on the queue.
    mutable std::deque<QString> _load_queue;  // GUARDED_BY(_mutex);
    mutable std::set<QString> _load_items;  // GUARDED_BY(_mutex);
    // Request loader thread to stop.
    mutable bool _loader_thread_request_exit = false;
    /// To loader thread, that either new item queuedNew item has been queued
    /// or request to exit.
    mutable QWaitCondition _queue_condition;

    /// From loader thread, to indicate that it is idle (and therefore safe to
    /// clear cache and change _store).
    mutable std::size_t _loader_threads_idle = 0;  // GUARDED_BY(_mutex);
    mutable QWaitCondition _idle_condition;

    /// From loader thread, one image has been loaded.
    mutable QWaitCondition _completion_condition;

    mutable QMutex _mutex;

    friend class PhotoLoaderThread;
};

class PhotoLoaderThread : public QThread {
public:
    PhotoLoaderThread(PhotoGalleryModel* model);
    ~PhotoLoaderThread() override;

    void run() override;

private:
    PhotoGalleryModel* _model;
};
