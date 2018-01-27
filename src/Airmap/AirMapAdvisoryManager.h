/****************************************************************************
 *
 *   (c) 2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

#include <QGeoCoordinate>

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
public:
    AirMapAdvisory (QObject* parent = NULL);
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
    AirMapAdvisoryManager                    (AirMapSharedState &shared, QObject *parent = nullptr);
    bool                valid           () override { return _valid; }
    AdvisoryColor       airspaceColor   () override { return _airspaceColor; }
    QmlObjectListModel* airspaces       () override { return &_airspaces; }
    void                setROI          (const QGeoCoordinate& center, double radiusMeters) override;
signals:
    void                error           (const QString& what, const QString& airmapdMessage, const QString& airmapdDetails);
private:
    void            _requestAdvisories  (const QGeoCoordinate& coordinate, double radiusMeters);
private:
    bool                _valid;
    AirMapSharedState&  _shared;
    QGeoCoordinate      _lastRoiCenter;
    QmlObjectListModel  _airspaces;
    AdvisoryColor       _airspaceColor;
};
