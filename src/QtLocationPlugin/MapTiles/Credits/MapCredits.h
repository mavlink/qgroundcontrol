#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

class EsriAttribution;

/// The credits that the providers of the maps ask for where their tiles are shown. A map is named as
/// MapCatalog::mapList() names it.
class MapCredits : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_PROPERTY(QString terrainAttribution READ terrainAttribution CONSTANT FINAL)
    /// Counts the changes of what areaAttribution() answers, as when the list of a map's credits arrives: a binding
    /// that reads it beside its call of areaAttribution() is evaluated again
    Q_PROPERTY(int areaAttributionRevision READ areaAttributionRevision NOTIFY areaAttributionChanged FINAL)

public:
    explicit MapCredits(QObject* parent = nullptr);
    static MapCredits* instance();

    /// The credit of the provider of the map \a mapType. Empty for a name that is no map, and for a provider
    /// without a credit.
    Q_INVOKABLE static QString mapAttribution(const QString& mapType);
    /// The credits that the provider of the map \a mapType asks for beside mapAttribution() where a view shows its
    /// tiles: the sources of what is on screen, which depend on the place and the zoom level. The best source
    /// there is first, and they are joined with ", ". The view has its centre at \a latitude and \a longitude, in
    /// degrees, and is \a width by \a height pixels of a map at the zoom level \a zoomLevel, whose tiles are 256
    /// pixels wide. The credits are those of the tile zoom level at or below \a zoomLevel, and of the area that
    /// such a view has from straight above, north up.
    /// Empty for a provider that publishes no list of such credits, and until its list has arrived:
    /// areaAttributionRevision changes when it has.
    Q_INVOKABLE QString areaAttribution(const QString& mapType, double latitude, double longitude, double zoomLevel,
                                        double width, double height);

    int areaAttributionRevision() const { return _areaAttributionRevision; }

    /// The credit of the terrain that the GeoMap engine shapes its surface with
    static QString terrainAttribution();

signals:
    void areaAttributionChanged();

private:
    /// The reader of the lists that areaAttribution() answers from, created when it is first needed. Unit tests reach
    /// no server: null under them while no test set the network manager of the map engine
    /// (MapEngine::setNetworkManagerForTest()).
    EsriAttribution* _areaAttributionLists();
    void _notifyAreaAttributionChanged();

    EsriAttribution* _esriAttribution = nullptr;
    int _areaAttributionRevision = 0;
};
