#pragma once

#include <limits>

#include "ComplexMissionItem.h"
#include "FactMetaData.h"
#include "MissionItem.h"
#include "QGCMapPolygon.h"
#include "SettingsFact.h"

class PlanMasterController;

/// \brief A custom complex mission item that generates waypoints following the perimeter of a user-defined polygon.
///
/// A custom complex mission item that generates waypoints following the
/// perimeter of a user-defined polygon. Shows full polygon-editing tools
/// on the map (same toolbar as Survey/StructureScan).

class PerimeterScanComplexItem : public ComplexMissionItem
{
    Q_OBJECT

public:
    /// @param kmlOrShpFile  Optional KML/SHP file to seed the polygon from; empty for blank.
    explicit PerimeterScanComplexItem(PlanMasterController *masterController,
                                      bool flyView,
                                      const QString &kmlOrShpFile = QString());

    Q_PROPERTY(QGCMapPolygon *perimeterPolygon READ perimeterPolygon CONSTANT)
    Q_PROPERTY(Fact          *altitude         READ altitude         CONSTANT)

    QGCMapPolygon *perimeterPolygon() { return &_perimeterPolygon; }
    Fact          *altitude()         { return &_altitudeFact; }

    // These are called by the editor QML's polygon-capture callbacks.
    Q_INVOKABLE void clearPolygon()                                              { _perimeterPolygon.clear(); }
    Q_INVOKABLE void addPolygonCoordinate(const QGeoCoordinate &coordinate)     { _perimeterPolygon.appendVertex(coordinate); }
    Q_INVOKABLE void adjustPolygonCoordinate(int vertexIndex,
                                             const QGeoCoordinate &coordinate)  { _perimeterPolygon.adjustVertex(vertexIndex, coordinate); }

    static constexpr const char *canonicalName           = "Perimeter Scan";
    static constexpr const char *jsonComplexItemTypeValue = "perimeterScan";
    static constexpr const char *settingsGroup            = "PerimeterScan";

    // ComplexMissionItem overrides
    QString             patternName         () const final { return tr(canonicalName); }
    double              complexDistance     () const final { return _scanDistance; }
    double              minAMSLAltitude     () const final { return amslEntryAlt(); }
    double              maxAMSLAltitude     () const final { return amslExitAlt(); }
    int                 lastSequenceNumber  () const final;
    bool                load                (const QJsonObject &complexObject, int sequenceNumber, QString &errorString) final;
    double              greatestDistanceTo  (const QGeoCoordinate &other) const final;
    QString             mapVisualQML        () const final { return QStringLiteral("qrc:/qml/Custom/Plan/PerimeterScanMapVisual.qml"); }

    // VisualMissionItem overrides
    bool                dirty                     () const final { return _dirty; }
    bool                isSimpleItem              () const final { return false; }
    bool                isStandaloneCoordinate    () const final { return false; }
    bool                specifiesCoordinate       () const final { return _perimeterPolygon.isValid(); }
    bool                specifiesAltitudeOnly     () const final { return false; }
    QString             commandDescription        () const final { return tr("Perimeter Scan"); }
    QString             commandName               () const final { return tr("Perimeter Scan"); }
    QString             abbreviation              () const final { return "P"; }
    QGeoCoordinate      coordinate                () const final;
    QGeoCoordinate      entryCoordinate           () const final { return coordinate(); }
    QGeoCoordinate      exitCoordinate            () const final;
    bool                exitCoordinateSameAsEntry () const final { return false; }
    double              editableAlt               () const final { return _altitudeFact.rawValue().toDouble(); }
    double              amslEntryAlt              () const final;
    double              amslExitAlt               () const final { return amslEntryAlt(); }
    int                 sequenceNumber            () const final { return _sequenceNumber; }
    double              specifiedFlightSpeed      () final { return std::numeric_limits<double>::quiet_NaN(); }
    double              specifiedGimbalYaw        () final { return std::numeric_limits<double>::quiet_NaN(); }
    double              specifiedGimbalPitch      () final { return std::numeric_limits<double>::quiet_NaN(); }
    void                appendMissionItems        (QList<MissionItem *> &items, QObject *missionItemParent) final;
    void                setMissionFlightStatus    (MissionFlightStatus_t &missionFlightStatus) final;
    void                applyNewAltitude          (double newAltitude) final;
    double              additionalTimeDelay       () const final { return 0; }
    ReadyForSaveState   readyForSaveState         () const final;
    void                setDirty                  (bool dirty) final;
    void                setCoordinate             (const QGeoCoordinate &coord) final;
    void                setSequenceNumber         (int sequenceNumber) final;
    void                save                      (QJsonArray &missionItems) final;

private slots:
    void _setDirty();
    void _polygonChanged();

private:
    void _recalcScanDistance();

    int                          _sequenceNumber = 0;
    double                       _scanDistance   = 0.0;
    QGCMapPolygon                _perimeterPolygon;
    QMap<QString, FactMetaData *> _metaDataMap;
    SettingsFact                 _altitudeFact;

    static constexpr const char *_jsonAltitudeKey = "altitude";
};
