/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AirspaceAdvisoryProvider.h"

AirspaceAdvisory::AirspaceAdvisory(QObject* parent)
    : QObject(parent)
{
}

AirspaceAdvisoryProvider::AirspaceAdvisoryProvider(QObject *parent)
    : QObject(parent)
{
}

//-- TODO: This enum is a bitmask, which implies an airspace can be any
//   combination of these types. However, I have not seen any that this
//   was the case.

QString
AirspaceAdvisory::typeStr()
{
    switch(type()) {
    case Airport:               return tr("Airport");
    case Controlled_airspace:   return tr("Controlled Airspace");
    case Special_use_airspace:  return tr("Special Use Airspace");
    case Tfr:                   return tr("TFR");
    case Wildfire:              return tr("Wild Fire");
    case Park:                  return tr("Park");
    case Power_plant:           return tr("Power Plant");
    case Heliport:              return tr("Heliport");
    case Prison:                return tr("Prison");
    case School:                return tr("School");
    case Hospital:              return tr("Hospital");
    case Fire:                  return tr("Fire");
    case Emergency:             return tr("Emergency");
    case Invalid:               return tr("Custom");
    default:                    return tr("Unknown");
    }
}
