/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

// TODO: https://github.com/PX4/PX4-Autopilot/blob/main/Tools/geotag_images_ulog.py

#include "GeoTagWorker.h"
#include "ExifParser.h"
#include "ULogParser.h"
#include "PX4LogParser.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QDir>

QGC_LOGGING_CATEGORY(GeoTagWorkerLog, "qgc.analyzeview.geotagworker")

GeoTagWorker::GeoTagWorker(QObject *parent)
    : QObject(parent)
{
    // qCDebug(GeoTagWorkerLog) << Q_FUNC_INFO << this;

#ifdef QT_DEBUG
    (void) connect(this, &GeoTagWorker::error, this, [](const QString &errorMsg) {
        qCDebug(GeoTagWorkerLog) << errorMsg;
    }, Qt::AutoConnection);
#endif
}

GeoTagWorker::~GeoTagWorker()
{
    // qCDebug(GeoTagWorkerLog) << Q_FUNC_INFO << this;
}

bool GeoTagWorker::process()
{
    _cancel = false;
    emit progressChanged(1);

    using StepFunction = bool (GeoTagWorker::*)();
    const StepFunction steps[] = {
        &GeoTagWorker::_loadImages,
        &GeoTagWorker::_parseExif,
        &GeoTagWorker::_parseLogs,
        &GeoTagWorker::_calibrate,
        &GeoTagWorker::_tagImages
    };

    for (StepFunction step : steps) {
        if (_cancel) {
            emit error(tr("Tagging cancelled"));
            return false;
        }

        if (!(this->*step)()) {
            return false;
        }
    }

    emit progressChanged(100);
    emit taggingComplete();

    return true;
}

bool GeoTagWorker::_loadImages()
{
    _imageList.clear();

    QDir imageDirectory = QDir(_imageDirectory);
    imageDirectory.setFilter(QDir::Files | QDir::Readable | QDir::NoSymLinks);
    imageDirectory.setSorting(QDir::Name);

    QStringList nameFilters;
    nameFilters << "*.jpg" << "*.JPG";
    imageDirectory.setNameFilters(nameFilters);

    _imageList = imageDirectory.entryInfoList();
    if (_imageList.isEmpty()) {
        emit error(tr("The image directory doesn't contain images, make sure your images are of the JPG format"));
        return false;
    }

    emit progressChanged(100. / kSteps);

    return true;
}

bool GeoTagWorker::_parseExif()
{
    _imageTimestamps.clear();

    // TODO: QtConcurrent::mapped?
    for (const QFileInfo& fileInfo : _imageList) {
        if (_cancel) {
            emit error(tr("Tagging cancelled"));
            return false;
        }

        QFile file(fileInfo.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly)) {
            emit error(tr("Geotagging failed. Couldn't open image: %1").arg(fileInfo.fileName()));
            return false;
        }

        const QByteArray imageBuffer = file.readAll();
        file.close();

        const QDateTime imageTime = ExifParser::readTime(imageBuffer);
        if (!imageTime.isValid()) {
            emit error(tr("Geotagging failed. Couldn't extract time from image: %1").arg(fileInfo.fileName()));
            return false;
        }

        (void) _imageTimestamps.append(imageTime.toSecsSinceEpoch());
    }

    emit progressChanged(2.0 * (100.0 / kSteps));

    return true;
}

bool GeoTagWorker::_parseLogs()
{
    _triggerList.clear();

    QFile file(_logFile);
    if (!file.open(QIODevice::ReadOnly)) {
        emit error(tr("Geotagging failed. Couldn't open log file."));
        return false;
    }

    const QByteArray log = file.readAll();
    file.close();

    bool parseComplete = false;
    QString errorString;
    if (_logFile.endsWith(".ulg", Qt::CaseSensitive)) {
        parseComplete = ULogParser::getTagsFromLog(log, _triggerList, errorString);
    } else {
        parseComplete = PX4LogParser::getTagsFromLog(log, _triggerList);
    }

    if (!parseComplete) {
        emit error(errorString.isEmpty() ? tr("Log parsing failed") : errorString);
        return false;
    }

    qCDebug(GeoTagWorkerLog) << "Found" << _triggerList.count() << "trigger logs.";
    if (_imageList.count() > _triggerList.count()) {
        qCDebug(GeoTagWorkerLog) << "Detected missing feedback packets.";
    } else if (_imageList.count() < _triggerList.count()) {
        qCDebug(GeoTagWorkerLog) << "Detected missing image frames.";
    }

    emit progressChanged(3. * (100. / kSteps));

    return true;
}

bool GeoTagWorker::_calibrate()
{
    _imageIndices.clear();
    _imageOffsets.clear();
    _triggerIndices.clear();

    if (_triggerList.isEmpty() || _imageTimestamps.isEmpty()) {
        emit error(tr("Calibration failed: No triggers or images available."));
        return false;
    }

    const qint64 lastImageTimestamp = _imageTimestamps.last();
    const qint64 lastTriggerTimestamp = _triggerList.last().timestamp;

    for (int i = 0; i < _imageTimestamps.size(); ++i) {
        const qint64 offset = lastImageTimestamp - _imageTimestamps[i];
        (void) _imageOffsets.insert(offset, i);
    }

    for (const CameraFeedbackPacket &trigger : _triggerList) {
        const qint64 triggerOffset = lastTriggerTimestamp - trigger.timestamp;
        if (_imageOffsets.contains(triggerOffset)) {
            const int imageIndex = _imageOffsets[triggerOffset];
            (void) _imageIndices.append(imageIndex);
            (void) _triggerIndices.append(&trigger - &_triggerList[0]);
        }
    }

    if (_imageIndices.isEmpty()) {
        emit error(tr("Calibration failed: No matching triggers found for images."));
        return false;
    }

    emit progressChanged(4. * (100. / kSteps));

    return true;
}

bool GeoTagWorker::_tagImages()
{
    const qsizetype maxIndex = std::min(_imageIndices.count(), _triggerIndices.count());
    for (int i = 0; i < maxIndex; i++) {
        if (_cancel) {
            emit error(tr("Tagging cancelled"));
            return false;
        }

        const int imageIndex = _imageIndices[i];
        if (imageIndex >= _imageList.count()) {
            emit error(tr("Geotagging failed. Requesting image #%1, but only %2 images present.").arg(imageIndex).arg(_imageList.count()));
            return false;
        }

        const QFileInfo &imageInfo = _imageList.at(imageIndex);
        QFile fileRead(imageInfo.absoluteFilePath());
        if (!fileRead.open(QIODevice::ReadOnly)) {
            emit error(tr("Geotagging failed. Couldn't open an image."));
            return false;
        }

        QByteArray imageBuffer = fileRead.readAll();
        fileRead.close();

        if (!ExifParser::write(imageBuffer, _triggerList[imageIndex])) {
            emit error(tr("Geotagging failed. Couldn't write to image: %1").arg(imageInfo.fileName()));
            return false;
        }

        QFile fileWrite;
        if (_saveDirectory.isEmpty()) {
            fileWrite.setFileName(_imageDirectory + "/TAGGED/" + imageInfo.fileName());
        } else {
            fileWrite.setFileName(_saveDirectory + "/" + imageInfo.fileName());
        }

        if (!fileWrite.open(QFile::WriteOnly)) {
            emit error(tr("Geotagging failed. Couldn't write to image: %1").arg(imageInfo.fileName()));
            return false;
        }

        fileWrite.write(imageBuffer);
        fileWrite.close();

        emit progressChanged(4. * (100. / kSteps) + ((100. / kSteps) / maxIndex) * i);
    }

    return true;
}
