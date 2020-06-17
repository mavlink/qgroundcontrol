/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QmlObjectListModel.h"

#include "LifetimeChecker.h"

#include "AirspaceAdvisoryProvider.h"
#include "AirMapSharedState.h"

#include "QGCGeoBoundingCube.h"

#include "airmap/status.h"

/**
 * @file AirMapAdvisoryManager.h
 * Advisory information provided by AirMap.
 */

//-----------------------------------------------------------------------------
class AirMapAdvisory : public AirspaceAdvisory
{
    Q_OBJECT
    friend class AirMapAdvisoryManager;
    friend class AirMapFlightPlanManager;
public:
    AirMapAdvisory (QObject* parent = nullptr);
    QString         id              () override { return _id; }
    QString         name            () override { return _name; }
    AdvisoryType    type            () override { return _type; }
    QGeoCoordinate  coordinates     () override { return _coordinates; }
    qreal           radius          () override { return _radius; }
    AirspaceAdvisoryProvider::AdvisoryColor color () override { return _color; }
private:
    QString         _id;
    QString         _name;
    AdvisoryType    _type;
    QGeoCoordinate  _coordinates;
    qreal           _radius;
    AirspaceAdvisoryProvider::AdvisoryColor _color;
};

//-----------------------------------------------------------------------------
class AirMapAdvisoryManager : public AirspaceAdvisoryProvider, public LifetimeChecker
{
    Q_OBJECT
public:
    AirMapAdvisoryManager                   (AirMapSharedState &shared, QObject *parent = nullptr);
    bool                valid               () override { return _valid; }
    AdvisoryColor       airspaceColor       () override { return _airspaceColor; }
    QmlObjectListModel* advisories          () override { return &_advisories; }
    void                setROI              (const QGCGeoBoundingCube& roi, bool reset = false) override;
signals:
    void                error               (const QString& what, const QString& airmapdMessage, const QString& airmapdDetails);
private:
    void                _requestAdvisories  ();
private:
    bool                _valid;
    AirMapSharedState&  _shared;
    QGCGeoBoundingCube  _lastROI;
    QmlObjectListModel  _advisories;
    AdvisoryColor       _airspaceColor;
};
