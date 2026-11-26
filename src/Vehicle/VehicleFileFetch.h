#pragma once

#include <functional>

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QUrl>

#include "MAVLinkEnums.h"

class FTPRequest;
class QGCCachedFileDownload;
class QGCFileDownload;
class Vehicle;

/// Fetches a file that a vehicle points to by its URI, such as its component metadata or a camera definition, into a
/// local file: a "mftp://" URI over MAVLink FTP from the vehicle, or an HTTP(S) URL. Nothing else is fetched, see
/// httpUrl(): a URI that a vehicle gives must not make QGC read its own disk.
///
/// A fetch is started by start() and is running when it returns. It is a child of the context given to it, calls its
/// callback once, with the local file or an error, and deletes itself once it has ended. cancel(), deleting the fetch
/// and deleting the context all end it the same way: the callback is not called. A fetch that fails or is ended leaves
/// no file behind; the file it fetched belongs to the caller.
///
/// @code
/// _fetch = VehicleFileFetch::start(this, vehicle, uri, {.directory = directory, .maxBytes = maxBytes},
///                                  [this](const QString& file, const QString& errorMsg) { ... });
/// // Later, if the file is no longer wanted. _fetch is a QPointer: the fetch deletes itself
/// if (_fetch) {
///     _fetch->cancel();
/// }
/// @endcode
class VehicleFileFetch : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(VehicleFileFetch)

public:
    /// Largest size of a file that a vehicle sends or points to and that QGC reads whole, decompressed if it is
    /// compressed: component metadata, its translations, events metadata and camera definitions (64 MiB). These are
    /// a few MiB at most, so a far larger file is not parsed. The fetch of such a file is held to it too
    /// (Options::maxBytes): a compressed file is not larger than its content
    static constexpr qint64 kMaxSmallFileBytes = 64LL * 1024 * 1024;

    struct Options
    {
        /// Directory the file is written to. It must exist.
        QString directory{};
        /// Name of the file in the directory, empty for the last component of the URI. Only its last component is
        /// used, after a slash or a backslash: a name that every system would not write as it is
        /// (QGCFileHelper::localFileName()) is not fetched.
        QString fileName{};
        /// Largest size of the file as it is transferred, 0 = unlimited. A larger file fails the fetch.
        qint64 maxBytes = 0;
        /// Component of the vehicle that a MAVLink FTP URI addresses unless it names one ("mftp://[;comp=<id>]...")
        uint8_t ftpComponentId = MAV_COMP_ID_AUTOPILOT1;
        /// HTTP: directory of the network disk cache, empty for none. A cached file no older than httpCacheMaxAgeSec
        /// (0 = of any age) is taken without asking the server, an older one only if the server cannot be reached.
        QString httpCacheDirectory{};
        int httpCacheMaxAgeSec = 0;
        /// HTTPS: false accepts any server certificate, also one that cannot be verified, such as a self-signed one
        bool verifyPeer = true;
    };

    /// Called with the local file and an empty error message, or with an empty file and the error
    using Callback = std::function<void(const QString& localFile, const QString& errorMsg)>;

    /// @param vehicle Needed for a MAVLink FTP URI only
    /// @return The fetch, running, or nullptr if it cannot start: the URI is neither MAVLink FTP nor HTTP(S), the
    ///         directory or, for MAVLink FTP, the vehicle is missing, or the URI or the file name cannot be used. A
    ///         warning says why, and nothing is called back. Hold it in a QPointer to cancel it.
    static VehicleFileFetch* start(QObject* context, Vehicle* vehicle, const QString& uri, const Options& options,
                                   Callback done);

    /// @return true if @a uri is fetched over MAVLink FTP
    static bool isMavlinkFtp(const QString& uri);

    /// The HTTP(S) URL that a URI given by a vehicle stands for. The scheme is taken in any case, and a URI without
    /// one that names a host is taken as HTTP, as QUrl::fromUserInput() does.
    /// @return The URL, or an empty one if the URI may not be fetched over HTTP: a path on this computer, absolute or
    ///         relative that exists, a Qt resource, a "file:" URL or any other scheme
    static QUrl httpUrl(const QString& uri);

    /// Ends the fetch without calling back
    ~VehicleFileFetch() override;

    /// Ends the fetch without calling back, as deleting it does, and deletes it later
    void cancel();

signals:
    /// The transfer begins. Over MAVLink FTP, the transfers of the vehicle queued before it have ended.
    void started();

    /// Progress of the transfer, from 0.0 to 1.0, while it is known
    void progressChanged(float progress);

private:
    VehicleFileFetch(QObject* context, Callback done);

    bool _startFtp(Vehicle* vehicle, const QString& uri, const Options& options);
    bool _startHttp(const QUrl& url, const Options& options);
    void _finish(const QString& localFile, const QString& errorMsg);
    /// Ends the transfer and removes what it has written
    void _end();
    void _removeHttpOutput() const;

    Callback _done;
    QPointer<FTPRequest> _ftpRequest;
    QGCCachedFileDownload* _cachedDownload = nullptr;
    /// The downloader of an HTTP fetch, that of _cachedDownload with the cache
    QGCFileDownload* _download = nullptr;
    bool _ended = false;
};
