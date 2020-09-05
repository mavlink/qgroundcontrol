/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "ComplexMissionItem.h"
#include "MissionItem.h"
#include "Fact.h"
#include "QGCLoggingCategory.h"

Q_DECLARE_LOGGING_CATEGORY(LandingComplexItemLog)

class PlanMasterController;

// Base class for landing patterns complex items.
class LandingComplexItem : public ComplexMissionItem
{
    Q_OBJECT

public:
    LandingComplexItem(PlanMasterController* masterController, bool flyView, QObject* parent);

    Q_PROPERTY(Fact*            loiterAltitude          READ    loiterAltitude                                              CONSTANT)
    Q_PROPERTY(Fact*            loiterRadius            READ    loiterRadius                                                CONSTANT)
    Q_PROPERTY(Fact*            landingAltitude         READ    landingAltitude                                             CONSTANT)
    Q_PROPERTY(Fact*            landingHeading          READ    landingHeading                                              CONSTANT)
    Q_PROPERTY(Fact*            landingDistance         READ    landingDistance                                             CONSTANT)
    Q_PROPERTY(bool             loiterClockwise         MEMBER  _loiterClockwise                                            NOTIFY loiterClockwiseChanged)
    Q_PROPERTY(bool             altitudesAreRelative    MEMBER  _altitudesAreRelative                                       NOTIFY altitudesAreRelativeChanged)
    Q_PROPERTY(Fact*            stopTakingPhotos        READ    stopTakingPhotos                                            CONSTANT)
    Q_PROPERTY(Fact*            stopTakingVideo         READ    stopTakingVideo                                             CONSTANT)
    Q_PROPERTY(QGeoCoordinate   loiterCoordinate        READ    loiterCoordinate            WRITE setLoiterCoordinate       NOTIFY loiterCoordinateChanged)
    Q_PROPERTY(QGeoCoordinate   loiterTangentCoordinate READ    loiterTangentCoordinate                                     NOTIFY loiterTangentCoordinateChanged)
    Q_PROPERTY(QGeoCoordinate   landingCoordinate       READ    landingCoordinate           WRITE setLandingCoordinate      NOTIFY landingCoordinateChanged)
    Q_PROPERTY(bool             landingCoordSet         MEMBER _landingCoordSet                                             NOTIFY landingCoordSetChanged)

    Q_INVOKABLE void setLandingHeadingToTakeoffHeading();

    // These are virtual so the Facts can be created with their own specific FactGroup metadata
    virtual Fact* loiterAltitude          (void) = 0;
    virtual Fact* loiterRadius            (void) = 0;
    virtual Fact* landingAltitude         (void) = 0;
    virtual Fact* landingDistance         (void) = 0;
    virtual Fact* landingHeading          (void) = 0;
    virtual Fact* stopTakingPhotos        (void) = 0;
    virtual Fact* stopTakingVideo         (void) = 0;

    QGeoCoordinate  landingCoordinate       (void) const { return _landingCoordinate; }
    QGeoCoordinate  loiterCoordinate        (void) const { return _loiterCoordinate; }
    QGeoCoordinate  loiterTangentCoordinate (void) const { return _loiterTangentCoordinate; }

    void setLandingCoordinate       (const QGeoCoordinate& coordinate);
    void setLoiterCoordinate        (const QGeoCoordinate& coordinate);

    // Overrides from ComplexMissionItem
    double complexDistance(void) const final;

signals:
    void loiterCoordinateChanged        (QGeoCoordinate coordinate);
    void loiterTangentCoordinateChanged (QGeoCoordinate coordinate);
    void landingCoordinateChanged       (QGeoCoordinate coordinate);
    void landingCoordSetChanged         (bool landingCoordSet);
    void loiterClockwiseChanged         (bool loiterClockwise);
    void altitudesAreRelativeChanged    (bool altitudesAreRelative);
    void _updateFlightPathSegmentsSignal(void);

protected slots:
    void _updateFlightPathSegmentsDontCallDirectly  (void);
    void _recalcFromHeadingAndDistanceChange        (void);
    void _recalcFromCoordinateChange                (void);

protected:
    virtual void _calcGlideSlope(void) = 0;

    void    _init       (void);
    QPointF _rotatePoint(const QPointF& point, const QPointF& origin, double angle);

    int             _sequenceNumber =               0;
    bool            _dirty  =                       false;
    QGeoCoordinate  _loiterCoordinate;
    QGeoCoordinate  _loiterTangentCoordinate;
    QGeoCoordinate  _landingCoordinate;
    bool            _landingCoordSet =              false;
    bool            _ignoreRecalcSignals =          false;
    bool            _loiterClockwise =              true;
    bool            _altitudesAreRelative =         true;

private slots:
    void    _recalcFromRadiusChange(void);
};
