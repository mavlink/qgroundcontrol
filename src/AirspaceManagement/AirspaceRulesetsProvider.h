/****************************************************************************
 *
 *   (c) 2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

//-----------------------------------------------------------------------------
/**
 * @class AirspaceRulesetsProvider
 * Base class that queries for airspace rulesets
 */

#include <QObject>
#include <QGeoCoordinate>

class AirspaceRulesetsProvider : public QObject {
    Q_OBJECT
public:
    AirspaceRulesetsProvider    (QObject* parent = NULL);
    ~AirspaceRulesetsProvider   () = default;
    /**
     * Set region of interest that should be queried. When finished, the requestDone() signal will be emmited.
     * @param center Center coordinate for ROI
     */
    virtual void setROI         (const QGeoCoordinate& center) = 0;
signals:
    void requestDone            (bool success);
};

