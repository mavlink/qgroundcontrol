// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QV4SEQUENCEWRAPPER_P_H
#define QV4SEQUENCEWRAPPER_P_H

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

#include <QtCore/qglobal.h>
#include <QtCore/qvariant.h>
#include <QtQml/qqml.h>

#include <private/qv4referenceobject_p.h>
#include <private/qv4value_p.h>
#include <private/qv4object_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

struct Sequence;
struct SequenceOwnPropertyKeyIterator;

struct Q_QML_EXPORT SequencePrototype : public QV4::Object
{
    V4_PROTOTYPE(arrayPrototype)
    void init();

    static ReturnedValue method_valueOf(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_shift(const FunctionObject *b, const Value *thisObject, const Value *, int);
    static ReturnedValue method_getLength(
            const FunctionObject *b, const Value *thisObject, const Value *, int);
    static ReturnedValue method_setLength(
            const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);

    static ReturnedValue newSequence(
        QV4::ExecutionEngine *engine, QMetaType type, QMetaSequence metaSequence, const void *data,
        Heap::Object *object, int propertyIndex, Heap::ReferenceObject::Flags flags);
    static ReturnedValue fromVariant(QV4::ExecutionEngine *engine, const QVariant &vd);
    static ReturnedValue fromData(
        QV4::ExecutionEngine *engine, QMetaType type, QMetaSequence metaSequence, const void *data);

    static QMetaType metaTypeForSequence(const Sequence *object);
    static QVariant toVariant(const Sequence *object);
    static QVariant toVariant(const Value &array, QMetaType targetType);

    enum RawCopyResult
    {
        Copied,
        WasEqual,
        TypeMismatch
    };

    static void *rawContainerPtr(const Sequence *sequence, QMetaType typeHint);
    static RawCopyResult setRawContainer(
            Sequence *sequence, const void *container, QMetaType typeHint);
    static RawCopyResult getRawContainer(
            const Sequence *sequence, void *container, QMetaType typeHint);
};

namespace Heap {

struct Sequence : ReferenceObject
{
    void init(QMetaType listType, QMetaSequence metaSequence, const void *container);
    void init(QMetaType listType, QMetaSequence metaSequence, const void *container,
              Object *object, int propertyIndex, Heap::ReferenceObject::Flags flags);

    Sequence *detached();
    void destroy();

    void *storagePointer();
    const void *storagePointer() const { return m_container; }

    bool isStoredInline() const
    {
        if (isReference())
            return true;

        const QMetaType valueType = valueMetaType();
        switch (valueType.id()) {
        case QMetaType::QVariant:
        case QMetaType::QVariantHash:
        case QMetaType::QVariantMap:
        case QMetaType::QVariantList:
        case QMetaType::QObjectStar:
            return false;
        default:
            break;
        }

        return !valueType.flags().testFlag(QMetaType::PointerToQObject);
    }

    bool isReadOnly() const { return m_object && !canWriteBack(); }

    bool setVariant(const QVariant &variant);
    QVariant toVariant() const;

    QMetaType listType() const { return QMetaType(m_listType); }
    QMetaType valueMetaType() const { return QMetaType(m_metaSequence->valueMetaType); }
    QMetaSequence metaSequence() const { return QMetaSequence(m_metaSequence); }

private:
    friend struct QV4::Sequence;
    friend struct QV4::SequencePrototype;
    friend struct QV4::SequenceOwnPropertyKeyIterator;

    void initTypes(QMetaType listType, QMetaSequence metaSequence);
    void createElementWrappers(const void *container);
    void createInlineStorage(const void *container);

    bool loadReference();
    bool storeReference();

    union {
        void *m_container; // if stored inline
        uint m_size;       // if stored out of line
    };
    const QtPrivate::QMetaTypeInterface *m_listType;
    const QtMetaContainerPrivate::QMetaSequenceInterface *m_metaSequence;
};

}

struct Q_QML_EXPORT Sequence : public QV4::ReferenceObject
{
    V4_OBJECT2(Sequence, QV4::ReferenceObject)
    Q_MANAGED_TYPE(V4Sequence)
    V4_PROTOTYPE(sequencePrototype)
    V4_NEEDS_DESTROY
public:
    static QV4::ReturnedValue virtualGet(
            const QV4::Managed *that, PropertyKey id, const Value *receiver, bool *hasProperty);
    static qint64 virtualGetLength(const Managed *m);
    static bool virtualPut(Managed *that, PropertyKey id, const QV4::Value &value, Value *receiver);
    static bool virtualDeleteProperty(QV4::Managed *that, PropertyKey id);
    static bool virtualIsEqualTo(Managed *that, Managed *other);
    static QV4::OwnPropertyKeyIterator *virtualOwnPropertyKeys(const Object *m, Value *target);
    static int virtualMetacall(Object *object, QMetaObject::Call call, int index, void **a);
};

}

#define QT_DECLARE_SEQUENTIAL_CONTAINER(LOCAL, FOREIGN, VALUE) \
    struct LOCAL \
    { \
        Q_GADGET \
        QML_ANONYMOUS \
        QML_SEQUENTIAL_CONTAINER(VALUE) \
        QML_FOREIGN(FOREIGN) \
        QML_ADDED_IN_VERSION(2, 0) \
    }

// We use the original QT_COORD_TYPE name because that will match up with relevant other
// types in plugins.qmltypes (if you use either float or double, that is; otherwise you're
// on your own).
#ifdef QT_COORD_TYPE
QT_DECLARE_SEQUENTIAL_CONTAINER(QStdRealVectorForeign, std::vector<qreal>, QT_COORD_TYPE);
QT_DECLARE_SEQUENTIAL_CONTAINER(QRealListForeign, QList<qreal>, QT_COORD_TYPE);
#else
QT_DECLARE_SEQUENTIAL_CONTAINER(QRealStdVectorForeign, std::vector<qreal>, double);
QT_DECLARE_SEQUENTIAL_CONTAINER(QRealListForeign, QList<qreal>, double);
#endif

QT_DECLARE_SEQUENTIAL_CONTAINER(QDoubleStdVectorForeign, std::vector<double>, double);
QT_DECLARE_SEQUENTIAL_CONTAINER(QFloatStdVectorForeign, std::vector<float>, float);
QT_DECLARE_SEQUENTIAL_CONTAINER(QIntStdVectorForeign, std::vector<int>, int);
QT_DECLARE_SEQUENTIAL_CONTAINER(QBoolStdVectorForeign, std::vector<bool>, bool);
QT_DECLARE_SEQUENTIAL_CONTAINER(QStringStdVectorForeign, std::vector<QString>, QString);
QT_DECLARE_SEQUENTIAL_CONTAINER(QUrlStdVectorForeign, std::vector<QUrl>, QUrl);

#undef QT_DECLARE_SEQUENTIAL_CONTAINER

QT_END_NAMESPACE

#endif // QV4SEQUENCEWRAPPER_P_H
