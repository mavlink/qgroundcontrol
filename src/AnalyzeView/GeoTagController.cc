/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "GeoTagController.h"
#include "QGCQFileDialog.h"
#include "QGCLoggingCategory.h"
#include "MainWindow.h"
#include <math.h>
#include <QtEndian>
#include <QMessageBox>
#include <QDebug>
#include <cfloat>

#include "ExifParser.h"
#include "ULogParser.h"
#include "PX4LogParser.h"

GeoTagController::GeoTagController(void)
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

void GeoTagController::pickLogFile(void)
{
    QString filename = QGCQFileDialog::getOpenFileName(MainWindow::instance(), "Select log file load", QString(), "ULog file (*.ulg);;PX4 log file (*.px4log);;All Files (*.*)");
    if (!filename.isEmpty()) {
        _worker.setLogFile(filename);
        emit logFileChanged(filename);
    }
}

void GeoTagController::pickImageDirectory(void)
{
    QString dir = QGCQFileDialog::getExistingDirectory(MainWindow::instance(), "Select image directory");
    if (!dir.isEmpty()) {
        _worker.setImageDirectory(dir);
        emit imageDirectoryChanged(dir);
    }
}

void GeoTagController::pickSaveDirectory(void)
{
    QString dir = QGCQFileDialog::getExistingDirectory(MainWindow::instance(), "Select save directory");
    if (!dir.isEmpty()) {
        _worker.setSaveDirectory(dir);
        emit saveDirectoryChanged(dir);
    }
}

void GeoTagController::startTagging(void)
{
    _errorMessage.clear();
    emit errorMessageChanged(_errorMessage);

    QDir imageDirectory = QDir(_worker.imageDirectory());
    if(!imageDirectory.exists()) {
        _errorMessage = tr("Cannot find the image directory");
        emit errorMessageChanged(_errorMessage);
        return;
    }
    if(_worker.saveDirectory() == "") {
        if(!imageDirectory.mkdir(_worker.imageDirectory() + "/TAGGED")) {
            QMessageBox msgBox(QMessageBox::Question,
                               tr("Images have alreay been tagged."),
                               tr("The images have already been tagged. Do you want to replace the previously tagged images?"),
                               QMessageBox::Cancel);
            msgBox.setWindowModality(Qt::ApplicationModal);
            msgBox.addButton(tr("Replace"), QMessageBox::ActionRole);
            if (msgBox.exec() == QMessageBox::Cancel) {
                _errorMessage = tr("Images have already been tagged");
                emit errorMessageChanged(_errorMessage);
                return;
            }
            QDir oldTaggedFolder = QDir(_worker.imageDirectory() + "/TAGGED");
            oldTaggedFolder.removeRecursively();
            if(!imageDirectory.mkdir(_worker.imageDirectory() + "/TAGGED")) {
                _errorMessage = tr("Couldn't replace the previously tagged images");
                emit errorMessageChanged(_errorMessage);
                return;
            }
        }
    } else {
        QDir saveDirectory = QDir(_worker.saveDirectory());
        if(!saveDirectory.exists()) {
            _errorMessage = tr("Cannot find the save directory");
            emit errorMessageChanged(_errorMessage);
            return;
        }
        saveDirectory.setFilter(QDir::Files | QDir::Readable | QDir::NoSymLinks | QDir::Writable);
        QStringList nameFilters;
        nameFilters << "*.jpg" << "*.JPG";
        saveDirectory.setNameFilters(nameFilters);
        QStringList imageList = saveDirectory.entryList();
        if(!imageList.isEmpty()) {
            QMessageBox msgBox(QMessageBox::Question,
                               tr("Save folder not empty."),
                               tr("The save folder already contains images. Do you want to replace them?"),
                               QMessageBox::Cancel);
            msgBox.setWindowModality(Qt::ApplicationModal);
            msgBox.addButton(tr("Replace"), QMessageBox::ActionRole);
            if (msgBox.exec() == QMessageBox::Cancel) {
                _errorMessage = tr("Save folder not empty");
                emit errorMessageChanged(_errorMessage);
                return;
            }
            foreach(QString dirFile, imageList)
            {
                if(!saveDirectory.remove(dirFile)) {
                    _errorMessage = tr("Couldn't replace the existing images");
                    emit errorMessageChanged(_errorMessage);
                    return;
                }
            }
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

GeoTagWorker::GeoTagWorker(void)
    : _cancel(false)
    , _logFile("")
    , _imageDirectory("")
    , _saveDirectory("")
{

}

void GeoTagWorker::run(void)
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
    if(isULog) {
        ULogParser parser;
        parseComplete = parser.getTagsFromLog(log, _triggerList);

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
            emit error(tr("Log parsing failed - tagging cancelled"));
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
    for(int i = 0; i < _imageTime.count() && i < _triggerList.count(); i++) {
        _imageIndices.append(i);
        _triggerIndices.append(i);
    }

    return true;
}
