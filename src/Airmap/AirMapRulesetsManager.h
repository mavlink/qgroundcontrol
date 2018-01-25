/****************************************************************************
 *
 *   (c) 2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

/**
 * @file AirMapRulesetsManager.h
 * Class to download rulesets from AirMap
 */

#include <QObject>
#include "AirMapManager.h"
#include "AirspaceRulesetsProvider.h"
#include "AirMapSharedState.h"

class AirMapRulesetsManager : public AirspaceRulesetsProvider, public LifetimeChecker
{
    Q_OBJECT
public:
    AirMapRulesetsManager   (AirMapSharedState& shared);

    void setROI             (const QGeoCoordinate& center) override;

signals:
    void error              (const QString& what, const QString& airmapdMessage, const QString& airmapdDetails);

private:
    enum class State {
        Idle,
        RetrieveItems,
    };
    State                   _state = State::Idle;
    AirMapSharedState&      _shared;
};


