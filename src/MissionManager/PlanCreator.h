#pragma once

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtPositioning/QGeoCoordinate>

#include "QGCMAVLinkTypes.h"

class PlanMasterController;
class MissionController;

/// Base class for PlanCreator objects which are used to create a full plan in a single step.
class PlanCreator : public QObject
{
    Q_OBJECT

public:
    /// @param planMasterController also used as the QObject parent
    PlanCreator(PlanMasterController* planMasterController, QString name, QString imageResource, QList<QGCMAVLinkTypes::VehicleClass_t> supportedVehicleClasses, bool blankPlan = false);

    Q_PROPERTY(QString  name            MEMBER _name            CONSTANT)
    Q_PROPERTY(QString  imageResource   MEMBER _imageResource   CONSTANT)
    Q_PROPERTY(bool     blankPlan       MEMBER _blankPlan       CONSTANT)

    Q_INVOKABLE virtual void createPlan(const QGeoCoordinate& mapCenterCoord) = 0;

    /// Returns true if this creator supports the given vehicle class
    bool supportsVehicleClass(QGCMAVLinkTypes::VehicleClass_t vehicleClass) const;

protected:
    PlanMasterController* _planMasterController;
    MissionController* _missionController;

private:
    QString _name;
    QString _imageResource;
    bool _blankPlan = false;
    QList<QGCMAVLinkTypes::VehicleClass_t> _supportedVehicleClasses;
};
