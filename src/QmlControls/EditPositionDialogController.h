/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "Fact.h"

#include <QtCore/QObject>
#include <QtCore/QLoggingCategory>
#include <QtPositioning/QGeoCoordinate>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(EditPositionDialogControllerLog)

class EditPositionDialogController : public QObject
{
    Q_OBJECT
    // QML_ELEMENT
    Q_PROPERTY(QGeoCoordinate   coordinate  READ coordinate WRITE setCoordinate NOTIFY coordinateChanged)
    Q_PROPERTY(Fact             *latitude   READ latitude                       CONSTANT)
    Q_PROPERTY(Fact             *longitude  READ longitude                      CONSTANT)
    Q_PROPERTY(Fact             *zone       READ zone                           CONSTANT)
    Q_PROPERTY(Fact             *hemisphere READ hemisphere                     CONSTANT)
    Q_PROPERTY(Fact             *easting    READ easting                        CONSTANT)
    Q_PROPERTY(Fact             *northing   READ northing                       CONSTANT)
    Q_PROPERTY(Fact             *mgrs       READ mgrs                           CONSTANT)

public:
    explicit EditPositionDialogController(QObject *parent = nullptr);
    ~EditPositionDialogController();

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

    static QMap<QString, FactMetaData*> _metaDataMap;

    static constexpr const char *_latitudeFactName = "Latitude";
    static constexpr const char *_longitudeFactName = "Longitude";
    static constexpr const char *_zoneFactName = "Zone";
    static constexpr const char *_hemisphereFactName = "Hemisphere";
    static constexpr const char *_eastingFactName = "Easting";
    static constexpr const char *_northingFactName = "Northing";
    static constexpr const char *_mgrsFactName = "MGRS";
};
