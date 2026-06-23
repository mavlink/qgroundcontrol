#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QQueue>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(OnboardLogFtpControllerLog)

class QGCOnboardLogFtpEntry;
class QmlObjectListModel;
class Vehicle;

class OnboardLogFtpController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_MOC_INCLUDE("Vehicle.h")
    Q_MOC_INCLUDE("QmlObjectListModel.h")
    Q_PROPERTY(QmlObjectListModel *model          READ _getModel            CONSTANT)
    Q_PROPERTY(bool               requestingList  READ _getRequestingList   NOTIFY requestingListChanged)
    Q_PROPERTY(bool               downloadingLogs READ _getDownloadingLogs  NOTIFY downloadingLogsChanged)

public:
    explicit OnboardLogFtpController(QObject *parent = nullptr);
    ~OnboardLogFtpController();

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void download(const QString &path = QString());
    Q_INVOKABLE void cancel();

signals:
    void requestingListChanged();
    void downloadingLogsChanged();
    void selectionChanged();

private slots:
    void _setActiveVehicle(Vehicle *vehicle);
    void _listDirComplete(const QStringList &dirList, const QString &errorMsg);
    void _downloadComplete(const QString &file, const QString &errorMsg);
    void _downloadProgress(float value);

private:
    enum ListState { Idle, ListingRoot, ListingSubdir };

    QmlObjectListModel *_getModel() const { return _logEntriesModel; }
    bool _getRequestingList() const { return _requestingLogEntries; }
    bool _getDownloadingLogs() const { return _downloadingLogs; }

    void _startListing();
    void _listRoot();
    void _listNextSubdir();
    uint _processFileEntries(const QStringList &dirList, const QString &subdir);
    void _downloadEntry(QGCOnboardLogFtpEntry *entry);
    void _downloadToDirectory(const QString &dir);
    void _resetSelection(bool canceled = false);
    void _setDownloading(bool active);
    void _setListing(bool active);
    void _finishListing();

    QmlObjectListModel *_logEntriesModel = nullptr;
    Vehicle *_vehicle = nullptr;

    bool _requestingLogEntries = false;
    bool _downloadingLogs = false;

    ListState _listState = Idle;
    QString _logRoot;
    bool _triedFallbackRoot = false;
    QStringList _dirsToList;
    uint _logIdCounter = 0;

    QQueue<QGCOnboardLogFtpEntry*> _downloadQueue;
    QGCOnboardLogFtpEntry *_currentDownloadEntry = nullptr;
    QString _downloadPath;
    QElapsedTimer _downloadElapsed;
    size_t _downloadBytesAtLastUpdate = 0;
    qreal _downloadRateAvg = 0.;

    static constexpr uint32_t kGUIRateMs = 17;
};
