#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QQueue>

#include "OnboardLogTransport.h"

class OnboardLogEntry;
class Vehicle;

class FtpTransport : public OnboardLogTransport
{
    Q_OBJECT
    Q_MOC_INCLUDE("Vehicle.h")

public:
    explicit FtpTransport(QObject* parent = nullptr);
    ~FtpTransport();

    bool canErase() const override { return true; }

    QString transportName() const override { return QStringLiteral("ftp"); }

    Q_INVOKABLE void refresh() override;
    Q_INVOKABLE void download(const QString& path = QString()) override;
    Q_INVOKABLE void eraseAll() override;
    Q_INVOKABLE void cancel() override;

private slots:
    void _setActiveVehicle(Vehicle* vehicle);
    void _listDirComplete(const QStringList& dirList, const QString& errorMsg);
    void _downloadComplete(const QString& file, const QString& errorMsg);
    void _downloadProgress(float value);
    void _deleteComplete(const QString& file, const QString& errorMsg);

private:
    enum ListState
    {
        Idle,
        ListingRoot,
        ListingSubdir
    };

    void _startListing();
    void _listRoot();
    void _listNextSubdir();
    uint _processFileEntries(const QStringList& dirList, const QString& subdir);
    void _downloadEntry(OnboardLogEntry* entry);
    void _downloadToDirectory(const QString& dir);
    void _resetSelection(bool canceled = false);
    void _setDownloading(bool active);
    void _setListing(bool active);
    void _finishListing();
    void _eraseNext();
    void _finishErase();

    Vehicle* _vehicle = nullptr;

    ListState _listState = Idle;
    QString _logRoot;
    bool _triedFallbackRoot = false;
    QStringList _dirsToList;
    uint _logIdCounter = 0;

    QQueue<OnboardLogEntry*> _downloadQueue;
    OnboardLogEntry* _currentDownloadEntry = nullptr;
    QString _downloadPath;
    QElapsedTimer _downloadElapsed;
    size_t _downloadBytesAtLastUpdate = 0;
    qreal _downloadRateAvg = 0.;

    bool _erasing = false;
    QQueue<OnboardLogEntry*> _eraseQueue;
    OnboardLogEntry* _currentEraseEntry = nullptr;
    uint _eraseFailureCount = 0;

    static constexpr uint32_t kGUIRateMs = 17;
};
