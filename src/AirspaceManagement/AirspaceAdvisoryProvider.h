/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

/**
 * @file AirspaceAdvisoryProvider.h
 * Weather information provided by the Airspace Managemement
 */

#include "QmlObjectListModel.h"
#include "QGCGeoBoundingCube.h"

#include <QObject>
#include <QGeoCoordinate>

//-----------------------------------------------------------------------------
class AirspaceAdvisoryProvider : public QObject
{
    Q_OBJECT
public:

    enum AdvisoryColor {
        Green,
        Yellow,
        Orange,
        Red
    };

    Q_ENUM(AdvisoryColor)

    AirspaceAdvisoryProvider            (QObject *parent = nullptr);
    virtual ~AirspaceAdvisoryProvider   () {}

    Q_PROPERTY(bool                 valid           READ valid          NOTIFY advisoryChanged)
    Q_PROPERTY(AdvisoryColor        airspaceColor   READ airspaceColor  NOTIFY advisoryChanged)
    Q_PROPERTY(QmlObjectListModel*  advisories      READ advisories     NOTIFY advisoryChanged)

    virtual bool                valid           () = 0;     ///< Current data is valid
    virtual AdvisoryColor       airspaceColor   () = 0;     ///< Aispace overall color
    virtual QmlObjectListModel* advisories      () = 0;     ///< List of AirspaceAdvisory

    /**
     * Set region of interest that should be queried. When finished, the advisoryChanged() signal will be emmited.
     * @param center Center coordinate for ROI
     */
    virtual void setROI (const QGCGeoBoundingCube& roi, bool reset = false) = 0;

signals:
    void advisoryChanged  ();
};

//-----------------------------------------------------------------------------
class AirspaceAdvisory : public QObject
{
    Q_OBJECT
public:

    enum AdvisoryType {
        Invalid              = 0,
        Airport              = 1 << 0,
        Controlled_airspace  = 1 << 1,
        Special_use_airspace = 1 << 2,
        Tfr                  = 1 << 3,
        Wildfire             = 1 << 4,
        Park                 = 1 << 5,
        Power_plant          = 1 << 6,
        Heliport             = 1 << 7,
        Prison               = 1 << 8,
        School               = 1 << 9,
        Hospital             = 1 << 10,
        Fire                 = 1 << 11,
        Emergency            = 1 << 12,
    };

    Q_ENUM(AdvisoryType)

    AirspaceAdvisory    (QObject* parent = nullptr);

    Q_PROPERTY(QString          id              READ id             CONSTANT)
    Q_PROPERTY(QString          name            READ name           CONSTANT)
    Q_PROPERTY(AdvisoryType     type            READ type           CONSTANT)
    Q_PROPERTY(QString          typeStr         READ typeStr        CONSTANT)
    Q_PROPERTY(QGeoCoordinate   coordinates     READ coordinates    CONSTANT)
    Q_PROPERTY(qreal            radius          READ radius         CONSTANT)

    Q_PROPERTY(AirspaceAdvisoryProvider::AdvisoryColor color READ color CONSTANT)

    virtual QString         id              () = 0;
    virtual QString         name            () = 0;
    virtual AdvisoryType    type            () = 0;
    virtual QString         typeStr         ();
    virtual QGeoCoordinate  coordinates     () = 0;
    virtual qreal           radius          () = 0;

    virtual AirspaceAdvisoryProvider::AdvisoryColor color () = 0;
};

