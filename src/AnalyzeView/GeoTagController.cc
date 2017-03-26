/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "GeoTagController.h"
#include "ExifParser.h"
#include "QGCQFileDialog.h"
#include "QGCLoggingCategory.h"
#include "MainWindow.h"
#include <math.h>
#include <QtEndian>
#include <QMessageBox>
#include <QDebug>
#include <cfloat>

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
    QString filename = QGCQFileDialog::getOpenFileName(MainWindow::instance(), "Select log file load", QString(), "PX4 log file (*.px4log);;All Files (*.*)");
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
    _tagTime.clear();
    for (int i = 0; i < _imageList.size(); ++i) {
        QFile file(_imageList.at(i).absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly)) {
            emit error(tr("Geotagging failed. Couldn't open an image."));
            return;
        }
        QByteArray imageBuffer = file.readAll();
        file.close();

        _tagTime.append(exifParser.readTime(imageBuffer));

        emit progressChanged((100/nSteps) + ((100/nSteps) / _imageList.size())*i);

        if (_cancel) {
            qCDebug(GeotaggingLog) << "Tagging cancelled";
            emit error(tr("Tagging cancelled"));
            return;
        }
    }

    // Load PX4 log
    _geoRef.clear();
    _triggerTime.clear();
    if (!parsePX4Log()) {
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

    qCDebug(GeotaggingLog) << "Found " << _geoRef.count() << " trigger logs.";

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

        if (!exifParser.write(imageBuffer, _geoRef[_triggerIndices[i]])) {
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

bool GeoTagWorker::parsePX4Log()
{
    // general message header
    char header[] = {(char)0xA3, (char)0x95, (char)0x00};
    // header for GPOS message header
    char gposHeaderHeader[] = {(char)0xA3, (char)0x95, (char)0x80, (char)0x10, (char)0x00};
    int gposHeaderOffset;
    // header for GPOS message
    char gposHeader[] = {(char)0xA3, (char)0x95, (char)0x10, (char)0x00};
    int gposOffsets[3] = {3, 7, 11};
    int gposLengths[3] = {4, 4, 4};
    // header for trigger message header
    char triggerHeaderHeader[] = {(char)0xA3, (char)0x95, (char)0x80, (char)0x37, (char)0x00};
    int triggerHeaderOffset;
    // header for trigger message
    char triggerHeader[] = {(char)0xA3, (char)0x95, (char)0x37, (char)0x00};
    int triggerOffsets[2] = {3, 11};
    int triggerLengths[2] = {8, 4};

    // load log
    QFile file(_logFile);
    if (!file.open(QIODevice::ReadOnly)) {
        qCDebug(GeotaggingLog) << "Could not open log file " << _logFile;
        return false;
    }
    QByteArray log = file.readAll();
    file.close();

    // extract header information: message lengths
    uint8_t* iptr = reinterpret_cast<uint8_t*>(log.mid(log.indexOf(gposHeaderHeader) + 4, 1).data());
    gposHeaderOffset = static_cast<int>(qFromLittleEndian(*iptr));
    iptr = reinterpret_cast<uint8_t*>(log.mid(log.indexOf(triggerHeaderHeader) + 4, 1).data());
    triggerHeaderOffset = static_cast<int>(qFromLittleEndian(*iptr));

    // extract trigger data
    int index = 1;
    int sequence = -1;
    QGeoCoordinate lastCoordinate;
    while(index < log.count() - 1) {

        if (_cancel) {
            return false;
        }

        // first extract trigger
        index = log.indexOf(triggerHeader, index + 1);
        // check for whether last entry has been passed
        if (index < 0) {
            break;
        }

        if (log.indexOf(header, index + 1) != index + triggerHeaderOffset) {
            continue;
        }
        uint64_t* time = reinterpret_cast<uint64_t*>(log.mid(index + triggerOffsets[0], triggerLengths[0]).data());
        double timeDouble = static_cast<double>(qFromLittleEndian(*time)) / 1.0e6;
        uint32_t* seq = reinterpret_cast<uint32_t*>(log.mid(index + triggerOffsets[1], triggerLengths[1]).data());
        int seqInt = static_cast<int>(qFromLittleEndian(*seq));
        if (sequence >= seqInt || sequence + 20 < seqInt) { // assume that logging has not skipped more than 20 triggers. this prevents wrong header detection
            continue;
        }
        _triggerTime.append(timeDouble);
        sequence = seqInt;

        // second extract position
        bool lookForGpos = true;
        while (lookForGpos) {

            if (_cancel) {
                return false;
            }

            int gposIndex = log.indexOf(gposHeader, index + 1);
            if (gposIndex < 0) {
                _geoRef.append(lastCoordinate);
                break;
            }
            index = gposIndex;
            // verify that at an offset of gposHeaderOffset the next log message starts
            if (gposIndex + gposHeaderOffset == log.indexOf(header, gposIndex + 1)) {
                int32_t* lat = reinterpret_cast<int32_t*>(log.mid(gposIndex + gposOffsets[0], gposLengths[0]).data());
                double latitude = static_cast<double>(qFromLittleEndian(*lat))/1.0e7;
                lastCoordinate.setLatitude(latitude);
                int32_t* lon = reinterpret_cast<int32_t*>(log.mid(gposIndex + gposOffsets[1], gposLengths[1]).data());
                double longitude = static_cast<double>(qFromLittleEndian(*lon))/1.0e7;
                longitude = fmod(180.0 + longitude, 360.0) - 180.0;
                lastCoordinate.setLongitude(longitude);
                float* alt = reinterpret_cast<float*>(log.mid(gposIndex + gposOffsets[2], gposLengths[2]).data());
                lastCoordinate.setAltitude(qFromLittleEndian(*alt));
                _geoRef.append(lastCoordinate);
                break;
            }
        }
    }
    return true;
}

bool GeoTagWorker::triggerFiltering()
{
    _imageIndices.clear();
    _triggerIndices.clear();
    for(int i = 0; i < _tagTime.count() && i < _triggerTime.count(); i++) {
        _imageIndices.append(i);
        _triggerIndices.append(i);
    }

    return true;
}
