#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <cstdint>
#include <memory>

class CommunicationLostInhibitor;
class OnboardLogModel;
class OnboardLogEntry;
class Vehicle;

/// Common contract for an onboard-log transport (LOG protocol or MAVLink FTP).
/// Concrete subclasses (LogProtocolTransport, FtpTransport) are owned by
/// OnboardLogController, which selects one per active vehicle and provides the
/// QML-facing facade.
class OnboardLogTransport : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(OnboardLogTransport)

public:
    enum class ListingResult : uint8_t
    {
        Success,
        Partial,
        Failed,
        Canceled,
    };
    Q_ENUM(ListingResult)

    ~OnboardLogTransport() override;

    OnboardLogModel* model() const { return _logEntriesModel; }

    bool requestingList() const { return _operation == Operation::Listing; }

    bool downloadingLogs() const { return _operation == Operation::Downloading; }

    bool busy() const { return _operation != Operation::Idle; }

    qreal batchProgress() const { return _batchProgress; }

    QString errorMessage() const { return _errorMessage; }

    virtual void setVehicle(Vehicle* vehicle) = 0;

    virtual void refresh() = 0;
    virtual void download(const QString& path = QString()) = 0;
    virtual void cancel() = 0;

    virtual void eraseAll() = 0;

signals:
    void listingFinished(ListingResult result);
    void requestingListChanged();
    void downloadingLogsChanged();
    void busyChanged();
    void batchProgressChanged();
    void errorMessageChanged();
    void selectionChanged();

protected:
    enum class Operation : uint8_t
    {
        Idle,
        Listing,
        Downloading,
        Erasing,
    };

    struct BatchProgressState
    {
        void reset();
        void addFile(uint64_t bytes);
        void completeFile(uint64_t bytes);
        qreal progress(uint64_t currentBytes = 0, qreal currentFileProgress = 0.) const;

        bool hasFiles() const { return totalFiles > 0; }

        uint64_t totalBytes = 0;
        uint64_t completedBytes = 0;
        uint totalFiles = 0;
        uint completedFiles = 0;
    };

    explicit OnboardLogTransport(QObject* parent = nullptr);

    void _setBatchProgress(qreal progress);
    void _updateBatchProgress(uint64_t currentBytes = 0, qreal currentFileProgress = 0.);
    void _setErrorMessage(const QString& errorMessage);
    void _setOperation(Operation operation, Vehicle* vehicle);
    void _setIdle(Vehicle* vehicle);
    void _setDownloading(Vehicle* vehicle, bool active);
    void _setListing(Vehicle* vehicle, bool active);
    void _setEntryError(OnboardLogEntry* entry, const QString& errorMessage);
    void _resetSelection(bool canceled = false, const QString& skippedError = QString());
    bool _prepareDownload(Vehicle* vehicle, QString& directory, bool hasSelectedEntries,
                          const QString& noSelectionError);

    OnboardLogModel* _logEntriesModel = nullptr;
    Operation _operation = Operation::Idle;
    BatchProgressState _batchState;
    qreal _batchProgress = 0.;
    QString _errorMessage;
    std::unique_ptr<CommunicationLostInhibitor> _communicationLostInhibitor;
};
