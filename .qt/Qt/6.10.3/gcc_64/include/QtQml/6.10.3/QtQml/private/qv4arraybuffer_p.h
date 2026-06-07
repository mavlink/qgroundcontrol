// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant
#ifndef QV4ARRAYBUFFER_H
#define QV4ARRAYBUFFER_H

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

#include "qv4object_p.h"
#include "qv4functionobject_p.h"
#include <QtCore/qarraydatapointer.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

namespace Heap {

struct SharedArrayBufferCtor : FunctionObject {
    void init(QV4::ExecutionEngine *engine);
};

struct ArrayBufferCtor : SharedArrayBufferCtor {
    void init(QV4::ExecutionEngine *engine);
};

struct Q_QML_EXPORT SharedArrayBuffer : Object {
    void init(size_t length);
    void init(const QByteArray& array);
    void destroy();

    void setSharedArrayBuffer(bool shared) noexcept { isShared = shared; }
    bool isSharedArrayBuffer() const noexcept { return isShared; }

    char *arrayData() noexcept { return arrayDataPointer()->data(); }
    const char *constArrayData() const noexcept { return constArrayDataPointer()->data(); }
    uint arrayDataLength() const noexcept { return constArrayDataPointer().size; }

    bool hasSharedArrayData() const noexcept { return constArrayDataPointer().isShared(); }
    bool hasDetachedArrayData() const noexcept { return constArrayDataPointer().isNull(); }
    void detachArrayData() noexcept { arrayDataPointer().clear(); }

    bool arrayDataNeedsDetach() const noexcept { return constArrayDataPointer().needsDetach(); }

private:
    const QArrayDataPointer<const char> &constArrayDataPointer() const noexcept
    {
        return *reinterpret_cast<const QArrayDataPointer<const char> *>(&arrayDataPointerStorage);
    }
    QArrayDataPointer<char> &arrayDataPointer() noexcept
    {
        return *reinterpret_cast<QArrayDataPointer<char> *>(&arrayDataPointerStorage);
    }

    template <typename T>
    struct storage_t { alignas(T) unsigned char data[sizeof(T)]; };

    storage_t<QArrayDataPointer<char>>
    arrayDataPointerStorage;
    bool isShared;
};

struct Q_QML_EXPORT ArrayBuffer : SharedArrayBuffer {
    void init(size_t length) {
        SharedArrayBuffer::init(length);
        setSharedArrayBuffer(false);
    }
    void init(const QByteArray& array) {
        SharedArrayBuffer::init(array);
        setSharedArrayBuffer(false);
    }
};

}

struct SharedArrayBufferCtor : FunctionObject
{
    V4_OBJECT2(SharedArrayBufferCtor, FunctionObject)

    static ReturnedValue virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *);
    static ReturnedValue virtualCall(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
};

struct ArrayBufferCtor : SharedArrayBufferCtor
{
    V4_OBJECT2(ArrayBufferCtor, SharedArrayBufferCtor)

    static ReturnedValue virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *);

    static ReturnedValue method_isView(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
};

struct Q_QML_EXPORT SharedArrayBuffer : Object
{
    V4_OBJECT2(SharedArrayBuffer, Object)
    V4_NEEDS_DESTROY
    V4_PROTOTYPE(sharedArrayBufferPrototype)

    QByteArray asByteArray() const;

    uint arrayDataLength() const { return d()->arrayDataLength(); }
    char *arrayData() { return d()->arrayData(); }
    const char *constArrayData() const { return d()->constArrayData(); }

    bool hasSharedArrayData() { return d()->hasSharedArrayData(); }
    bool hasDetachedArrayData() const { return d()->hasDetachedArrayData(); }
    bool isSharedArrayBuffer() const { return d()->isSharedArrayBuffer(); }
};

struct Q_QML_EXPORT ArrayBuffer : SharedArrayBuffer
{
    V4_OBJECT2(ArrayBuffer, SharedArrayBuffer)
    V4_NEEDS_DESTROY
    V4_PROTOTYPE(arrayBufferPrototype)

    QByteArray asByteArray() const;
    uint arrayDataLength() const { return d()->arrayDataLength(); }
    char *dataData() { if (d()->arrayDataNeedsDetach()) detach(); return d()->arrayData(); }
    // ### is that detach needed?
    const char *constArrayData() const { return d()->constArrayData(); }
    bool hasSharedArrayData() { return d()->hasSharedArrayData(); }
    void detachArrayData() { d()->detachArrayData(); }

    void detach();
};

struct SharedArrayBufferPrototype : Object
{
    void init(ExecutionEngine *engine, Object *ctor);

    static ReturnedValue method_get_byteLength(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_slice(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);

    static ReturnedValue slice(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc, bool shared);
};

struct ArrayBufferPrototype : SharedArrayBufferPrototype
{
    void init(ExecutionEngine *engine, Object *ctor);

    static ReturnedValue method_get_byteLength(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_slice(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_toString(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
};


} // namespace QV4

QT_END_NAMESPACE

#endif
