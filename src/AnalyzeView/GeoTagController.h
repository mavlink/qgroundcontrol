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

#include <QObject>
#include <QString>
#include <QThread>

class GeoTagWorker : public QThread
{
    Q_OBJECT

public:
    GeoTagWorker(void);

    QString logFile(void) const { return _logFile; }
    QString imageDirectory(void) const { return _imageDirectory; }

    void setLogFile(const QString& logFile) { _logFile = logFile; }
    void setImageDirectory(const QString& imageDirectory) { _imageDirectory = imageDirectory; }

    void cancellTagging(void) { _cancel = true; }

protected:
    void run(void) final;

signals:
    void error(QString errorMsg);
    void taggingComplete(void);
    void progressChanged(double progress);

private:
    bool    _cancel;
    QString _logFile;
    QString _imageDirectory;
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

    /// Set to an error message is geotagging fails
    Q_PROPERTY(QString  errorMessage    READ errorMessage   NOTIFY errorMessageChanged)

    /// Progress indicator: 0-100
    Q_PROPERTY(double   progress        READ progress       NOTIFY progressChanged)

    /// true: Currently in the process of tagging
    Q_PROPERTY(bool     inProgress      READ inProgress     NOTIFY inProgressChanged)

    Q_INVOKABLE void pickLogFile(void);
    Q_INVOKABLE void pickImageDirectory(void);
    Q_INVOKABLE void startTagging(void);
    Q_INVOKABLE void cancelTagging(void) { _worker.cancellTagging(); }

    QString logFile         (void) const { return _worker.logFile(); }
    QString imageDirectory  (void) const { return _worker.imageDirectory(); }
    double  progress        (void) const { return _progress; }
    bool    inProgress      (void) const { return _worker.isRunning(); }
    QString errorMessage    (void) const { return _errorMessage; }

signals:
    void logFileChanged(QString logFile);
    void imageDirectoryChanged(QString imageDirectory);
    void progressChanged(double progress);
    void inProgressChanged(void);
    void errorMessageChanged(QString errorMessage);

private slots:
    void _workerProgressChanged(double progress);
    void _workerError(QString errorMsg);

private:
    QString _errorMessage;
    double  _progress;
    bool    _inProgress;

    GeoTagWorker _worker;
};

#endif
