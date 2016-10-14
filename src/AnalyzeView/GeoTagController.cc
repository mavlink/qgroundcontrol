/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "GeoTagController.h"
#include "QGCFileDialog.h"
#include "ExifParser.h"
#include <QtEndian>
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
    QString filename = QGCFileDialog::getOpenFileName(NULL, "Select log file load", QString(), "PX4 log file (*.px4log);;All Files (*.*)");
    if (!filename.isEmpty()) {
        _worker.setLogFile(filename);
        emit logFileChanged(filename);
    }
}

void GeoTagController::pickImageDirectory(void)
{
    QString dir = QGCFileDialog::getExistingDirectory(NULL, "Select image directory");
    if (!dir.isEmpty()) {
        _worker.setImageDirectory(dir);
        emit imageDirectoryChanged(dir);
    }
}

void GeoTagController::startTagging(void)
{
    _errorMessage.clear();
    emit errorMessageChanged(_errorMessage);
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
{

}

void GeoTagWorker::run(void)
{
    _cancel = false;
    emit progressChanged(0);

    //used to time operations to get a feel for how much to progress the progressBar
    QElapsedTimer timerTotal;
    QElapsedTimer timerLoadImages;
    QElapsedTimer timerParseExif;
    QElapsedTimer timerFilter;
    QElapsedTimer timerLoadLogFile;
    QElapsedTimer timerGeotag;

    timerTotal.start();


    //////////// Load Images
    timerLoadImages.start();

    QDir imageDirectory = QDir(_imageDirectory);
    if(!imageDirectory.exists()) {
        emit error(tr("Cannot find the image directory"));
        return;
    }
    if(!imageDirectory.mkdir(_imageDirectory + "/TAGGED")) {
        emit error(tr("Images have already been tagged"));
        return;
    }

    imageDirectory.setFilter(QDir::Files | QDir::Readable | QDir::NoSymLinks | QDir::Writable);
    imageDirectory.setSorting(QDir::Name);
    QStringList nameFilters;
    nameFilters << "*.jpg" << "*.JPG";
    imageDirectory.setNameFilters(nameFilters);

    QFileInfoList imageList = imageDirectory.entryInfoList();
    if(imageList.isEmpty()) {
        emit error(tr("The image directory doesn't contain images, make sure your images are of the JPG format"));
        return;
    }

    _imageBuffers.clear();
    for (int i = 0; i < imageList.size(); ++i) {
        QFile file(imageList.at(i).absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly)) {
            continue;
        }
        _imageBuffers.append(QByteArray(file.readAll()));
        file.close();
    }

    qWarning() << "Image loading time elapsed: " << timerLoadImages.elapsed() << " milliseconds";

    ////////// Parse exif data
    timerParseExif.start();

    // Parse EXIF
    ExifParser exifParser;
    _tagTime.clear();
    for (int i = 0; i < _imageBuffers.count(); i++) {
         _tagTime.append(exifParser.readTime(_imageBuffers[i]));
    }

    qWarning() << "Parse exif data time elapsed: " << timerParseExif.elapsed() << " milliseconds";

    ////////// Load PX4 log
    timerLoadLogFile.start();

    _geoRef.clear();
    _triggerTime.clear();
    if (!parsePX4Log()) {
        qWarning() << "Geotagging failed";
        return;
    }
    qWarning() << "Found " << _geoRef.count() << " trigger logs.";

    qWarning() << "Log loading time elapsed: " << timerLoadLogFile.elapsed() << " milliseconds";

    ////////// Filter Trigger
    timerFilter.start();

    if (!triggerFiltering()) {
        qWarning() << "Geotagging failed";
        return;
    }

    qWarning() << "Filter time elapsed: " << timerFilter.elapsed() << " milliseconds";

    //////////// Tag images
    timerGeotag.start();

    for(int i = 0; i < _imageIndices.count() && i < _triggerIndices.count() && i < imageList.count(); i++) {
        if (!exifParser.write(_imageBuffers[_imageIndices[i]], _geoRef[_triggerIndices[i]])) {
            _cancel = true;
            break;
        } else {
            QFile file(_imageDirectory + "/TAGGED/" + imageList[_imageIndices[i]].fileName());
            if (file.open( QFile::WriteOnly)) {
                file.write(_imageBuffers[_imageIndices[i]]);
                file.close();
            }
        }
    }

    qWarning() << "Tagging images time elapsed: " << timerGeotag.elapsed() << " milliseconds";

    for (int i=0; i<10;i++) {
        if (_cancel) {
            emit error(tr("Tagging cancelled"));
            return;
        }
        emit progressChanged(i*10);
        //sleep(1);
    }

    qWarning() << "Total time elapsed: " << timerTotal.elapsed() << " milliseconds";
    emit progressChanged(100);
    emit taggingComplete();
}

