#pragma once

#include <QtCore/QString>
#include <QtLocation/private/qgeotiledmap_p.h>

class QGCGeoTiledMappingManagerEngine;

/// A map of the tiles of the plugin. It tells the copyright notice of a Map item whom the provider of its map type
/// asks to be credited: the credit of the provider and, of a provider that lists them, the sources of what the view
/// shows (MapCredits).
class QGCGeoTiledMap : public QGeoTiledMap
{
    Q_OBJECT

public:
    explicit QGCGeoTiledMap(QGCGeoTiledMappingManagerEngine* engine, QObject* parent = nullptr);
    ~QGCGeoTiledMap();

    /// A notice that becomes visible is told the credits again
    void setCopyrightVisible(bool visible) final;

protected:
    /// QtLocation calls this for a map with a visible notice, when its map type changes and when tiles come into view
    void evaluateCopyrights(const QSet<QGeoTileSpec>& visibleTiles) final;

private:
    void _updateCopyrights();

    QString _copyrights;  ///< As told last, in HTML
};
