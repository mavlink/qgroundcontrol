// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QV4VARIANTOBJECT_P_H
#define QV4VARIANTOBJECT_P_H

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
#include <QtQml/qqmllist.h>
#include <QtCore/qvariant.h>

#include <private/qv4value_p.h>
#include <private/qv4object_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

namespace Heap {

struct VariantObject : Object
{
    void init();
    void init(const QMetaType type, const void *data);
    void destroy() {
        Q_ASSERT(scarceData);
        if (isScarce())
            addVmePropertyReference();
        delete scarceData;
        Object::destroy();
    }
    bool isScarce() const;
    int vmePropertyReferenceCount;

    const QVariant &data() const { return scarceData->data; }
    QVariant &data() { return scarceData->data; }

    void addVmePropertyReference() { scarceData->node.remove(); }
    void removeVmePropertyReference() { internalClass->engine->scarceResources.insert(scarceData); }

private:
    ExecutionEngine::ScarceResourceData *scarceData;
};

}

struct Q_QML_EXPORT VariantObject : Object
{
    V4_OBJECT2(VariantObject, Object)
    V4_PROTOTYPE(variantPrototype)
    V4_NEEDS_DESTROY

    void addVmePropertyReference() const;
    void removeVmePropertyReference() const;

protected:
    static bool virtualIsEqualTo(Managed *m, Managed *other);
};

struct VariantPrototype : VariantObject
{
public:
    V4_PROTOTYPE(objectPrototype)
    void init();

    static ReturnedValue method_preserve(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_destroy(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_toString(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_valueOf(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
};

}

QT_END_NAMESPACE

#endif

