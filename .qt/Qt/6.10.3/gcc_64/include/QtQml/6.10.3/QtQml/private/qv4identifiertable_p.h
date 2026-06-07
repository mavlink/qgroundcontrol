// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant
#ifndef QV4IDENTIFIERTABLE_H
#define QV4IDENTIFIERTABLE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qv4identifierhash_p.h"
#include "qv4string_p.h"
#include "qv4engine_p.h"
#include <qset.h>
#include <limits.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

struct Q_QML_EXPORT IdentifierTable
{
    ExecutionEngine *engine;

    uint alloc;
    uint size;
    int numBits;
    Heap::StringOrSymbol **entriesByHash;
    Heap::StringOrSymbol **entriesById;

    QSet<IdentifierHashData *> idHashes;

    void addEntry(Heap::StringOrSymbol *str);

public:

    IdentifierTable(ExecutionEngine *engine, int numBits = 8);
    ~IdentifierTable();

    Heap::String *insertString(const QString &s);
    Heap::Symbol *insertSymbol(const QString &s);

    PropertyKey asPropertyKey(const Heap::String *str) {
        if (str->identifier.isValid())
            return str->identifier;
        return asPropertyKeyImpl(str);
    }
    PropertyKey asPropertyKey(const QV4::String *str) {
        return asPropertyKey(str->d());
    }

    enum KeyConversionBehavior { Default, ForceConversionToId };
    PropertyKey asPropertyKey(const QString &s, KeyConversionBehavior conversionBehavior = Default);

    PropertyKey asPropertyKeyImpl(const Heap::String *str);

    Heap::StringOrSymbol *resolveId(PropertyKey i) const;
    Heap::String *stringForId(PropertyKey i) const;
    Heap::Symbol *symbolForId(PropertyKey i) const;

    void markObjects(MarkStack *markStack);
    void sweep();

    void addIdentifierHash(IdentifierHashData *h) {
        idHashes.insert(h);
    }
    void removeIdentifierHash(IdentifierHashData *h) {
        idHashes.remove(h);
    }

private:
    Heap::String *resolveStringEntry(const QString &s, uint hash, uint subtype);
};

}

QT_END_NAMESPACE

#endif
