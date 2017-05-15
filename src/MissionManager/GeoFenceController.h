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

class GeoFenceManager;

class GeoFenceController : public PlanElementController
{
    Q_OBJECT
    
public:
    GeoFenceController(PlanMasterController* masterController, QObject* parent = NULL);
    ~GeoFenceController();

    Q_PROPERTY(QmlObjectListModel*  inclusionMapPolygons    READ inclusionMapPolygons                               CONSTANT)
    Q_PROPERTY(QmlObjectListModel*  exclusionMapPolygons    READ exclusionMapPolygons                               CONSTANT)
    Q_PROPERTY(QGeoCoordinate       breachReturnPoint       READ breachReturnPoint      WRITE setBreachReturnPoint  NOTIFY breachReturnPointChanged)

    // The following properties are reflections of properties from GeoFenceManager
    Q_PROPERTY(bool             circleEnabled           READ circleEnabled          NOTIFY circleEnabledChanged)
    Q_PROPERTY(Fact*            circleRadiusFact        READ circleRadiusFact       NOTIFY circleRadiusFactChanged)
    Q_PROPERTY(bool             polygonSupported        READ polygonSupported       NOTIFY polygonSupportedChanged)
    Q_PROPERTY(bool             polygonEnabled          READ polygonEnabled         NOTIFY polygonEnabledChanged)
    Q_PROPERTY(bool             breachReturnSupported   READ breachReturnSupported  NOTIFY breachReturnSupportedChanged)
    Q_PROPERTY(QVariantList     params                  READ params                 NOTIFY paramsChanged)
    Q_PROPERTY(QStringList      paramLabels             READ paramLabels            NOTIFY paramLabelsChanged)

    // FIXME: Method/Signal names here are bad

    Q_INVOKABLE void signalAddInclusionPolygon     (void) { emit addInclusionPolygon(); }
    Q_INVOKABLE void signalAddExclusionPolygon     (void) { emit addExclusionPolygon(); }

    Q_INVOKABLE void addInclusion(QGeoCoordinate topLeft, QGeoCoordinate bottomRight);
    Q_INVOKABLE void addExclusion(QGeoCoordinate topLeft, QGeoCoordinate bottomRight);

    void start                      (bool editMode) final;
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
    void managerVehicleChanged      (Vehicle* managerVehicle) final;
    bool showPlanFromManagerVehicle (void) final;

    bool                circleEnabled           (void) const;
    Fact*               circleRadiusFact        (void) const;
    bool                polygonSupported        (void) const;
    bool                polygonEnabled          (void) const;
    QmlObjectListModel* inclusionMapPolygons    (void) { return &_inclusionPolygons; }
    QmlObjectListModel* exclusionMapPolygons    (void) { return &_exclusionPolygons; }
    bool                breachReturnSupported   (void) const;
    QVariantList        params                  (void) const;
    QStringList         paramLabels             (void) const;
    QGeoCoordinate      breachReturnPoint       (void) const { return _breachReturnPoint; }

    void setBreachReturnPoint(const QGeoCoordinate& breachReturnPoint);

signals:
    void breachReturnPointChanged       (QGeoCoordinate breachReturnPoint);
    void editorQmlChanged               (QString editorQml);
    void loadComplete                   (void);
    void addInclusionPolygon            (void);
    void addExclusionPolygon            (void);
    void circleEnabledChanged           (bool circleEnabled);
    void circleRadiusFactChanged        (Fact* circleRadiusFact);
    void polygonSupportedChanged        (bool polygonSupported);
    void polygonEnabledChanged          (bool polygonEnabled);
    void breachReturnSupportedChanged   (bool breachReturnSupported);
    void paramsChanged                  (QVariantList params);
    void paramLabelsChanged             (QStringList paramLabels);

private slots:
    void _polygonDirtyChanged(bool dirty);
    void _setDirty(void);
    void _setPolygonsFromManager(const QList<QList<QGeoCoordinate>>& inclusionPolygons, const QList<QList<QGeoCoordinate>>& exclusionPolygons);
    void _setReturnPointFromManager(QGeoCoordinate breachReturnPoint);
    void _managerLoadComplete(void);
    void _updateContainsItems(void);
    void _managerSendComplete(bool error);
    void _managerRemoveAllComplete(bool error);

private:
    void _init(void);
    void _signalAll(void);

    GeoFenceManager*    _geoFenceManager;
    bool                _dirty;
    QmlObjectListModel  _inclusionPolygons;
    QmlObjectListModel  _exclusionPolygons;
    QGeoCoordinate      _breachReturnPoint;
    bool                _itemsRequested;

    static const char* _jsonFileTypeValue;
    static const char* _jsonBreachReturnKey;
};

#endif
