// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QV4CALLDATA_P_H
#define QV4CALLDATA_P_H

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

#include <private/qv4staticvalue_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

struct CallData
{
    enum Offsets {
        Function = 0,
        Context = 1,
        Accumulator = 2,
        This = 3,
        NewTarget = 4,
        Argc = 5,

        LastOffset = Argc,
        OffsetCount = LastOffset + 1
    };

    StaticValue function;
    StaticValue context;
    StaticValue accumulator;
    StaticValue thisObject;
    StaticValue newTarget;
    StaticValue _argc;

    int argc() const {
        Q_ASSERT(_argc.isInteger());
        return _argc.int_32();
    }

    void setArgc(int argc) {
        Q_ASSERT(argc >= 0);
        _argc.setInt_32(argc);
    }

    inline ReturnedValue argument(int i) const {
        return i < argc() ? args[i].asReturnedValue()
                          : StaticValue::undefinedValue().asReturnedValue();
    }

    StaticValue args[1];

    static constexpr int HeaderSize()
    {
        return offsetof(CallData, args) / sizeof(QV4::StaticValue);
    }

    template<typename Value>
    Value *argValues();

    template<typename Value>
    const Value *argValues() const;
};

Q_STATIC_ASSERT(std::is_standard_layout<CallData>::value);
Q_STATIC_ASSERT(offsetof(CallData, function   ) == CallData::Function    * sizeof(StaticValue));
Q_STATIC_ASSERT(offsetof(CallData, context    ) == CallData::Context     * sizeof(StaticValue));
Q_STATIC_ASSERT(offsetof(CallData, accumulator) == CallData::Accumulator * sizeof(StaticValue));
Q_STATIC_ASSERT(offsetof(CallData, thisObject ) == CallData::This        * sizeof(StaticValue));
Q_STATIC_ASSERT(offsetof(CallData, newTarget  ) == CallData::NewTarget   * sizeof(StaticValue));
Q_STATIC_ASSERT(offsetof(CallData, _argc      ) == CallData::Argc        * sizeof(StaticValue));
Q_STATIC_ASSERT(offsetof(CallData, args       ) == 6 * sizeof(StaticValue));

} // namespace QV4

QT_END_NAMESPACE

#endif // QV4CALLDATA_P_H
