/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "GeoTagWorker.h"
#include "ExifParser.h"
#include "ULogParser.h"
#include "PX4LogParser.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QDir>

QGC_LOGGING_CATEGORY(GeoTagWorkerLog, "qgc.analyzeview.geotagworker")

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
    _imageTime.clear();
    for (int i = 0; i < _imageList.size(); ++i) {
        QFile file(_imageList.at(i).absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly)) {
            emit error(tr("Geotagging failed. Couldn't open an image."));
            return;
        }
        QByteArray imageBuffer = file.readAll();
        file.close();

        _imageTime.append(ExifParser::readTime(imageBuffer));

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
    auto maxIndex = std::min(_imageIndices.count(), _triggerIndices.count());
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

        if (!ExifParser::write(imageBuffer, _triggerList[_triggerIndices[i]])) {
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
