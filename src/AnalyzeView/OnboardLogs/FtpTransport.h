#pragma once

#include <QtCore/QHash>
#include <QtCore/QPersistentModelIndex>
#include <QtCore/QPointer>
#include <chrono>

#include "FtpDownloadSession.h"
#include "FtpEraseSession.h"
#include "FtpListingSession.h"
#include "OnboardLogTransport.h"

class OnboardLogEntry;
class FTPManager;
class FTPDeleteJob;
class FTPDownloadJob;
class FTPListDirectoryJob;
class Vehicle;

class FtpTransport : public OnboardLogTransport
{
    Q_OBJECT

#ifdef QGC_UNITTEST_BUILD
    friend class OnboardLogDownloadTest;
#endif

public:
    explicit FtpTransport(QObject* parent = nullptr);
    ~FtpTransport() override;

    void setVehicle(Vehicle* vehicle) override;

    void refresh() override;
    void download(const QString& path = QString()) override;
    void eraseAll() override;
    void cancel() override;

private slots:
    void _listDirComplete(const QStringList& dirList, const QString& errorMsg, bool truncated);
    void _downloadComplete(const QString& file, const QString& errorMsg, const QString& warningMsg = QString());
    void _downloadProgress(float value);
    void _deleteComplete(const QString& file, const QString& errorMsg);

private:
    void _startListing();
    void _listRoot();
    void _listNextSubdir();
    uint _processFileEntries(const QStringList& dirList, const QString& subdir);
    void _downloadNext();
    void _scheduleDownloadNext();
    void _downloadEntry(OnboardLogEntry* entry);
    void _downloadToDirectory(const QString& dir);
    void _refreshAfterCancel();
    void _cancelListing(FTPManager* ftpManager);
    void _cancelDownload(FTPManager* ftpManager);
    void _finishDownloadCancellation();
    void _cancelErase(FTPManager* ftpManager);
    void _setDownloading(bool active);
    void _setListing(bool active);
    void _finishListing(ListingResult result);
    void _eraseNext();
    void _scheduleEraseNext();
    void _finishErase();
    bool _eraseEntryIsCurrent(const QPointer<OnboardLogEntry>& entry) const;
    static QString _localFilenameForEntry(const OnboardLogEntry& entry);

    QPointer<Vehicle> _vehicle;

    FtpListingSession _listingSession;
    FtpDownloadSession _downloadSession;
    FtpEraseSession _eraseSession;
    QPointer<FTPListDirectoryJob> _listingJob;
    QPointer<FTPDownloadJob> _downloadJob;
    QPointer<FTPDeleteJob> _deleteJob;
    QHash<const OnboardLogEntry*, QPersistentModelIndex> _eraseEntryIndexes;

    static constexpr std::chrono::milliseconds kGuiUpdateInterval{100};
};