bool GeoTagWorker::parsePX4Log()
{
    // general message header
    // char header[] = {0xA3, 0x95, 0x00};
    // header for GPOS message
    char gposHeader[] = {0xA3, 0x95, 0x10, 0x00};
    int gposOffsets[3] = {3, 7, 11};
    int gposLengths[3] = {4, 4, 4};
    // header for trigger message
    char triggerHeader[] = {0xA3, 0x95, 0x37, 0x00};
    int triggerOffsets[2] = {3, 11};
    int triggerLengths[2] = {8, 4};
    // load log
    QFile file(_logFile);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open log file";
        return false;
    }
    QByteArray log = file.readAll();
    file.close();

    // extract trigger data
    int index = 1;
    int sequence = -1;
    QGeoCoordinate lastCoordinate;
    while(index < log.count() - 1) {
        int gposIndex = log.indexOf(gposHeader, index + 1);
        int triggerIndex = log.indexOf(triggerHeader, index + 1);
        // check for whether last entry has been passed
        if ((gposIndex < 0 && triggerIndex < 0) || (gposIndex >= log.count() - 1 && triggerIndex >= log.count() - 1)) {
            break;
        } else if (gposIndex < 0) {
            gposIndex = triggerIndex + 1;
        } else if (triggerIndex < 0) {
            triggerIndex = gposIndex + 1;
        }
        // extract next entry, gpos or trigger
        if (gposIndex < triggerIndex) {
            // TODO: somehow verify that the gposIndex is really the header of a gpos message
            int32_t* lat = reinterpret_cast<int32_t*>(log.mid(gposIndex + gposOffsets[0], gposLengths[0]).data());
            double latitude = static_cast<double>(qFromLittleEndian(*lat))/1.0e7;
            lastCoordinate.setLatitude(latitude);
            int32_t* lon = reinterpret_cast<int32_t*>(log.mid(gposIndex + gposOffsets[1], gposLengths[1]).data());
            double longitude = static_cast<double>(qFromLittleEndian(*lon))/1.0e7;
            longitude = fmod(180.0 + longitude, 360.0) - 180.0;
            lastCoordinate.setLongitude(longitude);
            float* alt = reinterpret_cast<float*>(log.mid(gposIndex + gposOffsets[2], gposLengths[2]).data());
            lastCoordinate.setAltitude(qFromLittleEndian(*alt));
            index = gposIndex;
        } else {
            uint64_t* time = reinterpret_cast<uint64_t*>(log.mid(triggerIndex + triggerOffsets[0], triggerLengths[0]).data());
            double timeDouble = static_cast<double>(qFromLittleEndian(*time));
            uint32_t* seq = reinterpret_cast<uint32_t*>(log.mid(triggerIndex + triggerOffsets[1], triggerLengths[1]).data());
            int seqInt = static_cast<int>(qFromLittleEndian(*seq));
            if (sequence < seqInt && sequence + 20 > seqInt) { // assume that logging has not skipped more than 20 triggers. this prevents wrong header detection
               _geoRef.append(lastCoordinate);
               _triggerTime.append(timeDouble/1000000.0);
               sequence = seqInt;
            }
            index = triggerIndex;
        }
    }
    return true;
}

bool GeoTagWorker::triggerFiltering()
{
    for (int i = 0; i < _triggerTime.count() && i < _tagTime.count(); i++) {
        _triggerIndices.append(i);
        _imagesIndices.append(i);
    }
    return true;
}
