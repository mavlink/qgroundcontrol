// Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef QJSVALUE_P_H
#define QJSVALUE_P_H

#include <qjsvalue.h>
#include <private/qtqmlglobal_p.h>
#include <private/qv4value_p.h>
#include <private/qv4string_p.h>
#include <private/qv4engine_p.h>
#include <private/qv4mm_p.h>
#include <private/qv4persistent_p.h>

#include <QtCore/qthread.h>

QT_BEGIN_NAMESPACE

class QJSValuePrivate
{
    static constexpr quint64 s_tagBits = 3; // 3 bits mask
    static constexpr quint64 s_tagMask = (1 << s_tagBits) - 1;

    static constexpr quint64 s_pointerBit = 0x1;

public:
    enum class Kind {
        Undefined   = 0x0,
        Null        = 0x2,
        IntValue    = 0x4,
        BoolValue   = 0x6,
        DoublePtr   = 0x0 | s_pointerBit,
        QV4ValuePtr = 0x2 | s_pointerBit,
        QStringPtr  = 0x4 | s_pointerBit,
    };

    static_assert(quint64(Kind::Undefined)   <= s_tagMask);
    static_assert(quint64(Kind::Null)        <= s_tagMask);
    static_assert(quint64(Kind::IntValue)    <= s_tagMask);
    static_assert(quint64(Kind::BoolValue)   <= s_tagMask);
    static_assert(quint64(Kind::DoublePtr)   <= s_tagMask);
    static_assert(quint64(Kind::QV4ValuePtr) <= s_tagMask);
    static_assert(quint64(Kind::QStringPtr)  <= s_tagMask);

    static Kind tag(quint64 raw) { return Kind(raw & s_tagMask); }

#if QT_POINTER_SIZE == 4
    static void *pointer(quint64 raw)
    {
        Q_ASSERT(quint64(tag(raw)) & s_pointerBit);
        return reinterpret_cast<void *>(raw >> 32);
    }

    static quint64 encodePointer(void *pointer, Kind tag)
    {
        Q_ASSERT(quint64(tag) & s_pointerBit);
        return (quint64(quintptr(pointer)) << 32) | quint64(tag);
    }
#else
    static constexpr quint64 s_minAlignment = 1 << s_tagBits;
    static_assert(alignof(double)     >= s_minAlignment);
    static_assert(alignof(QV4::Value) >= s_minAlignment);
    static_assert(alignof(QString)    >= s_minAlignment);

    static void *pointer(quint64 raw)
    {
        Q_ASSERT(quint64(tag(raw)) & s_pointerBit);
        return reinterpret_cast<void *>(raw & ~s_tagMask);
    }

    static quint64 encodePointer(void *pointer, Kind tag)
    {
        Q_ASSERT(quint64(tag) & s_pointerBit);
        return quintptr(pointer) | quint64(tag);
    }
#endif

    static quint64 encodeUndefined()
    {
        return quint64(Kind::Undefined);
    }

    static quint64 encodeNull()
    {
        return quint64(Kind::Null);
    }

    static int intValue(quint64 v)
    {
        Q_ASSERT(tag(v) == Kind::IntValue);
        return v >> 32;
    }

    static quint64 encode(int intValue)
    {
        return (quint64(intValue) << 32) | quint64(Kind::IntValue);
    }

    static quint64 encode(uint uintValue)
    {
        return (uintValue < uint(std::numeric_limits<int>::max()))
                ? encode(int(uintValue))
                : encode(double(uintValue));
    }

    static bool boolValue(quint64 v)
    {
        Q_ASSERT(tag(v) == Kind::BoolValue);
        return v >> 32;
    }

    static quint64 encode(bool boolValue)
    {
        return (quint64(boolValue) << 32) | quint64(Kind::BoolValue);
    }

    static double *doublePtr(quint64 v)
    {
        Q_ASSERT(tag(v) == Kind::DoublePtr);
        return static_cast<double *>(pointer(v));
    }

