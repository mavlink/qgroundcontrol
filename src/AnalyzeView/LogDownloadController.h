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
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(LogDownloadControllerLog)

struct LogDownloadData;
class QGCLogEntry;
class QmlObjectListModel;
class QTimer;
class QThread;
class Vehicle;
class LogDownloadTest;

class LogDownloadController : public QObject
{
    Q_OBJECT
    // QML_ELEMENT
    // QML_SINGLETON
    Q_MOC_INCLUDE("Vehicle.h")
    Q_MOC_INCLUDE("QmlObjectListModel.h")
    Q_PROPERTY(QmlObjectListModel *model          READ _getModel            CONSTANT)
    Q_PROPERTY(bool               requestingList  READ _getRequestingList   NOTIFY requestingListChanged)
    Q_PROPERTY(bool               downloadingLogs READ _getDownloadingLogs  NOTIFY downloadingLogsChanged)

    friend class LogDownloadTest;

public:
    explicit LogDownloadController(QObject *parent = nullptr);
    ~LogDownloadController();

    static LogDownloadController *instance();

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void download(const QString &path = QString());
    Q_INVOKABLE void eraseAll();
    Q_INVOKABLE void cancel();

signals:
    void requestingListChanged();
    void downloadingLogsChanged();
    void selectionChanged();

private slots:
    void _setActiveVehicle(Vehicle *vehicle);
    void _logEntry(uint32_t time_utc, uint32_t size, uint16_t id, uint16_t num_logs, uint16_t last_log_num);
    void _logData(uint32_t ofs, uint16_t id, uint8_t count, const uint8_t *data);
    void _processDownload();

private:
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

    QGCLogEntry *_getNextSelected() const;

    QTimer *_timer = nullptr;
    QmlObjectListModel *_logEntriesModel = nullptr;

    bool _downloadingLogs = false;
    bool _requestingLogEntries = false;
    int _apmOffset = 0;
    int _retries = 0;
    std::unique_ptr<LogDownloadData> _downloadData;
    QString _downloadPath;
    Vehicle *_vehicle = nullptr;

    static constexpr uint32_t kTimeOutMs = 500;
    static constexpr uint32_t kGUIRateMs = 17; ///< 1000ms / 60fps
    static constexpr uint32_t kRequestLogListTimeoutMs = 5000;
};
