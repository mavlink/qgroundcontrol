/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtGui/QImage>

#include "TileMath.h"

class DecodedTileFetcher;
class QNetworkAccessManager;

/// Async cache-first source of map tile images for draping onto surface patches.
///
/// Thin adapter over QGC's existing map tile infrastructure: tile requests hit
/// the shared tile cache database first, fall back to a provider network fetch
/// on miss (URL/headers from the provider registry), and store fetched tiles
/// back into the cache. Patches are slippy-tile addressed, so one request maps
/// to exactly one provider tile.
///
/// Results are always delivered asynchronously: one of tileImageReady, tileImageFailed
/// and tileImageMissing for every request. No signal is emitted for a
/// cancelled request. No internal retries: a failed request stays failed, and
/// consumers re-request on their own cadence (patch eviction and re-add).
class TileImageSource : public QObject
{
    Q_OBJECT

public:
    /// mapType is a provider type string from UrlFactory::getProviderTypes()
    /// (e.g. "Google Satellite"). With an unknown type every request fails
    /// (asynchronously) without touching the cache or network.
    /// \a networkManager fetches the tiles instead of the network manager of
    /// the map engine (test injection seam); it is not owned and must outlive
    /// this object
    explicit TileImageSource(const QString& mapType, QObject* parent = nullptr,
                             QNetworkAccessManager* networkManager = nullptr);

    QString mapType() const { return _mapType; }

    /// Request the image for a tile. Returns a request id (> 0).
    int requestTileImage(const TileMath::TileKey& key);

    /// Cancel a pending request, aborting any in-flight network fetch.
    void cancelRequest(int requestId);

    int pendingCount() const;

signals:
    void tileImageReady(int requestId, const QImage& image);
    void tileImageFailed(int requestId);
    /// The provider has no tile at this position: a new request gets the same answer
    void tileImageMissing(int requestId);
    /// The user configured another source for the provider (QGCMapEngine::tileSourceChanged): the images delivered
    /// so far are of the previous source, and those of the requests still pending can be.
    void sourceChanged();

private:
    const QString _mapType;
    const int _mapId;
    DecodedTileFetcher* const _fetcher;  ///< its fetch ids are the request ids
};
