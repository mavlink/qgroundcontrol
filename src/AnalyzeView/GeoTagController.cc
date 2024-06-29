/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "GeoTagController.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QDir>
#include <QtCore/QUrl>

QGC_LOGGING_CATEGORY(GeoTagControllerLog, "qgc.analyzeview.geotagcontroller")

GeoTagController::GeoTagController()
    : _progress(0)
    , _inProgress(false)
{
    connect(&_worker, &GeoTagWorker::progressChanged,   this, &GeoTagController::_workerProgressChanged);
    connect(&_worker, &GeoTagWorker::error,             this, &GeoTagController::_workerError);
    connect(&_worker, &GeoTagWorker::started,           this, &GeoTagController::inProgressChanged);
    connect(&_worker, &GeoTagWorker::finished,          this, &GeoTagController::inProgressChanged);
}

GeoTagController::~GeoTagController()
{

}

void GeoTagController::setLogFile(QString filename)
{
    filename = QUrl(filename).toLocalFile();
    if (!filename.isEmpty()) {
        _worker.setLogFile(filename);
        emit logFileChanged(filename);
    }
}

void GeoTagController::setImageDirectory(QString dir)
{
    dir = QUrl(dir).toLocalFile();
    if (!dir.isEmpty()) {
        _worker.setImageDirectory(dir);
        emit imageDirectoryChanged(dir);
        if(_worker.saveDirectory() == "") {
            QDir saveDirectory = QDir(_worker.imageDirectory() + kTagged);
            if(saveDirectory.exists()) {
                _setErrorMessage(tr("Images have alreay been tagged. Existing images will be removed."));
                return;
            }
        }
    }
    _errorMessage.clear();
    emit errorMessageChanged(_errorMessage);
}

void GeoTagController::setSaveDirectory(QString dir)
{
    dir = QUrl(dir).toLocalFile();
    if (!dir.isEmpty()) {
        _worker.setSaveDirectory(dir);
        emit saveDirectoryChanged(dir);
        //-- Check and see if there are images already there
        QDir saveDirectory = QDir(_worker.saveDirectory());
        saveDirectory.setFilter(QDir::Files | QDir::Readable | QDir::NoSymLinks | QDir::Writable);
        QStringList nameFilters;
        nameFilters << "*.jpg" << "*.JPG";
        saveDirectory.setNameFilters(nameFilters);
        QStringList imageList = saveDirectory.entryList();
        if(!imageList.isEmpty()) {
            _setErrorMessage(tr("The save folder already contains images."));
            return;
        }
    }
    _errorMessage.clear();
    emit errorMessageChanged(_errorMessage);
}

void GeoTagController::startTagging()
{
    _errorMessage.clear();
    emit errorMessageChanged(_errorMessage);
    QDir imageDirectory = QDir(_worker.imageDirectory());
    if(!imageDirectory.exists()) {
        _setErrorMessage(tr("Cannot find the image directory."));
        return;
    }
    if(_worker.saveDirectory() == "") {
        QDir oldTaggedFolder = QDir(_worker.imageDirectory() + kTagged);
        if(oldTaggedFolder.exists()) {
            oldTaggedFolder.removeRecursively();
            if(!imageDirectory.mkdir(_worker.imageDirectory() + kTagged)) {
                _setErrorMessage(tr("Couldn't replace the previously tagged images"));
                return;
            }
        }
    } else {
        QDir saveDirectory = QDir(_worker.saveDirectory());
        if(!saveDirectory.exists()) {
            _setErrorMessage(tr("Cannot find the save directory."));
            return;
        }
    }
    _worker.start();
}

void GeoTagController::_workerProgressChanged(double progress)
{
    _progress = progress;
    emit progressChanged(progress);
}

void GeoTagController::_workerError(QString errorMessage)
{
    _errorMessage = errorMessage;
    emit errorMessageChanged(errorMessage);
}


void GeoTagController::_setErrorMessage(const QString& error)
{
    _errorMessage = error;
    emit errorMessageChanged(error);
}
