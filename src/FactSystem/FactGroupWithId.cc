/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "FactGroupWithId.h"

FactGroupWithId::FactGroupWithId(int updateRateMsecs, const QString &metaDataFile, QObject *parent, bool ignoreCamelCase)
    : FactGroup(updateRateMsecs, metaDataFile, parent, ignoreCamelCase)
{
    _addFact(&_idFact);
}
