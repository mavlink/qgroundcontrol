/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef RallyPointController_H
#define RallyPointController_H

#include "PlanElementController.h"
#include "RallyPointManager.h"
#include "Vehicle.h"
#include "MultiVehicleManager.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"

Q_DECLARE_LOGGING_CATEGORY(RallyPointControllerLog)

class GeoFenceManager;

class RallyPointController : public PlanElementController
{
    Q_OBJECT
    
public:
    RallyPointController(PlanMasterController* masterController, QObject* parent = NULL);
    ~RallyPointController();
    
    Q_PROPERTY(bool                 rallyPointsSupported    READ rallyPointsSupported                               NOTIFY rallyPointsSupportedChanged)
    Q_PROPERTY(QmlObjectListModel*  points                  READ points                                             CONSTANT)
    Q_PROPERTY(QString              editorQml               READ editorQml                                          CONSTANT)
    Q_PROPERTY(QObject*             currentRallyPoint       READ currentRallyPoint      WRITE setCurrentRallyPoint  NOTIFY currentRallyPointChanged)

    Q_INVOKABLE void addPoint(QGeoCoordinate point);
    Q_INVOKABLE void removePoint(QObject* rallyPoint);

    void save                   (QJsonObject& json) final;
    bool load                   (const QJsonObject& json, QString& errorString) final;
    void loadFromVehicle        (void) final;
    void sendToVehicle          (void) final;
    void removeAll              (void) final;
    void removeAllFromVehicle   (void) final;
    bool syncInProgress         (void) const final;
    bool dirty                  (void) const final { return _dirty; }
    void setDirty               (bool dirty) final;
    bool containsItems          (void) const final;
    void managerVehicleChanged  (Vehicle* managerVehicle) final;

    bool                rallyPointsSupported    (void) const;
    QmlObjectListModel* points                  (void) { return &_points; }
    QString             editorQml               (void) const;
    QObject*            currentRallyPoint       (void) const { return _currentRallyPoint; }

    void setCurrentRallyPoint(QObject* rallyPoint);

signals:
    void rallyPointsSupportedChanged(bool rallyPointsSupported);
    void currentRallyPointChanged(QObject* rallyPoint);
    void loadComplete(void);

private slots:
    void _loadComplete(const QList<QGeoCoordinate> rgPoints);
    void _setFirstPointCurrent(void);
    void _updateContainsItems(void);

private:
    RallyPointManager*  _rallyPointManager;
    bool                _dirty;
    QmlObjectListModel  _points;
    QObject*            _currentRallyPoint;

    static const char* _jsonFileTypeValue;
    static const char* _jsonPointsKey;
};

#endif
