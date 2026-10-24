#pragma once

#include "TransectStyleComplexItem.h"
#include "SettingsFact.h"
#include "QGCMapPolyline.h"

class CorridorScanComplexItem : public TransectStyleComplexItem
{
    Q_OBJECT

public:
    /// @param flyView true: Created for use in the Fly View, false: Created for use in the Plan View
    /// @param kmlOrShpFile Polyline comes from this file, empty for default polyline
    CorridorScanComplexItem(PlanMasterController* masterController, bool flyView, const QString& kmlOrShpFile);

    Q_PROPERTY(QGCMapPolyline*  corridorPolyline    READ corridorPolyline   CONSTANT)
    Q_PROPERTY(Fact*            corridorWidth       READ corridorWidth      CONSTANT)
    /// Corridor area outlines to draw on the map: each closed boundary ring, or the path itself for a single pass
    Q_PROPERTY(QVariantList corridorBoundaries READ corridorBoundaries NOTIFY corridorBoundariesChanged)

    // Note1: These values are persisted to plan files so they cannot be changed with breaking plan file back compat
    // Note2: rotateEntryPoint expects these values in this order
    enum EntryPointLocation {
        EntryPointDefaultOrder = 0,                 // Standard transect generation order
        EntryPointStartSameEndOppositeSide = 1,     // Start at same end, opposite side of center
        EntryPointStartOppositeEndSameSide = 2,     // Start at opposite end, same side
        EntryPointStartOppositeEndOppositeSide = 3, // Start at opposite end, opposite side
    };
    Q_ENUM(EntryPointLocation)

    Fact*           corridorWidth   (void) { return &_corridorWidthFact; }
    QGCMapPolyline* corridorPolyline(void) { return &_corridorPolyline; }

    QVariantList corridorBoundaries(void) const { return _corridorBoundaryPaths; }

    Q_INVOKABLE void rotateEntryPoint(void);

    // Overrides from TransectStyleComplexItem
    QString patternName         (void) const final { return tr(canonicalName); }
    void    save                (QJsonArray&  planItems) final;
    bool    specifiesCoordinate (void) const final;
    double  timeBetweenShots    (void) final;

    // Overrides from ComplexMissionItem
    bool    load                (const QJsonObject& complexObject, int sequenceNumber, QString& errorString) final;
    QString mapVisualQML        (void) const final { return QStringLiteral("CorridorScanMapVisual.qml"); }
    QString presetsSettingsGroup(void) { return settingsGroup; }
    void    savePreset          (const QString& name);
    void    loadPreset          (const QString& name);

    // Overrides from VisualMissionionItem
    QString             commandDescription  (void) const final { return tr("Corridor Scan"); }
    QString             commandName         (void) const final { return tr("Corridor Scan"); }
    QString             abbreviation        (void) const final { return tr("C"); }
    void                setCoordinate       (const QGeoCoordinate& coordinate) final;
    ReadyForSaveState   readyForSaveState   (void) const final;
    double              additionalTimeDelay (void) const final { return 0; }

    static constexpr const char* canonicalName = QT_TR_NOOP("Corridor Scan");

    static constexpr const char* settingsGroup =            "CorridorScan";
    static constexpr const char* corridorWidthName =        "CorridorWidth";

    static constexpr const char* jsonComplexItemTypeValue = "CorridorScan";

signals:
    void corridorBoundariesChanged(void);

private slots:
    void _polylineDirtyChanged          (bool dirty);
    void _rebuildCorridorPolygon        (void);
    void _updateWizardMode              (void);
    void _updateSpecifiesCoordinate     (void);

    // Overrides from TransectStyleComplexItem
    void _rebuildTransectsPhase1    (void) final;
    void _recalcCameraShots         (void) final;

protected:
    QList<QList<QGeoCoordinate>> _surveyAreaBoundaries(void) const final;

private:
    void _buildCorridorArea(void);
    double  _calcTransectSpacing    (void) const;
    int     _calcTransectCount      (void) const;
    void    _saveCommon             (QJsonObject& complexObject);
    bool    _loadWorker              (const QJsonObject& complexObject, int sequenceNumber, QString& errorString, bool forPresets);

    QGCMapPolyline                  _corridorPolyline;
    QList<QList<QGeoCoordinate>>    _transectSegments;      ///< Internal transect segments including grid exit, turnaround and internal camera points
    QList<QList<QGeoCoordinate>> _corridorBoundaryRings;    ///< Outer boundary first, followed by holes
    QVariantList _corridorBoundaryPaths;

    EntryPointLocation              _entryPointLocation;

    QMap<QString, FactMetaData*>    _metaDataMap;
    SettingsFact                    _corridorWidthFact;
    bool                            _specifiesCoordinate = false;
    bool _noTransects = false;

    static constexpr const char* _jsonEntryPointKey =       "EntryPoint";
};
