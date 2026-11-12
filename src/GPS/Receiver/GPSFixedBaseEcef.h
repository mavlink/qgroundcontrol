#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtQmlIntegration/QtQmlIntegration>

#include "Fact.h"

class RTKSettings;

/// The fixed base position of the receiver settings as ECEF X, Y and Z in metres: another view of the one saved
/// position, which stays latitude, longitude and ellipsoid height. The coordinates follow the saved position. An
/// edited coordinate saves the position it gives with the other two, and all three then keep the values entered, so
/// showing them changes neither them nor the saved position. Coordinates that are no point near the Earth's surface,
/// as while a new position is typed one coordinate after the other, are kept here and save nothing.
class GPSFixedBaseEcef : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_MOC_INCLUDE("RTKSettings.h")
    Q_PROPERTY(RTKSettings* settings READ settings WRITE setSettings NOTIFY settingsChanged FINAL)
    Q_PROPERTY(Fact* x READ x CONSTANT FINAL)
    Q_PROPERTY(Fact* y READ y CONSTANT FINAL)
    Q_PROPERTY(Fact* z READ z CONSTANT FINAL)
    /// The coordinates were edited to a point that is not near the Earth's surface, so the saved position is not
    /// theirs.
    Q_PROPERTY(bool unsaved READ unsaved NOTIFY unsavedChanged FINAL)

public:
    /// Decimals the coordinates show: 0.1 mm, the resolution of the station position in RTCM 1005.
    static constexpr int DECIMAL_PLACES = 4;

    explicit GPSFixedBaseEcef(QObject* parent = nullptr);

    RTKSettings* settings() const { return _settings; }

    void setSettings(RTKSettings* settings);

    Fact* x() { return &_xFact; }

    Fact* y() { return &_yFact; }

    Fact* z() { return &_zFact; }

    bool unsaved() const { return _unsaved; }

signals:
    void settingsChanged();
    void unsavedChanged();

private:
    /// Shows the saved position, unless this object is saving it.
    void _showSaved();
    /// Saves the position the coordinates give, unless they are being shown.
    void _saveEdited();
    void _setUnsaved(bool unsaved);

    QPointer<RTKSettings> _settings;
    Fact _xFact = Fact(0, QStringLiteral("fixedBasePositionEcefX"), FactMetaData::valueTypeDouble);
    Fact _yFact = Fact(0, QStringLiteral("fixedBasePositionEcefY"), FactMetaData::valueTypeDouble);
    Fact _zFact = Fact(0, QStringLiteral("fixedBasePositionEcefZ"), FactMetaData::valueTypeDouble);
    bool _showing = false;
    bool _saving = false;
    bool _unsaved = false;
};
