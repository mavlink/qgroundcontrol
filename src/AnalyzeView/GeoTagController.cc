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
#include <math.h>
#include <QtEndian>
#include <QDebug>
#include <cfloat>
#include <QDir>
#include <QUrl>

#include "ExifParser.h"
#include "ULogParser.h"
#include "PX4LogParser.h"

static const char* kTagged = "/TAGGED";

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

GeoTagWorker::GeoTagWorker()
    : _cancel(false)
{

}

void GeoTagWorker::run()
{
    _cancel = false;
    emit progressChanged(1);
    double nSteps = 5;

    // Load Images
    _imageList.clear();
    QDir imageDirectory = QDir(_imageDirectory);
    imageDirectory.setFilter(QDir::Files | QDir::Readable | QDir::NoSymLinks | QDir::Writable);
    imageDirectory.setSorting(QDir::Name);
    QStringList nameFilters;
    nameFilters << "*.jpg" << "*.JPG";
    imageDirectory.setNameFilters(nameFilters);
    _imageList = imageDirectory.entryInfoList();
    if(_imageList.isEmpty()) {
        emit error(tr("The image directory doesn't contain images, make sure your images are of the JPG format"));
        return;
    }
    emit progressChanged((100/nSteps));

    // Parse EXIF
    ExifParser exifParser;
    _imageTime.clear();
    for (int i = 0; i < _imageList.size(); ++i) {
        QFile file(_imageList.at(i).absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly)) {
            emit error(tr("Geotagging failed. Couldn't open an image."));
            return;
        }
        QByteArray imageBuffer = file.readAll();
        file.close();

        _imageTime.append(exifParser.readTime(imageBuffer));

        emit progressChanged((100/nSteps) + ((100/nSteps) / _imageList.size())*i);

        if (_cancel) {
            qCDebug(GeotaggingLog) << "Tagging cancelled";
            emit error(tr("Tagging cancelled"));
            return;
        }
    }

    // Load log
    bool isULog = _logFile.endsWith(".ulg", Qt::CaseSensitive);
    QFile file(_logFile);
    if (!file.open(QIODevice::ReadOnly)) {
        emit error(tr("Geotagging failed. Couldn't open log file."));
        return;
    }
    QByteArray log = file.readAll();
    file.close();

    // Instantiate appropriate parser
    _triggerList.clear();
    bool parseComplete = false;
    QString errorString;
    if (isULog) {
        ULogParser parser;
        parseComplete = parser.getTagsFromLog(log, _triggerList, errorString);

    } else {
        PX4LogParser parser;
        parseComplete = parser.getTagsFromLog(log, _triggerList);

    }

    if (!parseComplete) {
        if (_cancel) {
            qCDebug(GeotaggingLog) << "Tagging cancelled";
            emit error(tr("Tagging cancelled"));
            return;
        } else {
            qCDebug(GeotaggingLog) << "Log parsing failed";
            errorString = tr("%1 - tagging cancelled").arg(errorString.isEmpty() ? tr("Log parsing failed") : errorString);
            emit error(errorString);
            return;
        }
    }
    emit progressChanged(3*(100/nSteps));

    qCDebug(GeotaggingLog) << "Found " << _triggerList.count() << " trigger logs.";

    if (_cancel) {
        qCDebug(GeotaggingLog) << "Tagging cancelled";
        emit error(tr("Tagging cancelled"));
        return;
    }

    // Filter Trigger
    if (!triggerFiltering()) {
        qCDebug(GeotaggingLog) << "Geotagging failed in trigger filtering";
        emit error(tr("Geotagging failed in trigger filtering"));
        return;
    }
    emit progressChanged(4*(100/nSteps));

    if (_cancel) {
        qCDebug(GeotaggingLog) << "Tagging cancelled";
        emit error(tr("Tagging cancelled"));
        return;
    }

    // Tag images
    int maxIndex = std::min(_imageIndices.count(), _triggerIndices.count());
    maxIndex = std::min(maxIndex, _imageList.count());
    for(int i = 0; i < maxIndex; i++) {
        int imageIndex = _imageIndices[i];
        if (imageIndex >= _imageList.count()) {
            emit error(tr("Geotagging failed. Requesting image #%1, but only %2 images present.").arg(imageIndex).arg(_imageList.count()));
            return;
        }
        QFile fileRead(_imageList.at(_imageIndices[i]).absoluteFilePath());
        if (!fileRead.open(QIODevice::ReadOnly)) {
            emit error(tr("Geotagging failed. Couldn't open an image."));
            return;
        }
        QByteArray imageBuffer = fileRead.readAll();
        fileRead.close();

        if (!exifParser.write(imageBuffer, _triggerList[_triggerIndices[i]])) {
            emit error(tr("Geotagging failed. Couldn't write to image."));
            return;
        } else {
            QFile fileWrite;
            if(_saveDirectory == "") {
                fileWrite.setFileName(_imageDirectory + "/TAGGED/" + _imageList.at(_imageIndices[i]).fileName());
            } else {
                fileWrite.setFileName(_saveDirectory + "/" + _imageList.at(_imageIndices[i]).fileName());
            }
            if (!fileWrite.open(QFile::WriteOnly)) {
                emit error(tr("Geotagging failed. Couldn't write to an image."));
                return;
            }
            fileWrite.write(imageBuffer);
            fileWrite.close();
        }
        emit progressChanged(4*(100/nSteps) + ((100/nSteps) / maxIndex)*i);

        if (_cancel) {
            qCDebug(GeotaggingLog) << "Tagging cancelled";
            emit error(tr("Tagging cancelled"));
            return;
        }
    }

    if (_cancel) {
        qCDebug(GeotaggingLog) << "Tagging cancelled";
        emit error(tr("Tagging cancelled"));
        return;
    }

    emit progressChanged(100);
}

bool GeoTagWorker::triggerFiltering()
{
    _imageIndices.clear();
    _triggerIndices.clear();
    if(_imageList.count() > _triggerList.count()) {             // Logging dropouts
        qCDebug(GeotaggingLog) << "Detected missing feedback packets.";
    } else if (_imageList.count() < _triggerList.count()) {     // Camera skipped frames
        qCDebug(GeotaggingLog) << "Detected missing image frames.";
    }
    for(int i = 0; i < _imageList.count() && i < _triggerList.count(); i++) {
        _imageIndices.append(static_cast<int>(_triggerList[i].imageSequence));
        _triggerIndices.append(i);
    }
    return true;
}
