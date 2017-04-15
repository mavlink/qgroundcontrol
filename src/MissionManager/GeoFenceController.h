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
    GeoFenceController(QObject* parent = NULL);
    ~GeoFenceController();

    Q_PROPERTY(QGCMapPolygon*   mapPolygon              READ mapPolygon                                         CONSTANT)
    Q_PROPERTY(QGeoCoordinate   breachReturnPoint       READ breachReturnPoint      WRITE setBreachReturnPoint  NOTIFY breachReturnPointChanged)

    // The following properties are reflections of properties from GeoFenceManager
    Q_PROPERTY(bool             circleEnabled           READ circleEnabled          NOTIFY circleEnabledChanged)
    Q_PROPERTY(Fact*            circleRadiusFact        READ circleRadiusFact       NOTIFY circleRadiusFactChanged)
    Q_PROPERTY(bool             polygonSupported        READ polygonSupported       NOTIFY polygonSupportedChanged)
    Q_PROPERTY(bool             polygonEnabled          READ polygonEnabled         NOTIFY polygonEnabledChanged)
    Q_PROPERTY(bool             breachReturnSupported   READ breachReturnSupported  NOTIFY breachReturnSupportedChanged)
    Q_PROPERTY(QVariantList     params                  READ params                 NOTIFY paramsChanged)
    Q_PROPERTY(QStringList      paramLabels             READ paramLabels            NOTIFY paramLabelsChanged)

    Q_INVOKABLE void addPolygon     (void) { emit addInitialFencePolygon(); }
    Q_INVOKABLE void removePolygon  (void) { _mapPolygon.clear(); }

    void start                      (bool editMode) final;
    void startStaticActiveVehicle   (Vehicle* vehicle) final;
    void loadFromVehicle            (void) final;
    void sendToVehicle              (void) final;
    void loadFromFile               (const QString& filename) final;
    void saveToFile                 (const QString& filename) final;
    void removeAll                  (void) final;
    void removeAllFromVehicle       (void) final;
    bool syncInProgress             (void) const final;
    bool dirty                      (void) const final;
    void setDirty                   (bool dirty) final;
    bool containsItems              (void) const final;

    QString fileExtension(void) const final;

    bool            circleEnabled           (void) const;
    Fact*           circleRadiusFact        (void) const;
    bool            polygonSupported        (void) const;
    bool            polygonEnabled          (void) const;
    bool            breachReturnSupported   (void) const;
    QVariantList    params                  (void) const;
    QStringList     paramLabels             (void) const;
    QGCMapPolygon*  mapPolygon              (void) { return &_mapPolygon; }
    QGeoCoordinate  breachReturnPoint       (void) const { return _breachReturnPoint; }

    void setBreachReturnPoint(const QGeoCoordinate& breachReturnPoint);

signals:
    void breachReturnPointChanged       (QGeoCoordinate breachReturnPoint);
    void editorQmlChanged               (QString editorQml);
    void loadComplete                   (void);
    void addInitialFencePolygon         (void);
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
    void _setPolygonFromManager(const QList<QGeoCoordinate>& polygon);
    void _setReturnPointFromManager(QGeoCoordinate breachReturnPoint);
    void _loadComplete(const QGeoCoordinate& breachReturn, const QList<QGeoCoordinate>& polygon);
    void _updateContainsItems(void);

private:
    void _init(void);
    void _signalAll(void);
    bool _loadJsonFile(QJsonDocument& jsonDoc, QString& errorString);

    void _activeVehicleBeingRemoved(void) final;
    void _activeVehicleSet(void) final;

    bool            _dirty;
    QGCMapPolygon   _mapPolygon;
    QGeoCoordinate  _breachReturnPoint;

    static const char* _jsonFileTypeValue;
    static const char* _jsonBreachReturnKey;
};

#endif
