#pragma once

#include "OnboardLogTransport.h"

struct OnboardLogDownloadData;
class OnboardLogEntry;
class QTimer;
class Vehicle;

class LogProtocolTransport : public OnboardLogTransport
{
    Q_OBJECT
    Q_MOC_INCLUDE("Vehicle.h")
    Q_PROPERTY(bool compressLogs READ compressLogs WRITE setCompressLogs NOTIFY compressLogsChanged)
    Q_PROPERTY(bool compressing READ compressing NOTIFY compressingChanged)
    Q_PROPERTY(float compressionProgress READ compressionProgress NOTIFY compressionProgressChanged)

public:
    explicit LogProtocolTransport(QObject* parent = nullptr);
    ~LogProtocolTransport();

    bool canErase() const override { return true; }

    QString transportName() const override { return QStringLiteral("log"); }

    Q_INVOKABLE void refresh() override;
    Q_INVOKABLE void download(const QString& path = QString()) override;
    Q_INVOKABLE void eraseAll() override;
    Q_INVOKABLE void cancel() override;

    bool compressLogs() const { return _compressLogs; }

    void setCompressLogs(bool compress);

    bool compressing() const { return _compressing; }

    float compressionProgress() const { return _compressionProgress; }

    /// Compress a single log file
    Q_INVOKABLE bool compressLogFile(const QString& logPath);

    /// Cancel compression
    Q_INVOKABLE void cancelCompression();

signals:
    void compressLogsChanged();
    void compressingChanged();
    void compressionProgressChanged();
    void compressionComplete(const QString& outputPath, const QString& error);

private slots:
    void _setActiveVehicle(Vehicle* vehicle);
    void _logEntry(uint32_t time_utc, uint32_t size, uint16_t id, uint16_t num_logs, uint16_t last_log_num);
    void _logData(uint32_t ofs, uint16_t id, uint8_t count, const uint8_t* data);
    void _processDownload();
    void _handleCompressionProgress(qreal progress);
    void _handleCompressionFinished(bool success);

private:
    bool _chunkComplete() const;
    bool _entriesComplete() const;
    bool _logComplete() const;
    bool _prepareLogDownload();
    void _downloadToDirectory(const QString& dir);
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

    OnboardLogEntry* _getNextSelected() const;

    QTimer* _timer = nullptr;

    int _apmOffset = 0;
    int _retries = 0;
    std::unique_ptr<OnboardLogDownloadData> _downloadData;
    QString _downloadPath;
    Vehicle* _vehicle = nullptr;
    bool _compressLogs = false;
    bool _compressing = false;
    float _compressionProgress = 0.0F;

    static constexpr uint32_t kTimeOutMs = 500;
    static constexpr uint32_t kGUIRateMs = 500;  ///< Update download rate twice per second
    static constexpr uint32_t kRequestLogListTimeoutMs = 5000;
};
