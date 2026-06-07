// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QV4REFERENCEOBJECT_P_H
#define QV4REFERENCEOBJECT_P_H

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

#include <private/qv4object_p.h>
#include <private/qv4stackframe_p.h>
#include <private/qqmlnotifier_p.h>
#include <private/qv4qobjectwrapper_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {
namespace Heap {

struct ReferenceObject;
struct ReferenceObjectEndpoint : QQmlNotifierEndpoint {
    ReferenceObjectEndpoint(ReferenceObject* reference)
        : QQmlNotifierEndpoint(QQmlDirtyReferenceObject),
          reference(reference)
    {}

    ReferenceObject* reference;
};

#define ReferenceObjectMembers(class, Member) \
    Member(class, Pointer, Object *, m_object)

DECLARE_HEAP_OBJECT(ReferenceObject, Object) {
    DECLARE_MARKOBJECTS(ReferenceObject);

    enum Flag : quint8 {
        NoFlag           = 0,
        CanWriteBack     = 1 << 0,
        IsVariant        = 1 << 1,
        EnforcesLocation = 1 << 2,
        IsDirty          = 1 << 3,
        IsAlwaysDirty    = 1 << 4,
    };
    Q_DECLARE_FLAGS(Flags, Flag);

    void init(Object *object, int property, Flags flags)
    {
        setObject(object);
        m_property = property;
        m_flags = flags;

        setDirty(true);
        if (CppStackFrame *frame = internalClass->engine->currentStackFrame)
            setLocation(frame->v4Function, frame->statementNumber());
        else
            setLocation(nullptr, -1);
        Object::init();
    }

    Flags flags() const { return Flags(m_flags); }

    Object *object() const { return m_object.get(); }
    void setObject(Object *object) { m_object.set(internalClass->engine, object); }

    int property() const { return m_property; }

    bool canWriteBack() const { return hasFlag(CanWriteBack); }
    bool isVariant() const { return hasFlag(IsVariant); }
    bool enforcesLocation() const { return hasFlag(EnforcesLocation); }

    void setLocation(const Function *function, quint16 statement)
    {
        m_function = function;
        m_statementIndex = statement;
    }

    const Function *function() const { return m_function; }
    quint16 statementIndex() const { return m_statementIndex; }

    bool isAttachedToProperty() const
    {
        if (enforcesLocation()) {
            if (CppStackFrame *frame = internalClass->engine->currentStackFrame) {
                if (frame->v4Function != function() || frame->statementNumber() != statementIndex())
                    return false;
            } else {
                return false;
            }
        }

        return true;
    }

    bool isReference() const { return m_object; }

    bool isDirty() const { return hasFlag(IsDirty); }
    void setDirty(bool dirty) { setFlag(IsDirty, dirty); }

    bool isAlwaysDirty() const { return hasFlag(IsAlwaysDirty); }
    void setAlwaysDirty(bool alwaysDirty) { setFlag(IsAlwaysDirty, alwaysDirty); }

    bool isConnected() {
        return (referenceEndpoint && referenceEndpoint->isConnected()) || bindableNotifier;
    }

    void destroy() {
        // If we allocated any connection then we must have connected
        // to the destroyed signal too, and we should clean it up.
        if (referenceEndpoint || bindableNotifier) {
            QObject::disconnect(*reinterpret_cast<QMetaObject::Connection*>(&onDelete));
            std::destroy_at(reinterpret_cast<QMetaObject::Connection*>(&onDelete));
        }

        if (referenceEndpoint)
            delete referenceEndpoint;

        if (bindableNotifier)
            delete bindableNotifier;
    }

    void connectToNotifySignal(QObject *obj, int property, QQmlEngine *engine);
    void connectToBindable(QObject *obj, int property, QQmlEngine *engine);

private:

        bool hasFlag(Flag flag) const
        {
            return m_flags & quint8(flag);
        }

        void setFlag(Flag flag, bool set)
        {
            m_flags = set ? (m_flags | quint8(flag)) : (m_flags & ~quint8(flag));
        }

        const Function *m_function;
        int m_property;
        quint16 m_statementIndex;
        quint8 m_flags;
        ReferenceObjectEndpoint* referenceEndpoint;
        QPropertyNotifier* bindableNotifier;
        // We need to store an handle if we connect to the destroyed
        // signal so that we can disconnect from it. To avoid yet
        // another allocation, considering that
        // QMetaObject::Connection is not trivial, we store it in
        // block memory.
        alignas(alignof(QMetaObject::Connection))
        std::byte onDelete[sizeof(QMetaObject::Connection)];
    };

    Q_DECLARE_OPERATORS_FOR_FLAGS(ReferenceObject::Flags)

    } // namespace Heap


    struct ReferenceObject : public Object
    {
        V4_OBJECT2(ReferenceObject, Object)
        Q_MANAGED_TYPE(V4ReferenceObject)
        V4_NEEDS_DESTROY

    public:
        static constexpr const int AllProperties = -1;

        static bool shouldConnect(Heap::ReferenceObject *ref);
        static void connect(Heap::ReferenceObject *ref);

        template<typename HeapObject>
        static bool readReference(HeapObject *ref)
        {
            if (!ref->object())
                return false;

            if (!ref->isDirty())
                return true;

            QV4::Scope scope(ref->internalClass->engine);
            QV4::ScopedObject object(scope, ref->object());

            if (!ref->isConnected() && shouldConnect(ref))
                connect(ref);

            bool wasRead = false;
            if (ref->isVariant()) {
                QVariant variant;
                void *a[] = { &variant };
                wasRead = object->metacall(QMetaObject::ReadProperty, ref->property(), a)
                    && ref->setVariant(variant);
            } else {
                void *a[] = { ref->storagePointer() };
                wasRead = object->metacall(QMetaObject::ReadProperty, ref->property(), a);
            }

            ref->setDirty(!ref->isConnected() || !wasRead);
        return wasRead;
    }

    template<typename HeapObject>
    static bool writeBack(HeapObject *ref, int internalIndex = AllProperties)
    {
        if (!ref->object() || !ref->canWriteBack())
            return false;

        QV4::Scope scope(ref->internalClass->engine);
        QV4::ScopedObject object(scope, ref->object());

        int flags = QQmlPropertyData::HasInternalIndex;
        int status = -1;
        if (ref->isVariant()) {
            QVariant variant = ref->toVariant();
            void *a[] = { &variant, nullptr, &status, &flags, &internalIndex };
            return object->metacall(QMetaObject::WriteProperty, ref->property(), a);
        }

        void *a[] = { ref->storagePointer(), nullptr, &status, &flags, &internalIndex };
        return object->metacall(QMetaObject::WriteProperty, ref->property(), a);
    }

    template<typename HeapObject>
    static HeapObject *detached(HeapObject *ref)
    {
        if (ref->object() && !ref->enforcesLocation() && !readReference(ref))
            return ref; // It's dead. No point in detaching it anymore

        return ref->detached();
    }
};

} // namespace QV4

QT_END_NAMESPACE

#endif // QV4REFERENCEOBJECT_P_H
