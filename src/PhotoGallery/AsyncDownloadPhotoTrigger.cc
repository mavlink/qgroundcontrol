/****************************************************************************
 *
 *   (c) 2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AsyncDownloadPhotoTrigger.h"

#include <QNetworkReply>

#include "PhotoFileStore.h"

AsyncDownloadPhotoTriggerOperation::~AsyncDownloadPhotoTriggerOperation()
{
}

bool AsyncDownloadPhotoTriggerOperation::finished() const
{
    return _finished;
}
bool AsyncDownloadPhotoTriggerOperation::success() const
{
    return _success;
}

QString AsyncDownloadPhotoTriggerOperation::id() const
{
    return _id;
}

void AsyncDownloadPhotoTriggerOperation::finishSuccess(QString id)
{
    _finished = true;
    _success = true;
    _id = std::move(id);
    emit finish();
    delete this;
}

void AsyncDownloadPhotoTriggerOperation::finishFailure()
{
    _finished = true;
    _success = false;
    emit finish();
    delete this;
}

AsyncDownloadPhotoTrigger::~AsyncDownloadPhotoTrigger()
{
    failPhotoOperation();
}

AsyncDownloadPhotoTrigger::AsyncDownloadPhotoTrigger(
    std::function<bool()> photo_trigger_fn,
    const Config & config,
    PhotoFileStore * store ,
    QObject * parent)
    : AbstractPhotoTrigger(parent),
      _photo_trigger_fn(std::move(photo_trigger_fn)),
      _config(config)
{
    _photo_timeout.setSingleShot(true);
    connect(&_photo_timeout, SIGNAL(timeout()), this, SLOT(failPhotoOperation()));
    connect(&_download_manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadFinished(QNetworkReply*)));
    if (store) {
        setStore(store);
    }
}

AsyncDownloadPhotoTriggerOperation * AsyncDownloadPhotoTrigger::takePhoto()
{
    if (_store && !_active_op) {
        if (_photo_trigger_fn()) {
            _active_op = new AsyncDownloadPhotoTriggerOperation();
            _photo_timeout.start(_config.photo_timeout / std::chrono::milliseconds(1));
            return _active_op;
        } else {
            return nullptr;
        }
    } else {
        return nullptr;
    }
}

PhotoFileStore * AsyncDownloadPhotoTrigger::store() const
{
    return _store;
}

void AsyncDownloadPhotoTrigger::setStore(PhotoFileStore * store)
{
    failPhotoOperation();
    _store = store;
}


void AsyncDownloadPhotoTrigger::completePhotoWithURI(const QString & uri)
{
    // We can be called in two different scenarios:
    // - Photo was triggered through "takePhoto" API function: In this case
    //   there must be an active operation, but no download yet (because
    //   we would otherwise already have initiated download). Initiate download
    //   and track it.
    // - "Unsolicited": Some photo was taken, but we did not ask for it. In
    //   the latter case, also initiate download, but do not bother tracking.
    if (_active_op && !_active_reply) {
        _active_reply = _download_manager.get(QNetworkRequest(QUrl(uri)));
        // NB: QNetworkReply has progress signals, could pass that on.
    } else {
        _download_manager.get(QNetworkRequest(QUrl(uri)));
    }
}

void AsyncDownloadPhotoTrigger::completePhotoFailed()
{
    // See comments to completePhotoWithURI regarding this notification.
    if (_active_op && !_active_reply) {
        failPhotoOperation();
    }
}

void AsyncDownloadPhotoTrigger::failPhotoOperation()
{
    if (_active_op) {
        _active_op->finishFailure();
        _active_op = nullptr;
    }
    if (_active_reply) {
        _active_reply->abort();
        _active_reply = nullptr;
    }
}

void AsyncDownloadPhotoTrigger::downloadFinished(QNetworkReply * reply)
{
    if (reply == _active_reply) {
        if (reply->error()) {
            _active_op->finishFailure();
        } else {
            QByteArray data = reply->readAll();
            QString id = _store->add(reply->url().fileName(), std::move(data));
            _active_op->finishSuccess(id);
        }
        _active_op = nullptr;
        _active_reply = nullptr;
    } else {
        if (!reply->error()) {
            QByteArray data = reply->readAll();
            QString id = _store->add(reply->url().fileName(), std::move(data));
        }
    }
}
