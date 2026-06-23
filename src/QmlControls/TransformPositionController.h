#pragma once

#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtPositioning/QGeoCoordinate>
#include <QtQmlIntegration/QtQmlIntegration>

class Fact;
class FactMetaData;

class TransformPositionController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_MOC_INCLUDE("Fact.h")
    Q_PROPERTY(QGeoCoordinate   coordinate          READ coordinate WRITE setCoordinate NOTIFY coordinateChanged)
    Q_PROPERTY(Fact             *latitude           READ latitude                       CONSTANT)
    Q_PROPERTY(Fact             *longitude          READ longitude                      CONSTANT)
    Q_PROPERTY(Fact             *zone               READ zone                           CONSTANT)
    Q_PROPERTY(Fact             *hemisphere         READ hemisphere                     CONSTANT)
    Q_PROPERTY(Fact             *easting            READ easting                        CONSTANT)
    Q_PROPERTY(Fact             *northing           READ northing                       CONSTANT)
    Q_PROPERTY(Fact             *mgrs               READ mgrs                           CONSTANT)
    Q_PROPERTY(Fact             *offsetEast         READ offsetEast                     CONSTANT)
    Q_PROPERTY(Fact             *offsetNorth        READ offsetNorth                    CONSTANT)
    Q_PROPERTY(Fact             *offsetUp           READ offsetUp                       CONSTANT)
    Q_PROPERTY(Fact             *rotateDegreesCW    READ rotateDegreesCW                CONSTANT)

public:
    explicit TransformPositionController(QObject *parent = nullptr);
    ~TransformPositionController();

    Q_INVOKABLE void initValues();
    Q_INVOKABLE void setFromGeo();
    Q_INVOKABLE void setFromUTM();
    Q_INVOKABLE void setFromMGRS();
    Q_INVOKABLE void setFromVehicle();

    void setCoordinate(QGeoCoordinate coordinate);
    QGeoCoordinate coordinate() const { return _coordinate; }

    Fact *latitude() { return _latitudeFact; }
    Fact *longitude() { return _longitudeFact; }
    Fact *zone() { return _zoneFact; }
    Fact *hemisphere() { return _hemisphereFact; }
    Fact *easting() { return _eastingFact; }
    Fact *northing() { return _northingFact; }
    Fact *mgrs() { return _mgrsFact; }
    Fact *offsetEast() { return _offsetEastFact; }
    Fact *offsetNorth() { return _offsetNorthFact; }
    Fact *offsetUp() { return _offsetUpFact; }
    Fact *rotateDegreesCW() { return _rotateDegreesCWFact; }

signals:
    void coordinateChanged(QGeoCoordinate coordinate);

private:
    QGeoCoordinate _coordinate;

    Fact *_latitudeFact = nullptr;
    Fact *_longitudeFact = nullptr;
    Fact *_zoneFact = nullptr;
    Fact *_hemisphereFact = nullptr;
    Fact *_eastingFact = nullptr;
    Fact *_northingFact = nullptr;
    Fact *_mgrsFact = nullptr;
    Fact *_offsetEastFact = nullptr;
    Fact *_offsetNorthFact = nullptr;
    Fact *_offsetUpFact = nullptr;
    Fact *_rotateDegreesCWFact = nullptr;

    static QMap<QString, FactMetaData*> _metaDataMap;

    static constexpr const char *_latitudeFactName = "Latitude";
    static constexpr const char *_longitudeFactName = "Longitude";
    static constexpr const char *_zoneFactName = "Zone";
    static constexpr const char *_hemisphereFactName = "Hemisphere";
    static constexpr const char *_eastingFactName = "Easting";
    static constexpr const char *_northingFactName = "Northing";
    static constexpr const char *_mgrsFactName = "MGRS";
    static constexpr const char *_offsetEastFactName = "OffsetEast";
    static constexpr const char *_offsetNorthFactName = "OffsetNorth";
    static constexpr const char *_offsetUpFactName = "OffsetUp";
    static constexpr const char *_rotateDegreesCWFactName = "RotateDegreesCW";
};
