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
    case Airport:               return QString(tr("Airport"));
    case Controlled_airspace:   return QString(tr("Controlled Airspace"));
    case Special_use_airspace:  return QString(tr("Special Use Airspace"));
    case Tfr:                   return QString(tr("TFR"));
    case Wildfire:              return QString(tr("Wild Fire"));
    case Park:                  return QString(tr("Park"));
    case Power_plant:           return QString(tr("Power Plant"));
    case Heliport:              return QString(tr("Heliport"));
    case Prison:                return QString(tr("Prison"));
    case School:                return QString(tr("School"));
    case Hospital:              return QString(tr("Hospital"));
    case Fire:                  return QString(tr("Fire"));
    case Emergency:             return QString(tr("Emergency"));
    case Invalid:               return QString(tr("Custom"));
    default:                    return QString(tr("Unknown"));
    }
}
