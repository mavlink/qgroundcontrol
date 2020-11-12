/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AirspaceRulesetsProvider.h"

AirspaceRuleFeature::AirspaceRuleFeature(QObject* parent)
    : QObject(parent)
{
}

AirspaceRule::AirspaceRule(QObject *parent)
    : QObject(parent)
{
}

AirspaceRuleSet::AirspaceRuleSet(QObject* parent)
    : QObject(parent)
{
}

AirspaceRulesetsProvider::AirspaceRulesetsProvider(QObject *parent)
    : QObject(parent)
{
}
