/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtGui/QImage>

#include "TileMath.h"
#include "WarningThrottle.h"

class TileReply;

/// Async cache-first source of map tile images for draping onto surface patches.
///
/// Thin adapter over QGC's existing map tile infrastructure: tile requests hit
/// the shared tile cache database first, fall back to a provider network fetch
/// on miss (URL/headers from the provider registry), and store fetched tiles
/// back into the cache. Patches are slippy-tile addressed, so one request maps
/// to exactly one provider tile.
///
/// Results are always delivered asynchronously. No signal is emitted for a
/// cancelled request. No internal retries: a failed request stays failed, and
/// consumers re-request on their own cadence (patch eviction and re-add).
class TileImageSource : public QObject
{
    Q_OBJECT

public:
    /// mapType is a provider type string from MapProviderRegistry::providerTypes()
    /// (e.g. "Google Satellite"). With an unknown type every request fails
    /// (asynchronously) without touching the cache or network.
    explicit TileImageSource(const QString& mapType, QObject* parent = nullptr);

    QString mapType() const { return _mapType; }

    /// Request the image for a tile. Returns a request id (> 0).
    int requestTileImage(const TileMath::TileKey& key);

    /// Cancel a pending request, aborting any in-flight network fetch.
    void cancelRequest(int requestId);

    int pendingCount() const { return _pending.count(); }

signals:
    void tileImageReady(int requestId, const QImage& image);
    void tileImageFailed(int requestId);
    /// The user configured another source for the provider (MapEngine::tileSourceChanged): the images delivered
    /// so far are of the previous source, and those of the requests still pending can be.
    void sourceChanged();

private:
    void _warnFailure(const TileMath::TileKey& key, const QString& reason);

    const QString _mapType;
    const int _mapId;
    int _requestIdCounter = 0;
    /// Requests that have not finished or been cancelled, with their fetch
    QHash<int, TileReply*> _pending;
    WarningThrottle _failureWarnings;
};
