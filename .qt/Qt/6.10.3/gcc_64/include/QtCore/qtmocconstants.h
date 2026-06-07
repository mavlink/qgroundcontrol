// Copyright (C) 2024 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTMOCCONSTANTS_H
#define QTMOCCONSTANTS_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists to be used by the code that
// moc generates. This file will not change quickly, but it over the long term,
// it will likely change or even be removed.
//
// We mean it.
//

#include <QtCore/qflags.h>
#include <QtCore/qtconfigmacros.h>
#include <QtCore/qtypes.h>

QT_BEGIN_NAMESPACE

namespace QtMocConstants {
// revision 7 is Qt 5.0 everything lower is not supported
// revision 8 is Qt 5.12: It adds the enum name to QMetaEnum
// revision 9 is Qt 6.0: It adds the metatype of properties and methods
// revision 10 is Qt 6.2: The metatype of the metaobject is stored in the metatypes array
//                        and metamethods store a flag stating whether they are const
// revision 11 is Qt 6.5: The metatype for void is stored in the metatypes array
// revision 12 is Qt 6.6: It adds the metatype for enums
// revision 13 is Qt 6.9: Adds support for 64-bit QFlags and moves the method revision
enum { OutputRevision = 13 };   // Used by moc, qmetaobjectbuilder and qdbus

enum PropertyFlags : uint {
    Invalid = 0x00000000,
    Readable = 0x00000001,
    Writable = 0x00000002,
    Resettable = 0x00000004,
    EnumOrFlag = 0x00000008,
    Alias = 0x00000010,
    // Reserved for future usage = 0x00000020,
    StdCppSet = 0x00000100,
    Constant = 0x00000400,
    Final = 0x00000800,
    Designable = 0x00001000,
    Scriptable = 0x00004000,
    Stored = 0x00010000,
    User = 0x00100000,
    Required = 0x01000000,
    Bindable = 0x02000000,
};
inline constexpr PropertyFlags DefaultPropertyFlags { Readable | Designable | Scriptable | Stored };

enum MethodFlags : uint {
    AccessPrivate = 0x00,
    AccessProtected = 0x01,
    AccessPublic = 0x02,
    AccessMask = 0x03, // mask

    MethodMethod = 0x00,
    MethodSignal = 0x04,
    MethodSlot = 0x08,
    MethodConstructor = 0x0c,
    MethodTypeMask = 0x0c,

    MethodCompatibility = 0x10,
    MethodCloned = 0x20,
    MethodScriptable = 0x40,
    MethodRevisioned = 0x80,

    MethodIsConst = 0x100, // no use case for volatile so far
};

enum MetaObjectFlag : uint {
    DynamicMetaObject = 0x01,               // is derived from QAbstractDynamicMetaObject
    RequiresVariantMetaObject = 0x02,
    PropertyAccessInStaticMetaCall = 0x04,  // since Qt 5.5, property code is in the static metacall
    AllocatedMetaObject = 0x08,             // meta object was allocated in dynamic memory (and may be freed)
};

enum MetaDataFlags : uint {
    IsUnresolvedType = 0x80000000,
    TypeNameIndexMask = 0x7FFFFFFF,
    IsUnresolvedSignal = 0x70000000,
};

enum EnumFlags : uint {
    EnumIsFlag = 0x1,
    EnumIsScoped = 0x2,
    EnumIs64Bit = 0x40,
};

} // namespace QtMocConstants

QT_END_NAMESPACE

#endif // QTMOCCONSTANTS_H
