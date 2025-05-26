/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

class GeoTagWorker;
class QThread;

Q_DECLARE_LOGGING_CATEGORY(GeoTagControllerLog)

/// Controller for GeoTagPage.qml. Supports geotagging images based on logfile camera tags.
class GeoTagController : public QObject
{
    Q_OBJECT
    // QML_ELEMENT

    Q_PROPERTY(QString  logFile         READ logFile        WRITE setLogFile        NOTIFY logFileChanged)
    Q_PROPERTY(QString  imageDirectory  READ imageDirectory WRITE setImageDirectory NOTIFY imageDirectoryChanged)
    Q_PROPERTY(QString  saveDirectory   READ saveDirectory  WRITE setSaveDirectory  NOTIFY saveDirectoryChanged)
    Q_PROPERTY(QString  errorMessage    READ errorMessage                           NOTIFY errorMessageChanged)
    Q_PROPERTY(double   progress        READ progress                               NOTIFY progressChanged)
    Q_PROPERTY(bool     inProgress      READ inProgress                             NOTIFY inProgressChanged)

public:
    explicit GeoTagController(QObject *parent = nullptr);
    ~GeoTagController();

    Q_INVOKABLE void startTagging();
    Q_INVOKABLE void cancelTagging();

    QString logFile() const;
    QString imageDirectory() const;
    QString saveDirectory() const;

    /// Progress indicator: 0-100
    double progress() const { return _progress; }

    /// true: Currently in the process of tagging
    bool inProgress() const;

    /// Set to an error message if geotagging fails
    QString errorMessage() const { return _errorMessage; }

    void setLogFile(const QString &file);
    void setImageDirectory(const QString &dir);
    void setSaveDirectory(const QString &dir);

signals:
    void logFileChanged(const QString &logFile);
    void imageDirectoryChanged(const QString &imageDirectory);
    void saveDirectoryChanged(const QString &saveDirectory);
    void progressChanged(double progress);
    void inProgressChanged();
    void errorMessageChanged(const QString &errorMessage);

private slots:
    void _workerProgressChanged(double progress) { if (progress != _progress) { _progress = progress; emit progressChanged(_progress); } }
    void _setErrorMessage(const QString &errorMsg) { if (errorMsg != _errorMessage) { _errorMessage = errorMsg; emit errorMessageChanged(_errorMessage); } }
    void _workerError(const QString &errorMsg) { _setErrorMessage(errorMsg); }

private:
    QString _errorMessage;
    double _progress = 0.;
    bool _inProgress = false;
    GeoTagWorker *_worker = nullptr;
    QThread *_workerThread = nullptr;

    static constexpr const char *kTagged = "/TAGGED";
};
