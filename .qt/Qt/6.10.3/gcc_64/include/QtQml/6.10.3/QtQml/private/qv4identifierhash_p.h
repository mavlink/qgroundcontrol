// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant
#ifndef QV4IDENTIFIERHASH_P_H
#define QV4IDENTIFIERHASH_P_H

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

#include <qstring.h>
#include <private/qv4global_p.h>
#include <private/qv4propertykey_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

struct IdentifierHashEntry;
struct IdentifierHashData;
struct Q_QML_EXPORT IdentifierHash
{
    IdentifierHash() = default;
    IdentifierHash(ExecutionEngine *engine);
    IdentifierHash(const IdentifierHash &other);
    ~IdentifierHash();
    IdentifierHash &operator=(const IdentifierHash &other);

    bool isEmpty() const { return !d; }

    int count() const;

    void detach();

    void add(const QString &str, int value);
    void add(Heap::String *str, int value);

    int value(const QString &str) const;
    int value(String *str) const;
    QString findId(int value) const;

private:
    inline IdentifierHashEntry *addEntry(PropertyKey i);
    inline const IdentifierHashEntry *lookup(PropertyKey identifier) const;
    inline const IdentifierHashEntry *lookup(const QString &str) const;
    inline const IdentifierHashEntry *lookup(String *str) const;
    inline const PropertyKey toIdentifier(const QString &str) const;
    inline const PropertyKey toIdentifier(Heap::String *str) const;

    IdentifierHashData *d = nullptr;
};

} // namespace QV4

QT_END_NAMESPACE

#endif // QV4_IDENTIFIERHASH_P_H
