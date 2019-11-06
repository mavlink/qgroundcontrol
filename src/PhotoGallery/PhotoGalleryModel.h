/****************************************************************************
 *
 *   (c) 2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <memory>
#include <set>
#include <vector>

#include <QCache>
#include <QObject>

class PhotoFileStore;

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

    Q_PROPERTY(PhotoFileStore* store READ store WRITE setStore);

public:
    struct Item {
        QString id;
        std::shared_ptr<const QImage> image;
    };

    ~PhotoGalleryModel() override;

    explicit PhotoGalleryModel(PhotoFileStore * store, QObject * parent = nullptr);
    explicit PhotoGalleryModel(QObject * parent = nullptr);

    PhotoFileStore * store() const;
    void setStore(PhotoFileStore * store);

    /// Get item at index.
    ///
    /// \param index Index of element to access.
    ///
    /// Returns the item held for the given index, if any. Caller is expected
    /// to check whether id is empty -- this indicates that no item of given
    /// index exists.
    Item data(PhotoGalleryModelIndex index) const;

    /// Number of entries held.
    std::size_t numPhotos() const;

    /// Remove images by their ids.
    void remove(const std::set<QString> & ids);

signals:
    /// Notify that indices are added.
    void added(const std::set<PhotoGalleryModelIndex> & indices);
    /// Notify that indices are removed.
    void removed(const std::set<PhotoGalleryModelIndex> & indices);

private slots:
    /// Notification from data store that new photos have been added.
    void addedByStore(const std::set<QString> & ids);

    /// Notification from data store that photos have been deleted.
    void removedByStore(const std::set<QString> & ids);

private:
    /// Delete all data in model.
    void clear();

    PhotoFileStore * _store = nullptr;
    std::vector<QString> _ids;

    /// Cache of images.
    ///
    /// Keep a bounded number of images in memory (avoid consuming all
    /// memory in case we have a large number of images stored).
    mutable QCache<QString, std::shared_ptr<QImage>> _cache;
};
