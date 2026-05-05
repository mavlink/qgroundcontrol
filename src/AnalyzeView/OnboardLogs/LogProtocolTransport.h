#pragma once

#include <QtCore/QPointer>
#include <chrono>
#include <cstdint>

#include "LogProtocolDownloadBatchSession.h"
#include "LogProtocolListingSession.h"
#include "OnboardLogTransport.h"

class OnboardLogEntry;
class QTimer;
class Vehicle;

class LogProtocolTransport : public OnboardLogTransport
{
    Q_OBJECT

#ifdef QGC_UNITTEST_BUILD
    friend class OnboardLogDownloadTest;
#endif

public:
    explicit LogProtocolTransport(QObject* parent = nullptr);
    ~LogProtocolTransport() override;

    void setVehicle(Vehicle* vehicle) override;

    void refresh() override;
    void download(const QString& path = QString()) override;
    void eraseAll() override;
    void cancel() override;

private slots:
    void _logEntry(uint32_t time_utc, uint32_t size, uint16_t id, uint16_t num_logs, uint16_t last_log_num);
    void _logData(uint32_t ofs, uint16_t id, uint8_t count, const uint8_t* data);
    void _processDownload();

private:
    bool _prepareLogDownload();
    void _downloadToDirectory(const QString& dir);
    void _failDownload(const QString& reason);
    void _findMissingData();
    void _findMissingEntries();
    void _removePartialDownload();
    void _receivedAllData();
    void _receivedAllEntries(ListingResult result = ListingResult::Success);
    bool _requestLogData(uint16_t id, uint32_t offset, uint32_t count, int retryCount = 0);
    void _requestLogList(uint32_t start, uint32_t end);
    void _requestLogEnd();
    void _setDownloading(bool active);
    void _setListing(bool active);
    bool _updateDataRate(uint32_t byteThreshold = 102400);

    QTimer* _timer = nullptr;

    LogProtocolListingSession _listingSession;
    LogProtocolDownloadBatchSession _downloadBatch;
    QPointer<Vehicle> _vehicle;

    static constexpr std::chrono::milliseconds kTimeout{500};
    static constexpr std::chrono::milliseconds kGuiUpdateInterval{500};  ///< Update download rate twice per second
    static constexpr std::chrono::milliseconds kRequestLogListTimeout{5000};
    static constexpr int kMaxDownloadRetries = 5;
    static constexpr int kMaxLogEntries = LogProtocolListingSession::kMaxLogEntries;
};
