/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QThread>
#include <QtCore/QFileInfoList>
#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(GeoTagWorkerLog)

class GeoTagWorker : public QThread
{
    Q_OBJECT

public:
    GeoTagWorker();

    void setLogFile         (const QString& logFile)        { _logFile = logFile; }
    void setImageDirectory  (const QString& imageDirectory) { _imageDirectory = imageDirectory; }
    void setSaveDirectory   (const QString& saveDirectory)  { _saveDirectory = saveDirectory; }

    QString logFile         () const { return _logFile; }
    QString imageDirectory  () const { return _imageDirectory; }
    QString saveDirectory   () const { return _saveDirectory; }

    void cancelTagging      () { _cancel = true; }

    struct cameraFeedbackPacket {
        double timestamp;
        double timestampUTC;
        uint32_t imageSequence;
        double latitude;
        double longitude;
        float altitude;
        float groundDistance;
        float attitudeQuaternion[4];
        uint8_t captureResult;
    };

protected:
    void run() final;

signals:
    void error              (QString errorMsg);
    void taggingComplete    ();
    void progressChanged    (double progress);

private:
    bool triggerFiltering();

    bool                    _cancel;
    QString                 _logFile;
    QString                 _imageDirectory;
    QString                 _saveDirectory;
    QFileInfoList           _imageList;
    QList<double>           _imageTime;
    QList<cameraFeedbackPacket> _triggerList;
    QList<int>              _imageIndices;
    QList<int>              _triggerIndices;
};
