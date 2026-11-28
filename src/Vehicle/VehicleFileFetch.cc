#include "VehicleFileFetch.h"

#include <utility>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>

#include "FTPManager.h"
#include "QGCCachedFileDownload.h"
#include "QGCFileDownload.h"
#include "QGCFileHelper.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

QGC_LOGGING_CATEGORY(VehicleFileFetchLog, "Vehicle.VehicleFileFetch")

VehicleFileFetch* VehicleFileFetch::start(QObject* context, Vehicle* vehicle, const QString& uri,
                                          const Options& options, Callback done)
{
    qCDebug(VehicleFileFetchLog) << "Fetching" << uri << "to" << options.directory << options.fileName;

    if (!context) {
        qCWarning(VehicleFileFetchLog) << "Cannot fetch" << uri << "without a context";
        return nullptr;
    }
    // An empty path would make QDir fall back to the current working directory
    if (options.directory.isEmpty() || !QFileInfo(options.directory).isDir()) {
        qCWarning(VehicleFileFetchLog) << "Cannot fetch" << uri
                                       << "- the directory is not set or does not exist:" << options.directory;
        return nullptr;
    }

    const bool ftp = isMavlinkFtp(uri);
    const QUrl url = ftp ? QUrl() : httpUrl(uri);
    if (!ftp && url.isEmpty()) {
        qCWarning(VehicleFileFetchLog) << "Cannot fetch" << uri << "- only MAVLink FTP and HTTP(S) are fetched";
        return nullptr;
    }

    auto* const fetch = new VehicleFileFetch(context, std::move(done));
    const bool running = ftp ? fetch->_startFtp(vehicle, uri, options) : fetch->_startHttp(url, options);
    if (!running) {
        fetch->_end();
        delete fetch;
        return nullptr;
    }
    return fetch;
}

bool VehicleFileFetch::isMavlinkFtp(const QString& uri)
{
    return uri.startsWith(QStringLiteral("%1://").arg(FTPManager::mavlinkFTPScheme), Qt::CaseInsensitive);
}

QUrl VehicleFileFetch::httpUrl(const QString& uri)
{
    const QString trimmed = uri.trimmed();
    // A path on this computer, or a Qt resource, whether or not QUrl::fromUserInput() would take it for a file. What
    // starts like a path is refused unseen: looking up a path of another host (UNC) on Windows would already ask that
    // host
    if (trimmed.isEmpty() || QStringLiteral("/\\.~:").contains(trimmed.front()) || QFileInfo::exists(trimmed)) {
        return {};
    }

    // "host:port/..." is no scheme
    static const QRegularExpression schemePattern(QStringLiteral("^([A-Za-z][A-Za-z0-9+.-]*):(?!\\d+(/|$))"));
    const QRegularExpressionMatch scheme = schemePattern.match(trimmed);
    QUrl url;
    if (!scheme.hasMatch()) {
        url = QUrl(QStringLiteral("http://") + trimmed, QUrl::TolerantMode);
    } else if ((scheme.captured(1).compare(QLatin1String("http"), Qt::CaseInsensitive) == 0) ||
               (scheme.captured(1).compare(QLatin1String("https"), Qt::CaseInsensitive) == 0)) {
        url = QUrl(trimmed, QUrl::TolerantMode);
        url.setScheme(scheme.captured(1).toLower());
    }
    if (!url.isValid() || url.host().isEmpty()) {
        return {};
    }
    return url;
}

VehicleFileFetch::VehicleFileFetch(QObject* context, Callback done)
    : QObject(context)
    , _done(std::move(done))
{}

VehicleFileFetch::~VehicleFileFetch()
{
    _end();
}

void VehicleFileFetch::cancel()
{
    _end();
    deleteLater();
}

bool VehicleFileFetch::_startFtp(Vehicle* vehicle, const QString& uri, const Options& options)
{
    if (!vehicle) {
        qCWarning(VehicleFileFetchLog) << "Cannot fetch" << uri << "without a vehicle";
        return false;
    }

    _ftpRequest = vehicle->ftpManager()->download(
        this, options.ftpComponentId, uri, options.directory,
        {.fileName = options.fileName, .maxBytes = options.maxBytes},
        [this](const QString& file, const QString& errorMsg) { _finish(file, errorMsg); });
    if (!_ftpRequest) {
        return false;
    }

    (void) connect(_ftpRequest, &FTPRequest::started, this, &VehicleFileFetch::started);
    (void) connect(_ftpRequest, &FTPRequest::progressChanged, this, &VehicleFileFetch::progressChanged);
    return true;
}

