#include "OnboardLogTransport.h"

#include <QtCore/QAbstractItemModel>
#include <QtCore/QDir>
#include <QtCore/QPointer>
#include <QtCore/QSignalBlocker>
#include <QtCore/QStorageInfo>
#include <algorithm>
#include <utility>

#include "OnboardLogEntry.h"
#include "OnboardLogModel.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

OnboardLogTransport::OnboardLogTransport(QObject* parent) : QObject(parent), _logEntriesModel(new OnboardLogModel(this))
{
    (void) connect(_logEntriesModel, &QAbstractItemModel::rowsInserted, this, &OnboardLogTransport::selectionChanged);
    (void) connect(_logEntriesModel, &QAbstractItemModel::rowsRemoved, this, &OnboardLogTransport::selectionChanged);
    (void) connect(_logEntriesModel, &QAbstractItemModel::modelReset, this, &OnboardLogTransport::selectionChanged);
}

OnboardLogTransport::~OnboardLogTransport() = default;

void OnboardLogTransport::_setOperation(Operation operation, Vehicle* vehicle)
{
    if (operation == Operation::Idle) {
        _communicationLostInhibitor.reset();
    } else if (!_communicationLostInhibitor && vehicle) {
        _communicationLostInhibitor =
            std::make_unique<CommunicationLostInhibitor>(vehicle->vehicleLinkManager()->inhibitCommunicationLost());
    }

    if (_operation == operation) {
        return;
    }

    const bool wasRequestingList = requestingList();
    const bool wasDownloading = downloadingLogs();
    const bool wasBusy = busy();
    _operation = operation;

    if (wasRequestingList != requestingList()) {
        emit requestingListChanged();
    }
    if (wasDownloading != downloadingLogs()) {
        emit downloadingLogsChanged();
    }
    if (wasBusy != busy()) {
        emit busyChanged();
    }
}

void OnboardLogTransport::BatchProgressState::reset()
{
    totalBytes = 0;
    completedBytes = 0;
    totalFiles = 0;
    completedFiles = 0;
}

void OnboardLogTransport::BatchProgressState::addFile(uint64_t bytes)
{
    totalBytes += bytes;
    ++totalFiles;
}

void OnboardLogTransport::BatchProgressState::completeFile(uint64_t bytes)
{
    completedBytes += bytes;
    ++completedFiles;
}

qreal OnboardLogTransport::BatchProgressState::progress(uint64_t currentBytes, qreal currentFileProgress) const
{
    if (totalBytes > 0) {
        return static_cast<qreal>(completedBytes + currentBytes) / static_cast<qreal>(totalBytes);
    }
    if (totalFiles > 0) {
        return (static_cast<qreal>(completedFiles) + currentFileProgress) / static_cast<qreal>(totalFiles);
    }
    return 0.;
}

void OnboardLogTransport::_setBatchProgress(qreal progress)
{
    progress = std::clamp(progress, 0., 1.);
    if (_batchProgress == progress) {
        return;
    }

    _batchProgress = progress;
    emit batchProgressChanged();
}

void OnboardLogTransport::_updateBatchProgress(uint64_t currentBytes, qreal currentFileProgress)
{
    _setBatchProgress(_batchState.progress(currentBytes, currentFileProgress));
}

void OnboardLogTransport::_setErrorMessage(const QString& errorMessage)
{
    if (_errorMessage == errorMessage) {
        return;
    }

    _errorMessage = errorMessage;
    emit errorMessageChanged();
}

void OnboardLogTransport::_setDownloading(Vehicle* vehicle, bool active)
{
    if (!active) {
        _setIdle(vehicle);
        return;
    }
    _setOperation(Operation::Downloading, vehicle);
}

void OnboardLogTransport::_setListing(Vehicle* vehicle, bool active)
{
    if (!active) {
        _setIdle(vehicle);
        return;
    }
    _setOperation(Operation::Listing, vehicle);
}

void OnboardLogTransport::_setIdle(Vehicle* vehicle)
{
    _setOperation(Operation::Idle, vehicle);
}

void OnboardLogTransport::_setEntryError(OnboardLogEntry* entry, const QString& errorMessage)
{
    QPointer<OnboardLogEntry> guardedEntry(entry);
    if (guardedEntry) {
        guardedEntry->setState(OnboardLogEntry::State::Error, tr("Error"));
        if (guardedEntry) {
            guardedEntry->setErrorMessage(errorMessage);
        }
    }
    _setErrorMessage(errorMessage);
}

void OnboardLogTransport::_resetSelection(bool canceled, const QString& skippedError)
{
    QList<QPointer<OnboardLogEntry>> selectedEntries;
    const int logCount = _logEntriesModel->count();
    selectedEntries.reserve(logCount);
    for (int index = 0; index < logCount; ++index) {
        OnboardLogEntry* const entry = _logEntriesModel->value<OnboardLogEntry*>(index);
        if (entry && entry->selected()) {
            selectedEntries.append(entry);
        }
    }

    if (!selectedEntries.isEmpty()) {
        const QSignalBlocker blocker(this);
        for (const QPointer<OnboardLogEntry>& entry : std::as_const(selectedEntries)) {
            if (!entry) {
                continue;
            }
            if (canceled) {
                entry->setState(OnboardLogEntry::State::Canceled, tr("Canceled"));
            } else if (!skippedError.isEmpty()) {
                entry->setState(OnboardLogEntry::State::Skipped, tr("Skipped"));
                if (entry) {
                    entry->setErrorMessage(skippedError);
                }
            }
            if (entry) {
                entry->setSelected(false);
            }
        }
    }

    emit selectionChanged();
}

bool OnboardLogTransport::_prepareDownload(Vehicle* vehicle, QString& directory, bool hasSelectedEntries,
                                           const QString& noSelectionError)
{
    if (!vehicle) {
        _setErrorMessage(tr("No active vehicle is available for onboard log download"));
        return false;
    }

    if (directory.isEmpty()) {
        _setErrorMessage(tr("No download directory was selected"));
        return false;
    }

    if (!hasSelectedEntries) {
        _setErrorMessage(noSelectionError);
        return false;
    }

    if (!directory.endsWith(QDir::separator())) {
        directory += QDir::separator();
    }

    const QStorageInfo storage(directory);
    const qint64 bytesAvailable = storage.bytesAvailable();
    if (storage.isValid() && storage.isReady() && (bytesAvailable >= 0) &&
        (_batchState.totalBytes > static_cast<uint64_t>(bytesAvailable))) {
        _setErrorMessage(tr("There is not enough free space to download the selected onboard logs"));
        return false;
    }

    return true;
}
