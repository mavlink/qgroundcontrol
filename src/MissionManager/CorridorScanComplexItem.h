/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "TransectStyleComplexItem.h"
#include "MissionItem.h"
#include "SettingsFact.h"
#include "QGCLoggingCategory.h"
#include "QGCMapPolyline.h"
#include "QGCMapPolygon.h"

Q_DECLARE_LOGGING_CATEGORY(CorridorScanComplexItemLog)

class CorridorScanComplexItem : public TransectStyleComplexItem
{
    Q_OBJECT

public:
    /// @param vehicle Vehicle which this is being contructed for
    /// @param flyView true: Created for use in the Fly View, false: Created for use in the Plan View
    /// @param kmlFile Polyline comes from this file, empty for default polyline
    CorridorScanComplexItem(Vehicle* vehicle, bool flyView, const QString& kmlFile, QObject* parent);

    Q_PROPERTY(QGCMapPolyline*  corridorPolyline    READ corridorPolyline   CONSTANT)
    Q_PROPERTY(Fact*            corridorWidth       READ corridorWidth      CONSTANT)

    Fact*           corridorWidth   (void) { return &_corridorWidthFact; }
    QGCMapPolyline* corridorPolyline(void) { return &_corridorPolyline; }

    Q_INVOKABLE void rotateEntryPoint(void);

    // Overrides from TransectStyleComplexItem
    void    save                (QJsonArray&  planItems) final;
    bool    specifiesCoordinate (void) const final;
    void    appendMissionItems  (QList<MissionItem*>& items, QObject* missionItemParent) final;
    void    applyNewAltitude    (double newAltitude) final;
    double  timeBetweenShots    (void) final;

    // Overrides from ComplexMissionItem
    bool    load                (const QJsonObject& complexObject, int sequenceNumber, QString& errorString) final;
    QString mapVisualQML        (void) const final { return QStringLiteral("CorridorScanMapVisual.qml"); }

    // Overrides from VisualMissionionItem
    QString             commandDescription  (void) const final { return tr("Corridor Scan"); }
    QString             commandName         (void) const final { return tr("Corridor Scan"); }
    QString             abbreviation        (void) const final { return tr("C"); }
    ReadyForSaveState   readyForSaveState   (void) const final;
    double              additionalTimeDelay (void) const final { return 0; }

    static const char* jsonComplexItemTypeValue;

    static const char* settingsGroup;
    static const char* corridorWidthName;

private slots:
    void _polylineDirtyChanged      (bool dirty);
    void _rebuildCorridorPolygon    (void);

    // Overrides from TransectStyleComplexItem
    void _rebuildTransectsPhase1    (void) final;
    void _recalcComplexDistance     (void) final;
    void _recalcCameraShots         (void) final;

private:
    double  _transectSpacing            (void) const;
    int     _transectCount              (void) const;
    void    _buildAndAppendMissionItems (QList<MissionItem*>& items, QObject* missionItemParent);
    void    _appendLoadedMissionItems   (QList<MissionItem*>& items, QObject* missionItemParent);

    QGCMapPolyline                  _corridorPolyline;
    QList<QList<QGeoCoordinate>>    _transectSegments;      ///< Internal transect segments including grid exit, turnaround and internal camera points

    int                             _entryPoint;

    QMap<QString, FactMetaData*>    _metaDataMap;
    SettingsFact                    _corridorWidthFact;

    static const char* _jsonEntryPointKey;
};
