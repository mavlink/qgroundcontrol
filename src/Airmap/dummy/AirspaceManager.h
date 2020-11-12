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
 * @file AirspaceManager.h
 * Dummy file for when airspace management is disabled
 */

#include "QGCToolbox.h"
#include <QGeoCoordinate>

//-----------------------------------------------------------------------------
/**
 * @class AirspaceManager
 * Base class for airspace management. There is one (global) instantiation of this
 */
class AirspaceManager : public QGCTool {
    Q_OBJECT
public:
    AirspaceManager(QGCApplication* app, QGCToolbox* toolbox);
    virtual ~AirspaceManager() override;

    Q_PROPERTY(QString                      providerName        READ providerName       CONSTANT)
    Q_PROPERTY(QObject*                     weatherInfo         READ weatherInfo        CONSTANT)
    Q_PROPERTY(QObject*                     advisories          READ advisories         CONSTANT)
    Q_PROPERTY(QObject*                     ruleSets            READ ruleSets           CONSTANT)
    Q_PROPERTY(QObject*                     airspaces           READ airspaces          CONSTANT)
    Q_PROPERTY(QObject*                     flightPlan          READ flightPlan         CONSTANT)
    Q_PROPERTY(bool                         airspaceVisible     READ airspaceVisible    CONSTANT)

    Q_INVOKABLE void setROI                     (const QGeoCoordinate& pointNW, const QGeoCoordinate& pointSE, bool planView, bool reset = false);

    QObject*                    weatherInfo    () { return &_dummy; }
    QObject*                    advisories     () { return &_dummy; }
    QObject*                    ruleSets       () { return &_dummy; }
    QObject*                    airspaces      () { return &_dummy; }
    QObject*                    flightPlan     () { return &_dummy; }

    void setToolbox(QGCToolbox* toolbox) override;

    virtual QString             providerName    () const { return QString("None"); }

    virtual bool                airspaceVisible () { return false; }

signals:
    void                airspaceVisibleChanged  ();

private:
    QObject                     _dummy;
};
