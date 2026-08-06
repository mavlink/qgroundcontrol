#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QObject>
#include <QtCore/QQueue>
#include <QtCore/QStringList>
#include <QtQmlIntegration/QtQmlIntegration>

struct OnboardLogDownloadData;
class QGCOnboardLogEntry;
class QmlObjectListModel;
class QTimer;
class QThread;
class Vehicle;
class OnboardLogDownloadTest;
class OnboardLogFtpDownloadTest;

class OnboardLogController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_MOC_INCLUDE("Vehicle.h")
    Q_MOC_INCLUDE("QmlObjectListModel.h")

    Q_PROPERTY(QmlObjectListModel *model               READ _getModel                                           CONSTANT)
    Q_PROPERTY(bool               requestingList       READ _getRequestingList                                  NOTIFY requestingListChanged)
    Q_PROPERTY(bool               downloadingLogs      READ _getDownloadingLogs                                 NOTIFY downloadingLogsChanged)
    Q_PROPERTY(bool               allLogsSelected      READ allLogsSelected                                     NOTIFY selectionChanged)
    Q_PROPERTY(int                selectedCount        READ selectedCount                                       NOTIFY selectionChanged)
    Q_PROPERTY(bool               sortAscending        READ sortAscending          WRITE setSortAscending       NOTIFY sortAscendingChanged)
    Q_PROPERTY(bool               compressLogs         READ compressLogs           WRITE setCompressLogs        NOTIFY compressLogsChanged)
    Q_PROPERTY(bool               compressing          READ compressing                                         NOTIFY compressingChanged)
    Q_PROPERTY(float              compressionProgress  READ compressionProgress                                 NOTIFY compressionProgressChanged)
    Q_PROPERTY(QString            transport            READ transport                                           NOTIFY transportChanged)

    friend class OnboardLogDownloadTest;
    friend class OnboardLogFtpDownloadTest;

public:
    explicit OnboardLogController(QObject *parent = nullptr);
    ~OnboardLogController();

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void download(const QString &path = QString());
    Q_INVOKABLE void eraseAll();
    Q_INVOKABLE void eraseSelected();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void selectAll(bool select);
    int selectedCount() const;
    Q_INVOKABLE void toggleSortByDate();

    bool compressLogs() const { return _compressLogs; }
    void setCompressLogs(bool compress);
    bool compressing() const { return _compressing; }
    float compressionProgress() const { return _compressionProgress; }
    bool allLogsSelected() const;
    bool sortAscending() const { return _sortAscending; }
    void setSortAscending(bool ascending);

    /// Transport currently used to list/download logs: "messages" (LOG_* messages) or "ftp" (MAVLink FTP)
    QString transport() const { return (_transport == Transport::Ftp) ? QStringLiteral("ftp") : QStringLiteral("messages"); }

    /// Compress a single log file
    Q_INVOKABLE bool compressLogFile(const QString &logPath);

    /// Cancel compression
    Q_INVOKABLE void cancelCompression();

signals:
    void requestingListChanged();
    void downloadingLogsChanged();
    void transportChanged();
    void selectionChanged();
    void compressLogsChanged();
    void sortAscendingChanged();
    void compressingChanged();
    void compressionProgressChanged();
    void compressionComplete(const QString &outputPath, const QString &error);

private slots:
    void _setActiveVehicle(Vehicle *vehicle);

    void _logEntry(uint32_t time_utc, uint32_t size, uint16_t id, uint16_t num_logs, uint16_t last_log_num);
    void _logData(uint32_t ofs, uint16_t id, uint8_t count, const uint8_t *data);
    void _processDownload();
    void _handleCompressionProgress(qreal progress);
    void _handleCompressionFinished(bool success);

    void _ftpListDirComplete(const QStringList &dirList, const QString &errorMsg);
    void _ftpDownloadComplete(const QString &file, const QString &errorMsg);
    void _ftpDownloadProgress(float value);
    void _ftpDeleteComplete(const QString &file, const QString &errorMsg);

private:
    enum class Transport { Messages, Ftp };
    enum class FtpListState { Idle, ListingRoot, ListingSubdir };

    QmlObjectListModel *_getModel() const { return _logEntriesModel; }
    bool _getRequestingList() const { return _requestingLogEntries; }
    bool _getDownloadingLogs() const { return _downloadingLogs; }

    bool _chunkComplete() const;
    bool _entriesComplete() const;
    bool _logComplete() const;
    bool _prepareLogDownload();
    void _downloadToDirectory(const QString &dir);
    void _findMissingData();
    void _findMissingEntries();
    void _receivedAllData();
    void _receivedAllEntries();
    void _requestLogData(uint16_t id, uint32_t offset, uint32_t count, int retryCount = 0);
    void _requestLogList(uint32_t start, uint32_t end);
    void _requestLogEnd();
    void _resetSelection(bool canceled = false);
    void _setDownloading(bool active);
    void _setListing(bool active);
    void _updateDataRate();

    void _sortEntriesByTimestamp();

    void _setTransport(Transport transport);
    void _ftpStartListing();
    void _ftpListRoot();
    void _ftpListNextSubdir();
    uint _ftpProcessFileEntries(const QStringList &dirList, const QString &subdir);
    void _ftpFinishListing();
    void _ftpFallbackToMessages();
    void _ftpDownloadToDirectory(const QString &dir);
    void _ftpDownloadEntry(QGCOnboardLogEntry *entry);
    void _ftpDownloadQueueNext();
    void _ftpDeleteNext();

    QGCOnboardLogEntry *_getNextSelected() const;

    QTimer *_timer = nullptr;
    QmlObjectListModel *_logEntriesModel = nullptr;

    bool _downloadingLogs = false;
    bool _requestingLogEntries = false;
    int _apmOffset = 0;
    int _retries = 0;
    std::unique_ptr<OnboardLogDownloadData> _downloadData;
    QString _downloadPath;
    Vehicle *_vehicle = nullptr;
    bool _compressLogs = false;
    bool _compressing = false;
    float _compressionProgress = 0.0F;
    bool _sortAscending = false;

    Transport _transport = Transport::Messages;
    bool _ftpDisabled = false;    ///< Set after an FTP failure; subsequent refreshes use the message transport
    FtpListState _ftpListState = FtpListState::Idle;
    QString _ftpLogRoot;
    bool _ftpTriedFallbackRoot = false;
    QStringList _ftpDirsToList;
    uint _ftpLogIdCounter = 0;
    QQueue<QGCOnboardLogEntry*> _ftpDownloadQueue;
    QQueue<QGCOnboardLogEntry*> _ftpDeleteQueue;
    bool _ftpDeleting = false;    ///< A selective erase is in progress
    QGCOnboardLogEntry *_ftpCurrentDownloadEntry = nullptr;
    bool _ftpDownloadHadError = false;
    QElapsedTimer _ftpDownloadElapsed;
    size_t _ftpDownloadBytesAtLastUpdate = 0;
    qreal _ftpDownloadRateAvg = 0.;

    static constexpr uint32_t kTimeOutMs = 500;
    static constexpr uint32_t kGUIRateMs = 500; ///< Update download rate twice per second
    static constexpr uint32_t kRequestLogListTimeoutMs = 5000;
};
