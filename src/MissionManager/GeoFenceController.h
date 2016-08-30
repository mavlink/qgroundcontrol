/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef GeoFenceController_H
#define GeoFenceController_H

#include "PlanElementController.h"
#include "GeoFenceManager.h"
#include "QGCMapPolygon.h"
#include "Vehicle.h"
#include "MultiVehicleManager.h"
#include "QGCLoggingCategory.h"

Q_DECLARE_LOGGING_CATEGORY(GeoFenceControllerLog)

class GeoFenceController : public PlanElementController
{
    Q_OBJECT
    
public:
    GeoFenceController(QObject* parent = NULL);
    ~GeoFenceController();
    
    enum GeoFenceTypeEnum {
        GeoFenceNone =      GeoFenceManager::GeoFenceNone,
        GeoFenceCircle =    GeoFenceManager::GeoFenceCircle,
        GeoFencePolygon  =  GeoFenceManager::GeoFencePolygon,
    };

    Q_PROPERTY(GeoFenceTypeEnum     fenceType           READ fenceType          WRITE setFenceType          NOTIFY fenceTypeChanged)
    Q_PROPERTY(float                circleRadius        READ circleRadius       WRITE setCircleRadius       NOTIFY circleRadiusChanged)
    Q_PROPERTY(QGCMapPolygon*       polygon             READ polygon                                        CONSTANT)
    Q_PROPERTY(QGeoCoordinate       breachReturnPoint   READ breachReturnPoint  WRITE setBreachReturnPoint  NOTIFY breachReturnPointChanged)
    Q_PROPERTY(QVariantList         params              READ params                                         NOTIFY paramsChanged)

    void start              (bool editMode) final;
    void loadFromVehicle    (void) final;
    void sendToVehicle      (void) final;
    void loadFromFilePicker (void) final;
    void loadFromFile       (const QString& filename) final;
    void saveToFilePicker   (void) final;
    void saveToFile         (const QString& filename) final;
    void removeAll          (void) final;
    bool syncInProgress     (void) const final;
    bool dirty              (void) const final;
    void setDirty           (bool dirty) final;

    GeoFenceTypeEnum    fenceType           (void) const { return (GeoFenceTypeEnum)_geoFence.fenceType; }
    float               circleRadius        (void) const { return _geoFence.circleRadius; }
    QGCMapPolygon*      polygon             (void) { return &_geoFence.polygon; }
    QGeoCoordinate      breachReturnPoint   (void) const { return _geoFence.breachReturnPoint; }
    QVariantList        params              (void) { return _params; }

    void setFenceType(GeoFenceTypeEnum fenceType);
    void setCircleRadius(float circleRadius);
    void setBreachReturnPoint(const QGeoCoordinate& breachReturnPoint);

signals:
    void fenceTypeChanged           (GeoFenceTypeEnum fenceType);
    void circleRadiusChanged        (float circleRadius);
    void polygonPathChanged         (const QVariantList& polygonPath);
    void breachReturnPointChanged   (QGeoCoordinate breachReturnPoint);
    void paramsChanged              (void);

private slots:
    void _parameterReadyVehicleAvailableChanged(bool parameterReadyVehicleAvailable);
    void _newGeoFenceAvailable(void);
    void _polygonDirtyChanged(bool dirty);

private:
    void _setParams(void);
    void _clearGeoFence(void);
    void _setGeoFence(const GeoFenceManager::GeoFence_t& geoFence);

    void _activeVehicleBeingRemoved(void) final;
    void _activeVehicleSet(void) final;

    bool                        _dirty;
    GeoFenceManager::GeoFence_t _geoFence;
    QVariantList                _params;
};

#endif
