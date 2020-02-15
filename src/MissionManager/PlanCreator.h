/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QString>
#include "QGeoCoordinate"

class PlanMasterController;
class MissionController;

/// Base class for PlanCreator objects which are used to create a full plan in a single step.
class PlanCreator : public QObject
{
    Q_OBJECT
    
public:
    PlanCreator(PlanMasterController* planMasterController, QString name, QString imageResource, QObject* parent = nullptr);

    Q_PROPERTY(QString  name            MEMBER _name            CONSTANT)
    Q_PROPERTY(QString  imageResource   MEMBER _imageResource   CONSTANT)

    Q_INVOKABLE virtual void createPlan(const QGeoCoordinate& mapCenterCoord) = 0;

protected:
    PlanMasterController*   _planMasterController;
    MissionController*      _missionController;
    QString                 _name;
    QString                 _imageResource;
};
