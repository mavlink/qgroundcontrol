/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "GeoTagController.h"
#include "GeoTagWorker.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QThread>
#include <QtCore/QUrl>

QGC_LOGGING_CATEGORY(GeoTagControllerLog, "qgc.analyzeview.geotagcontroller")

GeoTagController::GeoTagController(QObject *parent)
    : QObject(parent)
    , _worker(new GeoTagWorker())
    , _workerThread(new QThread(this))
{
    // qCDebug(GeoTagControllerLog) << Q_FUNC_INFO << this;

    _worker->moveToThread(_workerThread);

    (void) connect(_worker, &GeoTagWorker::progressChanged, this, &GeoTagController::_workerProgressChanged);
    (void) connect(_worker, &GeoTagWorker::error, this, &GeoTagController::_workerError);
    (void) connect(_workerThread, &QThread::started, _worker, &GeoTagWorker::process);
    (void) connect(_workerThread, &QThread::started, this, &GeoTagController::inProgressChanged);
    (void) connect(_workerThread, &QThread::finished, this, &GeoTagController::inProgressChanged);
}

GeoTagController::~GeoTagController()
{
    cancelTagging();
    delete _worker;

    // qCDebug(GeoTagControllerLog) << Q_FUNC_INFO << this;
}

void GeoTagController::cancelTagging()
{
    (void) QMetaObject::invokeMethod(_worker, "cancelTagging", Qt::AutoConnection);
    (void) QMetaObject::invokeMethod(_workerThread, "quit", Qt::AutoConnection);

    _workerThread->wait();
}

QString GeoTagController::logFile() const
{
    return _worker->logFile();
}

QString GeoTagController::imageDirectory() const
{
    return _worker->imageDirectory();
}

QString GeoTagController::saveDirectory() const
{
    return _worker->saveDirectory();
}

bool GeoTagController::inProgress() const
{
    return _workerThread->isRunning();
}

void GeoTagController::setLogFile(const QString &filename)
{
    if (filename.isEmpty()) {
        _setErrorMessage(tr("Empty Filename."));
        return;
    }

    const QFileInfo logFileInfo = QFileInfo(filename);
    if (!logFileInfo.exists() || !logFileInfo.isFile()) {
        _setErrorMessage(tr("Invalid Filename."));
        return;
    }

    _worker->setLogFile(filename);
    emit logFileChanged(filename);

    _setErrorMessage(QString());
}

void GeoTagController::setImageDirectory(const QString &dir)
{
    if (dir.isEmpty()) {
        _setErrorMessage(tr("Invalid Directory."));
        return;
    }

    const QFileInfo imageDirectoryInfo = QFileInfo(dir);
    if (!imageDirectoryInfo.exists() || !imageDirectoryInfo.isDir()) {
        _setErrorMessage(tr("Invalid Directory."));
        return;
    }

    _worker->setImageDirectory(dir);
    emit imageDirectoryChanged(dir);

    if (_worker->saveDirectory().isEmpty()) {
        const QDir saveDirectory = QDir(_worker->imageDirectory() + kTagged);
        if (saveDirectory.exists()) {
            _setErrorMessage(tr("Images have already been tagged. Existing images will be removed."));
            return;
        }
    }

    _setErrorMessage(QString());
}

void GeoTagController::setSaveDirectory(const QString &dir)
{
    if (dir.isEmpty()) {
        _setErrorMessage(tr("Invalid Directory."));
        return;
    }

    const QFileInfo saveDirectoryInfo = QFileInfo(dir);
    if (!saveDirectoryInfo.exists() || !saveDirectoryInfo.isDir()) {
        _setErrorMessage(tr("Invalid Directory."));
        return;
    }

    _worker->setSaveDirectory(dir);
    emit saveDirectoryChanged(dir);

    QDir saveDirectory = QDir(_worker->saveDirectory());
    saveDirectory.setFilter(QDir::Files | QDir::Readable | QDir::NoSymLinks | QDir::Writable);

    QStringList nameFilters;
    nameFilters << "*.jpg" << "*.JPG";
    saveDirectory.setNameFilters(nameFilters);

    const QStringList imageList = saveDirectory.entryList();
    if (!imageList.isEmpty()) {
        _setErrorMessage(tr("The save folder already contains images."));
        return;
    }

    _setErrorMessage(QString());
}

void GeoTagController::startTagging()
{
    _setErrorMessage(QString());

    const QDir imageDirectory = QDir(_worker->imageDirectory());
    if (!imageDirectory.exists()) {
        _setErrorMessage(tr("Cannot find the image directory."));
        return;
    }

    if (_worker->saveDirectory().isEmpty()) {
        QDir oldTaggedFolder = QDir(_worker->imageDirectory() + kTagged);
        if (oldTaggedFolder.exists()) {
            oldTaggedFolder.removeRecursively();
            if (!imageDirectory.mkdir(_worker->imageDirectory() + kTagged)) {
                _setErrorMessage(tr("Couldn't replace the previously tagged images"));
                return;
            }
        }
    } else {
        const QDir saveDirectory = QDir(_worker->saveDirectory());
        if (!saveDirectory.exists()) {
            _setErrorMessage(tr("Cannot find the save directory."));
            return;
        }
    }

    (void) QMetaObject::invokeMethod(_workerThread, "start", Qt::AutoConnection);
}
