#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtQmlIntegration/QtQmlIntegration>

/// The maps that there are, and the one of them that the user chose. A map is named "<provider> <type>", or by its
/// provider alone when that has one map without a type.
class MapCatalog : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_PROPERTY(QStringList mapList READ mapList CONSTANT FINAL)
    Q_PROPERTY(QStringList downloadableMapList READ downloadableMapList CONSTANT FINAL)
    /// Name of the map the user chose, as mapList has it. The settings select the map by a provider and a type that
    /// are set one after the other and name no map in between: the map chosen before stays until they name one.
    /// Saved settings that name no map are repaired when the catalog is created: they get the first type of their
    /// provider, or the default map when no map has that provider.
    Q_PROPERTY(QString chosenMap READ chosenMap NOTIFY chosenMapChanged FINAL)
    Q_PROPERTY(QStringList mapProviderList READ mapProviderList CONSTANT FINAL)
    Q_PROPERTY(QStringList elevationProviderList READ elevationProviderList CONSTANT FINAL)

public:
    explicit MapCatalog(QObject* parent = nullptr);
    static MapCatalog* instance();

    /// True for a map \a mapType, named as mapList() names it, that a map engine can show: one that is not an
    /// elevation provider. False for a name that is not in that list.
    Q_INVOKABLE static bool isImageryMap(const QString& mapType);
    /// True for a map \a mapType, named as mapList() names it, of satellite or aerial pictures, also one with
    /// labels on them (a hybrid map). The map style of the provider tells, as the name of a map need not say it.
    /// False for a name that is not in that list.
    Q_INVOKABLE static bool isSatelliteMap(const QString& mapType);
    /// True for a map \a mapType, named as mapList() names it, that is dark: a satellite map, or a map in the colors
    /// of the night. Views draw what they put on such a map in light colors. False for a name that is not in that
    /// list.
    Q_INVOKABLE static bool isDarkMap(const QString& mapType);
    /// The deepest zoom level of which the provider of the map \a mapType, named as mapList() names it, has tiles
    /// (MapProvider::maxZoom()). That of a provider whose depth is not known for a name that is not in that list.
    Q_INVOKABLE static int maxZoom(const QString& mapType);
    Q_INVOKABLE static QStringList mapTypeList(const QString& provider);

    QString chosenMap() const { return _chosenMap; }

    /// The maps and the elevation providers, without the maps that are offered no more
    /// (MapProvider::isUserSelectable()): one of those is still shown for an offline set that has it
    static QStringList mapList();
    /// The maps of mapList() whose tiles an offline set can download: those that have a server
    /// (MapProvider::hasServer())
    static QStringList downloadableMapList();
    /// The name, as mapList() has it, of the map of \a provider that has \a type: the two parts by which the
    /// settings select a map. A provider whose only map has no type names that map itself, and \a type is ignored.
    static QString mapName(const QString& provider, const QString& type);
    static QStringList mapProviderList();
    static QStringList elevationProviderList();

signals:
    void chosenMapChanged();

private slots:
    void _updateChosenMap();

private:
    /// The maps that a map engine can show: mapList() without the elevation providers
    static QStringList _imageryMapList();
    /// The name that the provider and the type in the settings make: no map has it while one of the two is being set
    static QString _mapOfSettings();
    /// Makes settings that name no map name one: they get the first type of their provider or, when no map has that
    /// provider, the default map. They stay as they are when the default settings name no map either. Earlier
    /// versions saved the provider "Street" with the type "Street Map" for OpenStreetMap, which names no map.
    static void _repairMapOfSettings();

    QString _chosenMap;
};
