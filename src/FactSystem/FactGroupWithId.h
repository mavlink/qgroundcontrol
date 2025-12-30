/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "FactGroup.h"

/// FactGroupWithId is a FactGroup which has an id Fact which can be used to identify the group.
/// It is mainly used in combination with FactGroupListModel to manage dynamic FactGroups.

class FactGroupWithId : public FactGroup
{
    Q_OBJECT

    Q_PROPERTY(Fact *id READ id CONSTANT)

public:
    explicit FactGroupWithId(int updateRateMsecs, const QString &metaDataFile, QObject *parent = nullptr, bool ignoreCamelCase = false);

    Fact *id() { return &_idFact; }

protected:
    Fact _idFact = Fact(0, QStringLiteral("id"), FactMetaData::valueTypeUint32);
};