    static quint64 encode(double doubleValue)
    {
        return encodePointer(new double(doubleValue), Kind::DoublePtr);
    }

    static QV4::Value *qv4ValuePtr(quint64 v)
    {
        Q_ASSERT(tag(v) == Kind::QV4ValuePtr);
        return static_cast<QV4::Value *>(pointer(v));
    }

    static quint64 encode(const QV4::Value &qv4Value)
    {
        switch (qv4Value.type()) {
        case QV4::StaticValue::Boolean_Type:
            return encode(qv4Value.booleanValue());
        case QV4::StaticValue::Integer_Type:
            return encode(qv4Value.integerValue());
        case QV4::StaticValue::Managed_Type: {
            auto managed = qv4Value.as<QV4::Managed>();
            auto engine = managed->engine();
            auto mm = engine->memoryManager;
            QV4::Value *m = mm->m_persistentValues->allocate();
            Q_ASSERT(m);
            // we create a new strong reference to the heap managed object
            // to avoid having to rescan the persistent values, we mark it here
            QV4::WriteBarrier::markCustom(engine, [&](QV4::MarkStack *stack){
                if constexpr (QV4::WriteBarrier::isInsertionBarrier)
                    managed->heapObject()->mark(stack);
            });
            *m = qv4Value;
            return encodePointer(m, Kind::QV4ValuePtr);
        }
        case QV4::StaticValue::Double_Type:
            return encode(qv4Value.doubleValue());
        case QV4::StaticValue::Null_Type:
            return encodeNull();
        case QV4::StaticValue::Empty_Type:
            Q_UNREACHABLE();
            break;
        case QV4::StaticValue::Undefined_Type:
            break;
        }

        return encodeUndefined();
    }

    static QString *qStringPtr(quint64 v)
    {
        Q_ASSERT(tag(v) == Kind::QStringPtr);
        return static_cast<QString *>(pointer(v));
    }

    static quint64 encode(QString stringValue)
    {
        return encodePointer(new QString(std::move(stringValue)), Kind::QStringPtr);
    }

    static quint64 encode(QLatin1String stringValue)
    {
        return encodePointer(new QString(std::move(stringValue)), Kind::QStringPtr);
    }

    static QJSValue fromReturnedValue(QV4::ReturnedValue d)
    {
        QJSValue result;
        setValue(&result, QV4::Value::fromReturnedValue(d));
        return result;
    }

    template<typename T>
    static const T *asManagedType(const QJSValue *jsval)
    {
        if (tag(jsval->d) == Kind::QV4ValuePtr) {
            if (const QV4::Value *value = qv4ValuePtr(jsval->d))
                return value->as<T>();
        }
        return nullptr;
    }

    // This is a move operation and transfers ownership.
    static QV4::Value *takeManagedValue(QJSValue *jsval)
    {
        if (tag(jsval->d) == Kind::QV4ValuePtr) {
            if (QV4::Value *value = qv4ValuePtr(jsval->d)) {
                jsval->d = encodeUndefined();
                return value;
            }
        }
        return nullptr;
    }

    static QV4::ReturnedValue asPrimitiveType(const QJSValue *jsval)
    {
        switch (tag(jsval->d)) {
        case Kind::BoolValue:
            return QV4::Encode(boolValue(jsval->d));
        case Kind::IntValue:
            return QV4::Encode(intValue(jsval->d));
        case Kind::DoublePtr:
            return QV4::Encode(*doublePtr(jsval->d));
        case Kind::Null:
            return QV4::Encode::null();
        case Kind::Undefined:
        case Kind::QV4ValuePtr:
        case Kind::QStringPtr:
            break;
        }

        return QV4::Encode::undefined();
    }

    // Beware: This only returns a non-null string if the QJSValue actually holds one.
    //         QV4::Strings are kept as managed values. Retrieve those with getValue().
    static const QString *asQString(const QJSValue *jsval)
    {
        if (tag(jsval->d) == Kind::QStringPtr) {
            if (const QString *string = qStringPtr(jsval->d))
                return string;
        }
        return nullptr;
    }

