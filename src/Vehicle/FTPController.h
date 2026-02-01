#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtQmlIntegration/QtQmlIntegration>

#include "MAVLink/MAVLinkLib.h"
#include "QGCLoggingCategory.h"


class FTPManager;
class Vehicle;
class QGCArchiveModel;
class QGCCompressionJob;

/// QML-facing controller for MAVLink FTP operations.
class FTPController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_MOC_INCLUDE("Vehicle.h")

    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool listInProgress READ listInProgress NOTIFY activeOperationChanged)
    Q_PROPERTY(bool downloadInProgress READ downloadInProgress NOTIFY activeOperationChanged)
    Q_PROPERTY(bool uploadInProgress READ uploadInProgress NOTIFY activeOperationChanged)
    Q_PROPERTY(bool deleteInProgress READ deleteInProgress NOTIFY activeOperationChanged)
    Q_PROPERTY(float progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
    Q_PROPERTY(QString currentPath READ currentPath NOTIFY currentPathChanged)
    Q_PROPERTY(QStringList directoryEntries READ directoryEntries NOTIFY directoryEntriesChanged)
    Q_PROPERTY(QString lastDownloadFile READ lastDownloadFile NOTIFY lastDownloadFileChanged)
    Q_PROPERTY(QString lastUploadTarget READ lastUploadTarget NOTIFY lastUploadTargetChanged)
    Q_PROPERTY(bool lastDownloadIsArchive READ lastDownloadIsArchive NOTIFY lastDownloadFileChanged)
    Q_PROPERTY(bool extracting READ extracting NOTIFY extractingChanged)
    Q_PROPERTY(float extractionProgress READ extractionProgress NOTIFY extractionProgressChanged)
    Q_PROPERTY(QGCArchiveModel* archiveModel READ archiveModel CONSTANT)

public:
    explicit FTPController(QObject *parent = nullptr);

    bool busy() const { return _busy; }
    bool listInProgress() const { return _operation == Operation::List; }
    bool downloadInProgress() const { return _operation == Operation::Download; }
    bool uploadInProgress() const { return _operation == Operation::Upload; }
    bool deleteInProgress() const { return _operation == Operation::Delete; }
    float progress() const { return _progress; }
    QString errorString() const { return _errorString; }
    QString currentPath() const { return _currentPath; }
    QStringList directoryEntries() const { return _directoryEntries; }
    QString lastDownloadFile() const { return _lastDownloadFile; }
    QString lastUploadTarget() const { return _lastUploadTarget; }
    bool lastDownloadIsArchive() const { return _lastDownloadIsArchive; }
    bool extracting() const { return _extracting; }
    float extractionProgress() const { return _extractionProgress; }
    QGCArchiveModel* archiveModel() const { return _archiveModel; }

    Q_INVOKABLE bool listDirectory(const QString &uri, int componentId = MAV_COMP_ID_AUTOPILOT1);
    Q_INVOKABLE bool downloadFile(const QString &uri, const QString &localDir, const QString &fileName = QString(), int componentId = MAV_COMP_ID_AUTOPILOT1);
    Q_INVOKABLE bool uploadFile(const QString &localFile, const QString &uri, int componentId = MAV_COMP_ID_AUTOPILOT1);
    Q_INVOKABLE bool deleteFile(const QString &uri, int componentId = MAV_COMP_ID_AUTOPILOT1);
    Q_INVOKABLE void cancelActiveOperation();

    /// Browse the contents of a downloaded archive file
    /// @param archivePath Path to the archive file (use lastDownloadFile after download)
    /// @return true if archive was opened successfully
    Q_INVOKABLE bool browseArchive(const QString &archivePath);

    /// Extract a downloaded archive to the specified directory
    /// @param archivePath Path to the archive file
    /// @param outputDir Directory to extract to (empty = same as archive location)
    /// @return true if extraction started successfully
    Q_INVOKABLE bool extractArchive(const QString &archivePath, const QString &outputDir = QString());

    /// Cancel an in-progress extraction
    Q_INVOKABLE void cancelExtraction();

signals:
    void busyChanged();
    void activeOperationChanged();
    void progressChanged();
    void errorStringChanged();
    void currentPathChanged();
    void directoryEntriesChanged();
    void lastDownloadFileChanged();
    void lastUploadTargetChanged();
    void downloadComplete(const QString &filePath, const QString &error);
    void uploadComplete(const QString &remotePath, const QString &error);
    void deleteComplete(const QString &remotePath, const QString &error);
    void extractingChanged();
    void extractionProgressChanged();
    void extractionComplete(const QString &outputDir, const QString &error);

private slots:
    void _handleDownloadComplete(const QString &filePath, const QString &error);
    void _handleUploadComplete(const QString &remotePath, const QString &error);
    void _handleDirectoryComplete(const QStringList &entries, const QString &error);
    void _handleDeleteComplete(const QString &remotePath, const QString &error);
    void _handleCommandProgress(float value);
    void _handleExtractionProgress(qreal progress);
    void _handleExtractionFinished(bool success);

private:
    enum class Operation {
        None,
        List,
        Download,
        Upload,
        Delete,
    };

    void _setBusy(bool busy);
    void _setOperation(Operation operation);
    void _clearOperation();
    void _setErrorString(const QString &error);
    void _resetDirectoryState();
    uint8_t _componentIdForRequest(int componentId) const;

    Vehicle *_vehicle = nullptr;
    FTPManager *_ftpManager = nullptr;
    Operation _operation = Operation::None;
    bool _busy = false;
    float _progress = 0.0F;
    QString _errorString;
    QString _currentPath;
    QStringList _directoryEntries;
    QString _lastDownloadFile;
    QString _lastUploadTarget;
    bool _lastDownloadIsArchive = false;
    bool _extracting = false;
    float _extractionProgress = 0.0F;
    QString _extractionOutputDir;
    QGCArchiveModel *_archiveModel = nullptr;
    QGCCompressionJob *_extractionJob = nullptr;
};