bool VehicleFileFetch::_startHttp(const QUrl& url, const Options& options)
{
    const QString uri = url.toString();

    // The name, also one that the URL gives, names the file only, never where it goes. A name that cannot be used is
    // refused, as over MAVLink FTP
    const QString givenName = options.fileName.isEmpty() ? url.fileName() : options.fileName;
    const QString fileName = QGCFileHelper::localFileName(givenName);
    if (fileName.isEmpty()) {
        qCWarning(VehicleFileFetchLog) << "Cannot fetch" << uri
                                       << "- not a file name that every system writes as it is:" << givenName;
        return false;
    }
    const QString localPath = QDir(options.directory).filePath(fileName);
    if (!QGCFileHelper::isInDirectory(QDir::cleanPath(QDir(options.directory).absolutePath()),
                                      QDir::cleanPath(QFileInfo(localPath).absoluteFilePath()))) {
        qCWarning(VehicleFileFetchLog) << "Cannot fetch" << uri << "- the file would be outside" << options.directory;
        return false;
    }

    if (options.httpCacheDirectory.isEmpty()) {
        _download = new QGCFileDownload(this);
    } else {
        _cachedDownload = new QGCCachedFileDownload(options.httpCacheDirectory, this);
        _download = _cachedDownload->fileDownloader();
    }
    _download->setMaxBytes(options.maxBytes);
    _download->setVerifyPeer(options.verifyPeer);
    _download->setOutputPath(localPath);

    const auto finished = [this](bool success, const QString& localFile, const QString& errorMsg) {
        if (success) {
            _finish(localFile, QString());
        } else {
            _finish(QString(), errorMsg.isEmpty() ? tr("Download failed") : errorMsg);
        }
    };
    const auto progress = [this](qreal value) { emit progressChanged(static_cast<float>(value)); };

    bool running = false;
    if (_cachedDownload) {
        (void) connect(_cachedDownload, &QGCCachedFileDownload::finished, this,
                       [finished](bool success, const QString& localFile, const QString& errorMsg, bool) {
                           finished(success, localFile, errorMsg);
                       });
        (void) connect(_cachedDownload, &QGCCachedFileDownload::progressChanged, this, progress);
        running = _cachedDownload->download(uri, options.httpCacheMaxAgeSec);
    } else {
        (void) connect(_download, &QGCFileDownload::finished, this, finished);
        (void) connect(_download, &QGCFileDownload::progressChanged, this, progress);
        running = _download->start(uri);
    }
    if (!running) {
        qCWarning(VehicleFileFetchLog) << "Cannot fetch" << uri << "-" << _download->errorString();
        return false;
    }

    // From the event loop, as over MAVLink FTP, so that the caller can connect to it
    (void) QMetaObject::invokeMethod(
        this,
        [this]() {
            if (!_ended) {
                emit started();
            }
        },
        Qt::QueuedConnection);
    return true;
}

void VehicleFileFetch::_finish(const QString& localFile, const QString& errorMsg)
{
    if (_ended) {
        return;
    }
    _ended = true;

    qCDebug(VehicleFileFetchLog) << "Fetched" << localFile << errorMsg;

    if (_download) {
        _download->disconnect(this);
        if (_cachedDownload) {
            _cachedDownload->disconnect(this);
        }
        if (!errorMsg.isEmpty()) {
            _removeHttpOutput();
        }
    }

    // Not deleted now: this runs in a slot of the downloader, which is a child of the fetch
    deleteLater();
    const Callback done = std::exchange(_done, nullptr);
    if (done) {
        done(errorMsg.isEmpty() ? localFile : QString(), errorMsg);
    }
}

void VehicleFileFetch::_end()
{
    if (_ended) {
        return;
    }
    _ended = true;
    _done = nullptr;

    if (_ftpRequest) {
        // Removes the part of the file that has arrived
        _ftpRequest->cancel();
    }
    if (_download) {
        _download->disconnect(this);
        if (_cachedDownload) {
            _cachedDownload->disconnect(this);
            _cachedDownload->cancel();
        } else {
            _download->cancel();
        }
        _removeHttpOutput();
    }
}

void VehicleFileFetch::_removeHttpOutput() const
{
    const QString localPath = _download->localPath();
    if (!localPath.isEmpty()) {
        (void) QFile::remove(localPath);
    }
}
