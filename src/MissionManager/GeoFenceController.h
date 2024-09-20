/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtPositioning/QGeoCoordinate>

#include "PlanElementController.h"
#include "QmlObjectListModel.h"
#include "Fact.h"

Q_DECLARE_LOGGING_CATEGORY(GeoFenceControllerLog)

class GeoFenceManager;
class QGCFenceCircle;
class QGCFencePolygon;
class Vehicle;

class GeoFenceController : public PlanElementController
{
    Q_OBJECT
    Q_MOC_INCLUDE("QGCFencePolygon.h")
    Q_MOC_INCLUDE("QGCFenceCircle.h")

public:
    GeoFenceController(PlanMasterController* masterController, QObject* parent = nullptr);
    ~GeoFenceController();

    Q_PROPERTY(QmlObjectListModel*  polygons                READ polygons                                           CONSTANT)
    Q_PROPERTY(QmlObjectListModel*  circles                 READ circles                                            CONSTANT)
    Q_PROPERTY(QGeoCoordinate       breachReturnPoint       READ breachReturnPoint      WRITE setBreachReturnPoint  NOTIFY breachReturnPointChanged)
    Q_PROPERTY(Fact*                breachReturnAltitude    READ breachReturnAltitude                               CONSTANT)

    // Radius of the "paramCircularFence" which is called the "Geofence Failsafe" in PX4 and the "Circular Geofence" on ArduPilot
    Q_PROPERTY(double               paramCircularFence      READ paramCircularFence                                 NOTIFY paramCircularFenceChanged)

    /// Add a new inclusion polygon to the fence
    ///     @param topLeft: Top left coordinate or map viewport
    ///     @param bottomRight: Bottom right left coordinate or map viewport
    Q_INVOKABLE void addInclusionPolygon(QGeoCoordinate topLeft, QGeoCoordinate bottomRight);

    /// Add a new inclusion circle to the fence
    ///     @param topLeft: Top left coordinate or map viewport
    ///     @param bottomRight: Bottom right left coordinate or map viewport
    Q_INVOKABLE void addInclusionCircle(QGeoCoordinate topLeft, QGeoCoordinate bottomRight);

    /// Deletes the specified polygon from the polygon list
    ///     @param index: Index of poygon to delete
    Q_INVOKABLE void deletePolygon(int index);

    /// Deletes the specified circle from the circle list
    ///     @param index: Index of circle to delete
    Q_INVOKABLE void deleteCircle(int index);

    /// Clears the interactive bit from all fence items
    Q_INVOKABLE void clearAllInteractive(void);

#ifdef QGC_UTM_ADAPTER
    Q_INVOKABLE void loadFlightPlanData(void);
#endif

    double  paramCircularFence  (void);
    Fact*   breachReturnAltitude(void) { return &_breachReturnAltitudeFact; }

    // Overrides from PlanElementController
    bool supported                  (void) const final;
    void start                      (bool flyView) final;
    void save                       (QJsonObject& json) final;
    bool load                       (const QJsonObject& json, QString& errorString) final;
    void loadFromVehicle            (void) final;
    void sendToVehicle              (void) final;
    void removeAll                  (void) final;
    void removeAllFromVehicle       (void) final;
    bool syncInProgress             (void) const final;
    bool dirty                      (void) const final;
    void setDirty                   (bool dirty) final;
    bool containsItems              (void) const final;
    bool showPlanFromManagerVehicle (void) final;

    QmlObjectListModel* polygons                (void) { return &_polygons; }
    QmlObjectListModel* circles                 (void) { return &_circles; }
    QGeoCoordinate      breachReturnPoint       (void) const { return _breachReturnPoint; }

    void setBreachReturnPoint   (const QGeoCoordinate& breachReturnPoint);
    bool isEmpty                (void) const;

signals:
    void breachReturnPointChanged       (QGeoCoordinate breachReturnPoint);
    void editorQmlChanged               (QString editorQml);
    void loadComplete                   (void);
    void paramCircularFenceChanged      (void);

#ifdef QGC_UTM_ADAPTER
    void uploadFlagSent         (bool flag);
    void polygonBoundarySent    (QList<QGeoCoordinate> coords);
#endif

private slots:
    void _polygonDirtyChanged       (bool dirty);
    void _setDirty                  (void);
    void _setFenceFromManager       (const QList<QGCFencePolygon>& polygons, const QList<QGCFenceCircle>&  circles);
    void _setReturnPointFromManager (QGeoCoordinate breachReturnPoint);
    void _managerLoadComplete       (void);
    void _updateContainsItems       (void);
    void _managerSendComplete       (bool error);
    void _managerRemoveAllComplete  (bool error);
    void _parametersReady           (void);
    void _managerVehicleChanged      (Vehicle* managerVehicle);

private:
    void _init(void);

    Vehicle*            _managerVehicle =               nullptr;
    GeoFenceManager*    _geoFenceManager =              nullptr;
    bool                _dirty =                        false;
    QmlObjectListModel  _polygons;
    QmlObjectListModel  _circles;
    QGeoCoordinate      _breachReturnPoint;
    Fact                _breachReturnAltitudeFact;
    double              _breachReturnDefaultAltitude =  qQNaN();
    bool                _itemsRequested =               false;

    Fact*               _px4ParamCircularFenceFact =        nullptr;
    Fact*               _apmParamCircularFenceRadiusFact =  nullptr;
    Fact*               _apmParamCircularFenceEnabledFact = nullptr;
    Fact*               _apmParamCircularFenceTypeFact =    nullptr;

    static QMap<QString, FactMetaData*> _metaDataMap;

    static constexpr int _jsonCurrentVersion = 2;

    static constexpr const char* _jsonFileTypeValue =        "GeoFence";
    static constexpr const char* _jsonBreachReturnKey =      "breachReturn";
    static constexpr const char* _jsonPolygonsKey =          "polygons";
    static constexpr const char* _jsonCirclesKey =           "circles";

    static constexpr const char* _breachReturnAltitudeFactName = "Altitude";

    static constexpr const char* _px4ParamCircularFence =    "GF_MAX_HOR_DIST";
    static constexpr const char* _apmParamCircularFenceRadius =    "FENCE_RADIUS";
    static constexpr const char* _apmParamCircularFenceEnabled =    "FENCE_ENABLE";
    static constexpr const char* _apmParamCircularFenceType =    "FENCE_TYPE";
};
