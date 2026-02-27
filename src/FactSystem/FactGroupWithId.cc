#include "FactGroupWithId.h"

FactGroupWithId::FactGroupWithId(int updateRateMsecs, const QString &metaDataFile, QObject *parent, bool ignoreCamelCase)
    : FactGroup(updateRateMsecs, metaDataFile, parent, ignoreCamelCase)
{
    _addFact(&_idFact);
}