    static QV4::ReturnedValue asReturnedValue(const QJSValue *jsval)
    {
        switch (tag(jsval->d)) {
        case Kind::BoolValue:
            return QV4::Encode(boolValue(jsval->d));
        case Kind::IntValue:
            return QV4::Encode(intValue(jsval->d));
        case Kind::DoublePtr:
            return QV4::Encode(*doublePtr(jsval->d));
        case Kind::Null:
            return QV4::Encode::null();
        case Kind::QV4ValuePtr:
            return qv4ValuePtr(jsval->d)->asReturnedValue();
        case Kind::Undefined:
        case Kind::QStringPtr:
            break;
        }

        return QV4::Encode::undefined();
    }

    static void setString(QJSValue *jsval, QString s)
    {
        jsval->d = encode(std::move(s));
    }

    // Only use this with an existing persistent value.
    // Ownership is transferred to the QJSValue.
    static void adoptPersistentValue(QJSValue *jsval, QV4::Value *v)
    {
        jsval->d = encodePointer(v, Kind::QV4ValuePtr);
    }

    static void setValue(QJSValue *jsval, const QV4::Value &v)
    {
        jsval->d = encode(v);
    }

    // Moves any QString onto the V4 heap, changing the value to reflect that.
    static void manageStringOnV4Heap(QV4::ExecutionEngine *e, QJSValue *jsval)
    {
        if (const QString *string = asQString(jsval)) {
            jsval->d = encode(QV4::Value::fromHeapObject(e->newString(*string)));
            delete string;
        }
    }

    // Converts any QString on the fly, involving an allocation.
    // Does not change the value.
    static QV4::ReturnedValue convertToReturnedValue(QV4::ExecutionEngine *e,
                                                     const QJSValue &jsval)
    {
        if (const QString *string = asQString(&jsval))
            return e->newString(*string)->asReturnedValue();
        if (const QV4::Value *val = asManagedType<QV4::Managed>(&jsval)) {
            if (QV4::PersistentValueStorage::getEngine(val) == e)
                return val->asReturnedValue();

            qWarning("JSValue can't be reassigned to another engine.");
            return QV4::Encode::undefined();
        }
        return asPrimitiveType(&jsval);
    }

    static QV4::ExecutionEngine *engine(const QJSValue *jsval)
    {
        if (tag(jsval->d) == Kind::QV4ValuePtr) {
            if (const QV4::Value *value = qv4ValuePtr(jsval->d))
                return QV4::PersistentValueStorage::getEngine(value);
        }

        return nullptr;
    }

    static bool checkEngine(QV4::ExecutionEngine *e, const QJSValue &jsval)
    {
        QV4::ExecutionEngine *v4 = engine(&jsval);
        return !v4 || v4 == e;
    }

    static void free(QJSValue *jsval)
    {
        switch (tag(jsval->d)) {
        case Kind::Undefined:
        case Kind::Null:
        case Kind::IntValue:
        case Kind::BoolValue:
            return;
        case Kind::DoublePtr:
            delete doublePtr(jsval->d);
            return;
        case Kind::QStringPtr:
            delete qStringPtr(jsval->d);
            return;
        case Kind::QV4ValuePtr:
            break;
        }

        // We need a mutable value for free(). It needs to write to the actual memory.
        QV4::Value *m = qv4ValuePtr(jsval->d);
        Q_ASSERT(m); // Otherwise it would have been undefined above.
        if (QV4::ExecutionEngine *e = QV4::PersistentValueStorage::getEngine(m)) {
            if (QJSEngine *jsEngine = e->jsEngine()) {
                if (jsEngine->thread() != QThread::currentThread()) {
                    QMetaObject::invokeMethod(
                            jsEngine, [m](){ QV4::PersistentValueStorage::free(m); });
                    return;
                }
            }
        }
        QV4::PersistentValueStorage::free(m);
    }
};

QT_END_NAMESPACE

#endif
