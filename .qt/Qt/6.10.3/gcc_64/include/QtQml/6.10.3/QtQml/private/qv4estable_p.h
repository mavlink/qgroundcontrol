// Copyright (C) 2018 Crimson AS <info@crimson.no>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

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

#ifndef QV4ESTABLE_P_H
#define QV4ESTABLE_P_H

#include <vector>
#include <limits>

#include <QtCore/q20vector.h>

#include "qv4value_p.h"

class tst_qv4estable;

QT_BEGIN_NAMESPACE

namespace QV4 {

class Q_AUTOTEST_EXPORT ESTable
{
public:
    // Can be used to observe changes in the position of the element at index pivot by registering an instance
    // with `observeShifts`.
    // This is used by implementations of `forEach`, for `ESTable`
    // backed collections, to respect the correct order of iteration
    // in the face of a `callbackFn` that mutates the collection
    // itself.
    struct ShiftObserver {
        static constexpr uint OUT_OF_TABLE = std::numeric_limits<uint>::max();

        uint pivot = 0;

        void next() {
            pivot = pivot == OUT_OF_TABLE ? 0 : pivot + 1;
        }
    };

public:
    ESTable();
    ~ESTable();

    void markObjects(MarkStack *s, bool isWeakMap);
    void clear();
    void set(const Value &k, const Value &v);
    bool has(const Value &k) const;
    ReturnedValue get(const Value &k, bool *hasValue = nullptr) const;
    bool remove(const Value &k);
    uint size() const;
    void iterate(uint idx, Value *k, Value *v);

    void removeUnmarkedKeys();

    inline void observeShifts(ShiftObserver& observer) {
        if (std::find(m_observers.cbegin(), m_observers.cend(), &observer) == m_observers.cend())
            m_observers.push_back(&observer);
    }
    inline void stopObservingShifts(ShiftObserver& observer) {
        q20::erase(m_observers, &observer);
    }

private:
    friend class ::tst_qv4estable;

    Value *m_keys = nullptr;
    Value *m_values = nullptr;
    uint m_size = 0;
    uint m_capacity = 0;

    std::vector<ShiftObserver*> m_observers;
};

} // namespace QV4

QT_END_NAMESPACE

#endif
