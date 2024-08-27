/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

GeoTagWorker::GeoTagWorker(QObject *parent)
    : QObject(parent)
{
    // qCDebug(GeoTagWorkerLog) << Q_FUNC_INFO << this;

    (void) connect(this, &GeoTagWorker::error, this, [](const QString &errorMsg) {
        qCDebug(GeoTagWorkerLog) << errorMsg;
    }, Qt::AutoConnection);
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
        &GeoTagWorker::_initParser,
        &GeoTagWorker::_triggerFiltering,
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
    _imageTime.clear();

    for (int i = 0; i < _imageList.size(); ++i) {
        QFile file(_imageList.at(i).absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly)) {
            emit error(tr("Geotagging failed. Couldn't open an image."));
            return false;
        }

        const QByteArray imageBuffer = file.readAll();
        file.close();

        (void) _imageTime.append(ExifParser::readTime(imageBuffer));

        emit progressChanged((100. / kSteps) + ((100. / kSteps) / _imageList.size()) * i);

        if (_cancel) {
            emit error(tr("Tagging cancelled"));
            return false;
        }
    }

    return true;
}

bool GeoTagWorker::_initParser()
{
    QFile file(_logFile);
    if (!file.open(QIODevice::ReadOnly)) {
        emit error(tr("Geotagging failed. Couldn't open log file."));
        return false;
    }

    const QByteArray log = file.readAll();
    file.close();

    _triggerList.clear();

    bool parseComplete = false;
    QString errorString;
    if (_logFile.endsWith(".ulg", Qt::CaseSensitive)) {
        parseComplete = ULogParser::getTagsFromLog(log, _triggerList, errorString);
    } else {
        parseComplete = PX4LogParser::getTagsFromLog(log, _triggerList);
    }

    if (!parseComplete) {
        if (_cancel) {
            emit error(tr("Tagging cancelled"));
            return false;
        } else {
            errorString = tr("%1 - tagging cancelled").arg(errorString.isEmpty() ? tr("Log parsing failed") : errorString);
            emit error(errorString);
            return false;
        }
    }

    qCDebug(GeoTagWorkerLog) << "Found" << _triggerList.count() << "trigger logs.";

    emit progressChanged(3. * (100. / kSteps));

    return true;
}

bool GeoTagWorker::_triggerFiltering()
{
    _imageIndices.clear();
    _triggerIndices.clear();

    if (_imageList.count() > _triggerList.count()) {
        qCDebug(GeoTagWorkerLog) << "Detected missing feedback packets.";
    } else if (_imageList.count() < _triggerList.count()) {
        qCDebug(GeoTagWorkerLog) << "Detected missing image frames.";
    }

    // TODO: handle _triggerList does not start at 0, causes issue in _tagImages loop counter
    if (_triggerList.first().imageSequence != 0) {
        qCDebug(GeoTagWorkerLog) << "Image sequence does not start at beginning.";
    }

    for (int i = 0; i < std::min(_imageList.count(), _triggerList.count()); i++) {
        (void) _imageIndices.append(static_cast<int>(_triggerList[i].imageSequence));
        (void) _triggerIndices.append(i);
    }

    emit progressChanged(4. * (100. / kSteps));

    return true;
}

bool GeoTagWorker::_tagImages()
{
    const qsizetype maxIndex = std::min(_imageList.count(), std::min(_imageIndices.count(), _triggerIndices.count()));
    for (int i = 0; i < maxIndex; i++) {
        const int imageIndex = _imageIndices[i];
        if (imageIndex >= _imageList.count()) {
            emit error(tr("Geotagging failed. Requesting image #%1, but only %2 images present.").arg(imageIndex).arg(_imageList.count()));
            return false;
        }

        const QFileInfo imageInfo = _imageList.at(imageIndex);
        QFile fileRead(imageInfo.absoluteFilePath());
        if (!fileRead.open(QIODevice::ReadOnly)) {
            emit error(tr("Geotagging failed. Couldn't open an image."));
            return false;
        }

        QByteArray imageBuffer = fileRead.readAll();
        fileRead.close();

        if (!ExifParser::write(imageBuffer, _triggerList[imageIndex])) {
            emit error(tr("Geotagging failed. Couldn't write to image."));
            return false;
        }

        QFile fileWrite;
        if (_saveDirectory.isEmpty()) {
            fileWrite.setFileName(_imageDirectory + "/TAGGED/" + imageInfo.fileName());
        } else {
            fileWrite.setFileName(_saveDirectory + "/" + imageInfo.fileName());
        }

        if (!fileWrite.open(QFile::WriteOnly)) {
            emit error(tr("Geotagging failed. Couldn't write to an image."));
            return false;
        }

        fileWrite.write(imageBuffer);
        fileWrite.close();

        emit progressChanged(4. * (100. / kSteps) + ((100. / kSteps) / maxIndex) * i);

        if (_cancel) {
            emit error(tr("Tagging cancelled"));
            return false;
        }
    }

    return true;
}
