// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QV4VARIANTASSOCIATIONOBJECT_P_H_
#define QV4VARIANTASSOCIATIONOBJECT_P_H_

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
#include <private/qv4referenceobject_p.h>
#include <private/qv4value_p.h>

#include <QtCore/QVariantMap>
#include <QtCore/QVariantHash>

QT_BEGIN_NAMESPACE

namespace QV4 {

    struct Q_QML_EXPORT VariantAssociationPrototype : public QV4::Object
    {
        V4_PROTOTYPE(objectPrototype);

        static ReturnedValue fromQVariantMap(
            ExecutionEngine *engine,
            const QVariantMap& variantMap,
            QV4::Heap::Object* container,
            int property, Heap::ReferenceObject::Flags flags);

        static ReturnedValue fromQVariantHash(
            ExecutionEngine *engine,
            const QVariantHash& variantHash,
            QV4::Heap::Object* container,
            int property, Heap::ReferenceObject::Flags flags);
    };

    namespace Heap {

        #define VariantAssociationObjectMembers(class, Member) \
            Member(class, Pointer, ArrayObject *, propertyIndexMapping);

        DECLARE_HEAP_OBJECT(VariantAssociationObject, ReferenceObject)
        {
            DECLARE_MARKOBJECTS(VariantAssociationObject)

            enum class AssociationType: quint8 {
                VariantMap,
                VariantHash
            };

            void init(
                const QVariantMap& variantMap,
                QV4::Heap::Object* container,
                int property, Heap::ReferenceObject::Flags flags);

            void init(
                const QVariantHash& variantHash,
                QV4::Heap::Object* container,
                int property, Heap::ReferenceObject::Flags flags);

            template<typename Association>
            void createElementWrappers(const Association &association)
            {
                Q_ASSERT(!object());

                QV4::Scope scope(internalClass->engine);
                QV4::ScopedObject self(scope, this);
                for (auto it = association.begin(), end = association.end(); it != end; ++it) {
                    QV4::ScopedString key(scope, scope.engine->newString(it.key()));
                    QV4::ScopedValue val(scope, scope.engine->fromVariant(*it));
                    self->put(key, val);
                }
            }

            QV4::ReturnedValue getElement(const QString &id, bool *hasProperty);

            void destroy();

            void *storagePointer() { return &m_variantAssociation; }

            QVariant toVariant();
            bool setVariant(const QVariant &variant);

            VariantAssociationObject *detached();

            // The alignment calculation needs to be out of the
            // `alignas` due to a GCC 8.3 bug (that at the time of
            // writing is used on the QNX 7.1 platform).
            // See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=94929
            static constexpr auto alignment =
                std::max(alignof(QVariantMap), alignof(QVariantHash));
            alignas(alignment)
            std::byte m_variantAssociation[std::max(sizeof(QVariantMap), sizeof(QVariantHash))];

            AssociationType m_type;
        };

    } // namespace Heap

    struct Q_QML_EXPORT VariantAssociationObject : public QV4::ReferenceObject
    {
        V4_OBJECT2(VariantAssociationObject, QV4::ReferenceObject);
        V4_PROTOTYPE(variantAssociationPrototype);
        V4_NEEDS_DESTROY

        static bool virtualPut(Managed *that, PropertyKey id, const QV4::Value &value,
                               Value *receiver);
        static QV4::ReturnedValue virtualGet(const QV4::Managed *that, PropertyKey id,
                                             const Value *receiver, bool *hasProperty);

        static bool virtualDeleteProperty(QV4::Managed *that, PropertyKey id);

        static QV4::OwnPropertyKeyIterator *virtualOwnPropertyKeys(const Object *m, Value *target);

        static PropertyAttributes virtualGetOwnProperty(const Managed *m, PropertyKey id,
                                                        Property *p);

        static int virtualMetacall(Object *object, QMetaObject::Call call, int index, void **a);
    };

} // namespace QV4

QT_END_NAMESPACE

#endif // QV4VARIANTASSOCIATIONOBJECT_P_H_
