/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
#include "CameraCalc.h"

Q_DECLARE_LOGGING_CATEGORY(CorridorScanComplexItemLog)

class CorridorScanComplexItem : public TransectStyleComplexItem
{
    Q_OBJECT

public:
    CorridorScanComplexItem(Vehicle* vehicle, QObject* parent = NULL);

    Q_PROPERTY(CameraCalc*      cameraCalc          READ cameraCalc         CONSTANT)
    Q_PROPERTY(QGCMapPolyline*  corridorPolyline    READ corridorPolyline   CONSTANT)
    Q_PROPERTY(Fact*            corridorWidth       READ corridorWidth      CONSTANT)

    Fact*           corridorWidth   (void) { return &_corridorWidthFact; }
    QGCMapPolyline* corridorPolyline(void) { return &_corridorPolyline; }

    Q_INVOKABLE void rotateEntryPoint(void);

    // Overrides from ComplexMissionItem

    int         lastSequenceNumber  (void) const final;
    bool        load                (const QJsonObject& complexObject, int sequenceNumber, QString& errorString) final;
    QString     mapVisualQML        (void) const final { return QStringLiteral("CorridorScanMapVisual.qml"); }

    // Overrides from TransectStyleComplexItem

    void        save                (QJsonArray&  missionItems) final;
    bool        specifiesCoordinate (void) const final;
    void        appendMissionItems  (QList<MissionItem*>& items, QObject* missionItemParent) final;
    void        applyNewAltitude    (double newAltitude) final;

    static const char* jsonComplexItemTypeValue;

    static const char* settingsGroup;
    static const char* corridorWidthName;

private slots:
    void _polylineDirtyChanged              (bool dirty);
    void _polylineCountChanged              (int count);
    void _rebuildCorridor                   (void);

    // Overrides from TransectStyleComplexItem
    virtual void _rebuildTransects          (void) final;

private:
    int _transectCount          (void) const;
    void _rebuildCorridorPolygon(void);


    QGCMapPolyline                  _corridorPolyline;
    QList<QList<QGeoCoordinate>>    _transectSegments;      ///< Internal transect segments including grid exit, turnaround and internal camera points

    bool            _ignoreRecalc;
    int             _entryPoint;

    QMap<QString, FactMetaData*>    _metaDataMap;
    SettingsFact                    _corridorWidthFact;

    static const char* _entryPointName;
};
