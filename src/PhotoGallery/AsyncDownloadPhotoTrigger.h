/****************************************************************************
 *
 *   (c) 2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QTimer>

#include "AbstractPhotoTrigger.h"

class AsyncDownloadPhotoTrigger;
class PhotoFileStore;


/// A "photo" operation in progress
///
/// Represents a photo being taken. This object exists while this is in
/// progress.
class AsyncDownloadPhotoTriggerOperation final : public AbstractPhotoTriggerOperation {
    Q_OBJECT
public:
    ~AsyncDownloadPhotoTriggerOperation() override;

    bool finished() const override;
    bool success() const override;
    QString id() const override;

private:
    bool _finished = false;
    bool _success = false;
    QString _id;

    /// This operation has succeeded in taking a photo.
    ///
    /// \param id The id by which it was put into the store
    ///
    /// Called by the controller to conclude this operation. Sets state, emits
    /// signal and destroys this object afterwards.
    void finishSuccess(QString id);

    /// This operation has failed.
    ///
    /// Called by the controller to conclude this operation. Sets state, emits
    /// signal and destroys this object afterwards.
    void finishFailure();

    friend class AsyncDownloadPhotoTrigger;
};

/// Take a photo by async helper and download.
///
/// Take a photo by invoking a (parameter-less) asynchronous helper. The trigger
///function is parameterizable to allow injecting either the trigger to
/// mavlink camera (for real integration), or a mock (for testing).
class AsyncDownloadPhotoTrigger final : public AbstractPhotoTrigger {
    Q_OBJECT
    Q_PROPERTY(PhotoFileStore* store READ store WRITE setStore)
public:
    class Config {
    public:
        /// Timeout for photo
        ///
        /// Defines the time between initiating a photo and the time we expect
        /// the response back. If it takes longer than this, taking the photo
        /// is declared to have failed.
        std::chrono::system_clock::duration photo_timeout = std::chrono::seconds(5);
    };

    ~AsyncDownloadPhotoTrigger() override;

    /// Construct photo trigger object
    ///
    /// \param photo_trigger_fn Function to be called to start taking photo
    /// \param config Configuration
    /// \param store (Optional) the data store where photos are pushed to
    /// \param parent Qt parent object for memory management
    AsyncDownloadPhotoTrigger(
        std::function<bool()> photo_trigger_fn,
        const Config & config,
        PhotoFileStore * store = nullptr,
        QObject * parent = nullptr);

    /// Take a photo.
    ///
    /// \returns Operation in progress or nullptr.
    ///
    /// Starts taking a photo. If no photo can presently be taken at all
    /// (e.g. no camera) then this may return nullptr to indicate failure.
    /// Otherwise, returns a continuation object to represent the operation in
    /// progress.
    ///
    /// The continuation object is alive for as long as the operation is in
    /// progress, up and until its "finish" signal has been emitted.
    AsyncDownloadPhotoTriggerOperation * takePhoto() override;

    PhotoFileStore * store() const;
    void setStore(PhotoFileStore * store);

public slots:
    /// External notice of completed photo operation
    ///
    /// Called from other system (mavlink camera handler) to indicate that a
    /// photo has been taken. This tries correlates it with an operation
    /// initiated through this interface.
    void completePhotoWithURI(const QString & uri);

    /// External notice of completed photo operation
    ///
    /// Called from other system (mavlink camera handler) to indicate that an
    /// attempt to take a photo has failed. This tries to correlate it with
    /// an operation initiated through this interface.
    void completePhotoFailed();

private slots:
    /// Fail an operation started by "takePhoto".
    void failPhotoOperation();

    /// Callback issued when network download has completed.
    void downloadFinished(QNetworkReply * reply);

private:

    /// Functional to trigger a photo
    ///
    /// Attempts to start taking a photo. If the attempt fails outright,
    /// should return false. Otherwise, should return true to indicate
    /// that an operation is in progress.
    std::function<bool()> _photo_trigger_fn;

    Config _config;

    /// Store where to put acquired photos.
    PhotoFileStore * _store = nullptr;

    /// Currently  ongoing operation.
    AsyncDownloadPhotoTriggerOperation * _active_op = nullptr;

    /// Download for ongoing operation.
    ///
    /// Invariant: this may only be non-null if _active_op is non-null as well.
    QNetworkReply * _active_reply = nullptr;

    /// Timer to declare on overdue operation terminated.
    QTimer _photo_timeout;

    /// Manager for retrieval if images.
    QNetworkAccessManager _download_manager;
};
