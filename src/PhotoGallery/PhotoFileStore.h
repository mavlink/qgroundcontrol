/****************************************************************************
 *
 *   (c) 2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QMutex>
#include <QObject>

#include <set>

#include "PhotoFileStoreInterface.h"

/// Represent photo collection stored on filesystem
///
/// Manages a collection of photos stored as files in some filesystem. It
/// provides facilities to list, retrieve, add, modify and remove files. Its
/// raison d'être is to
/// - provide notifiers about changes to store such that* different actors can
///   have a consistent view. (NB: does///not* imply thread synchronization --
///   assuming to be accessed from main GUI thread only), and
/// - manage assigning ids to incoming files
///
/// The store presently does not manage folders/sub-directories. This is a
/// feature that could be added, although it might be preferrable from GUI
/// perspective to use "tags" independent of physical storage location for
/// grouping.
///
/// Another feature that is not fully clear whether this is the best place is
/// synchronization with external storage -- might go here or elsewhere.
class PhotoFileStore : public PhotoFileStoreInterface {
    Q_OBJECT

public:
    explicit PhotoFileStore(QObject * parent = nullptr);
    explicit PhotoFileStore(QString location, QObject * parent = nullptr);

    ~PhotoFileStore() override;

    /// Adds photo to the store.
    ///
    /// \param name_hint Proposed internal id for photo (also, filename). There
    /// is no guarantee that this name is going to be used, though.
    /// \param data Raw photo data.
    /// \returns ID of the photo stored, for later access.
    ///
    /// This adds a new photo to the store. The name given is taken as a hint
    /// such that the resulting file has the same name as on the origin device
    /// (simplifies diagnostics), but ultimately this may choose a new name
    /// to avoid collisions and/or help with maintaining internal sorting.
    QString add(QString name_hint, QByteArray data) override;

    /// Gets list of photo ids held.
    const std::set<QString> & ids() const override;

    /// Permanently erases photos from store.
    ///
    /// \param ids Identifiers of photos to be erased.
    void remove(const std::set<QString> & ids) override;

    /// Reads photo data.
    ///
    /// \param id Identifier of photo to be read.
    ///
    /// Reads data for the given photo from store. The returned QVariant will
    /// contain a QByteArray if the read succeeded, or be empty otherwise.
    /// Check via .canConvert<QByteArray> and access via .value<QByteArray>.
    ///
    /// Read failure should be considered as a "broken image".
    ///
    /// This function is thread-safe with respect to all other member functions
    /// including concurrent reads.
    QVariant read(const QString & id) const override;

    void setLocation(QString local_storage) override;
    const QString & location() const override;

    void setVideoLocation(QString local_storage);
    const QString & videoLocation() const;

private:
    /// Check filesystem whether any file added/removed.
    void rescan();

    QString _videoLocation;
    std::set<QString> _photo_ids;
    QString _location;  // GUARDED_BY(_mutex)
    mutable QMutex _mutex;
};
