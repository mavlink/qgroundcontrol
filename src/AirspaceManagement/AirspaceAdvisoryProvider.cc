/****************************************************************************
 *
 *   (c) 2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
    case Airport:               return QString(tr("Airport")); break;
    case Controlled_airspace:   return QString(tr("Controlled Airspace")); break;
    case Special_use_airspace:  return QString(tr("Special Use Airspace")); break;
    case Tfr:                   return QString(tr("TFR")); break;
    case Wildfire:              return QString(tr("Wild Fire")); break;
    case Park:                  return QString(tr("Park")); break;
    case Power_plant:           return QString(tr("Power Plant")); break;
    case Heliport:              return QString(tr("Heliport")); break;
    case Prison:                return QString(tr("Prison")); break;
    case School:                return QString(tr("School")); break;
    case Hospital:              return QString(tr("Hospital")); break;
    case Fire:                  return QString(tr("Fire")); break;
    case Emergency:             return QString(tr("Emergency")); break;
    case Invalid:               return QString(tr("Invalid")); break;
    default:                    return QString(tr("Unknown")); break;
    }
}
