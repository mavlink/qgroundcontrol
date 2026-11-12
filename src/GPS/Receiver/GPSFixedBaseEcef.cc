#include "GPSFixedBaseEcef.h"

#include "GPSProtocolMath.h"
#include "RTKSettings.h"

GPSFixedBaseEcef::GPSFixedBaseEcef(QObject* parent)
    : QObject(parent)
{
    const auto describe = [this](Fact& fact, const QString& label) {
        auto* const metaData = new FactMetaData(FactMetaData::valueTypeDouble, fact.name(), this);
        metaData->setLabel(label);
        metaData->setShortDescription(label);
        // No unit: one would follow the application's distance unit, and a data sheet's coordinates are in metres.
        metaData->setDecimalPlaces(DECIMAL_PLACES);
        metaData->setRawMin(-GPSProtocolMath::EARTH_SURFACE_MAX_RADIUS_METERS);
        metaData->setRawMax(GPSProtocolMath::EARTH_SURFACE_MAX_RADIUS_METERS);
        fact.setMetaData(metaData);
        (void) connect(&fact, &Fact::rawValueChanged, this, &GPSFixedBaseEcef::_saveEdited);
    };
    describe(_xFact, tr("Base Position X (ECEF, m)"));
    describe(_yFact, tr("Base Position Y (ECEF, m)"));
    describe(_zFact, tr("Base Position Z (ECEF, m)"));
}

void GPSFixedBaseEcef::setSettings(RTKSettings* settings)
{
    if (settings == _settings) {
        return;
    }
    if (_settings) {
        for (Fact* const fact : {_settings->fixedBasePositionLatitude(), _settings->fixedBasePositionLongitude(),
                                 _settings->fixedBasePositionAltitude()}) {
            (void) fact->disconnect(this);
        }
    }
    _settings = settings;
    if (settings) {
        for (Fact* const fact : {settings->fixedBasePositionLatitude(), settings->fixedBasePositionLongitude(),
                                 settings->fixedBasePositionAltitude()}) {
            (void) connect(fact, &Fact::rawValueChanged, this, &GPSFixedBaseEcef::_showSaved);
        }
        _showSaved();
    }
    emit settingsChanged();
}

void GPSFixedBaseEcef::_showSaved()
{
    if (_saving || !_settings) {
        return;
    }
    const GPSProtocolMath::Ecef ecef =
        GPSProtocolMath::toEcef({.latitudeDegrees = _settings->fixedBasePositionLatitude()->rawValue().toDouble(),
                                 .longitudeDegrees = _settings->fixedBasePositionLongitude()->rawValue().toDouble(),
                                 .altitudeMeters = _settings->fixedBasePositionAltitude()->rawValue().toDouble()});
    _showing = true;
    _xFact.setRawValue(ecef.x);
    _yFact.setRawValue(ecef.y);
    _zFact.setRawValue(ecef.z);
    _showing = false;
    _setUnsaved(false);
}

void GPSFixedBaseEcef::_saveEdited()
{
    if (_showing || !_settings) {
        return;
    }
    const GPSProtocolMath::Ecef ecef{
        .x = _xFact.rawValue().toDouble(), .y = _yFact.rawValue().toDouble(), .z = _zFact.rawValue().toDouble()};
    if (!GPSProtocolMath::nearEarthSurface(ecef)) {
        _setUnsaved(true);
        return;
    }
    const GPSEllipsoidPosition position = GPSProtocolMath::fromEcef(ecef);
    // Not shown again while its three values are saved one after the other: the coordinates keep what was entered.
    _saving = true;
    _settings->fixedBasePositionLatitude()->setRawValue(position.latitudeDegrees);
    _settings->fixedBasePositionLongitude()->setRawValue(position.longitudeDegrees);
    _settings->fixedBasePositionAltitude()->setRawValue(position.altitudeMeters);
    _saving = false;
    _setUnsaved(false);
}

void GPSFixedBaseEcef::_setUnsaved(bool unsaved)
{
    if (_unsaved != unsaved) {
        _unsaved = unsaved;
        emit unsavedChanged();
    }
}
