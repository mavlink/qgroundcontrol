/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef GeoTagController_H
#define GeoTagController_H

#include "QmlObjectListModel.h"
#include "Fact.h"
#include "FactMetaData.h"
#include <QObject>
#include <QString>
#include <QThread>
#include <QFileInfoList>
#include <QElapsedTimer>
#include <QDebug>
#include <QGeoCoordinate>

class GeoTagWorker : public QThread
{
    Q_OBJECT

public:
    GeoTagWorker(void);

    void setLogFile         (const QString& logFile)        { _logFile = logFile; }
    void setImageDirectory  (const QString& imageDirectory) { _imageDirectory = imageDirectory; }
    void setSaveDirectory   (const QString& saveDirectory)  { _saveDirectory = saveDirectory; }

    QString logFile         (void) const { return _logFile; }
    QString imageDirectory  (void) const { return _imageDirectory; }
    QString saveDirectory   (void) const { return _saveDirectory; }

    void cancelTagging      (void) { _cancel = true; }

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
    void run(void) final;

signals:
    void error              (QString errorMsg);
    void taggingComplete    (void);
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

/// Controller for GeoTagPage.qml. Supports geotagging images based on logfile camera tags.
class GeoTagController : public QObject
{
    Q_OBJECT
    
public:
    GeoTagController(void);
    ~GeoTagController();
    
    Q_PROPERTY(QString  logFile         READ logFile        NOTIFY logFileChanged)
    Q_PROPERTY(QString  imageDirectory  READ imageDirectory NOTIFY imageDirectoryChanged)
    Q_PROPERTY(QString  saveDirectory   READ saveDirectory  NOTIFY saveDirectoryChanged)

    /// Set to an error message is geotagging fails
    Q_PROPERTY(QString  errorMessage    READ errorMessage   NOTIFY errorMessageChanged)

    /// Progress indicator: 0-100
    Q_PROPERTY(double   progress        READ progress       NOTIFY progressChanged)

    /// true: Currently in the process of tagging
    Q_PROPERTY(bool     inProgress      READ inProgress     NOTIFY inProgressChanged)

    Q_INVOKABLE void pickLogFile(void);
    Q_INVOKABLE void pickImageDirectory(void);
    Q_INVOKABLE void pickSaveDirectory(void);
    Q_INVOKABLE void startTagging(void);
    Q_INVOKABLE void cancelTagging(void) { _worker.cancelTagging(); }

    QString logFile             (void) const { return _worker.logFile(); }
    QString imageDirectory      (void) const { return _worker.imageDirectory(); }
    QString saveDirectory       (void) const { return _worker.saveDirectory(); }
    double  progress            (void) const { return _progress; }
    bool    inProgress          (void) const { return _worker.isRunning(); }
    QString errorMessage        (void) const { return _errorMessage; }

signals:
    void logFileChanged                 (QString logFile);
    void imageDirectoryChanged          (QString imageDirectory);
    void saveDirectoryChanged           (QString saveDirectory);
    void progressChanged                (double progress);
    void inProgressChanged              (void);
    void errorMessageChanged            (QString errorMessage);

private slots:
    void _workerProgressChanged (double progress);
    void _workerError           (QString errorMsg);
    void _setErrorMessage       (const QString& error);

private:
    QString             _errorMessage;
    double              _progress;
    bool                _inProgress;

    GeoTagWorker        _worker;
};

#endif
