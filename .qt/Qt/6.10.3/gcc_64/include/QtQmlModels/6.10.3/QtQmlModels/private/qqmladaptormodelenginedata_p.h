// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant


#ifndef QQMLADAPTORMODELENGINEDATA_P_H
#define QQMLADAPTORMODELENGINEDATA_P_H

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

#include <private/qqmldelegatemodel_p_p.h>
#include <private/qmetaobjectbuilder_p.h>
#include <private/qqmlproperty_p.h>

#include <private/qv4value_p.h>
#include <private/qv4functionobject_p.h>

QT_BEGIN_NAMESPACE

class QQmlAdaptorModelEngineData : public QV4::ExecutionEngine::Deletable
{
public:
    QQmlAdaptorModelEngineData(QV4::ExecutionEngine *v4);

    QV4::ExecutionEngine *v4;
    QV4::PersistentValue listItemProto;

    static QV4::ReturnedValue get_index(const QV4::FunctionObject *f, const QV4::Value *thisObject, const QV4::Value *, int)
    {
        QV4::Scope scope(f);
        QV4::Scoped<QQmlDelegateModelItemObject> o(scope, thisObject->as<QQmlDelegateModelItemObject>());
        if (!o)
            RETURN_RESULT(scope.engine->throwTypeError(QStringLiteral("Not a valid DelegateModel object")));

        RETURN_RESULT(QV4::Encode(o->d()->item->modelIndex()));
    }

    template <typename T, typename M> static void setModelDataType(QMetaObjectBuilder *builder, M *metaType)
    {
        builder->setFlags(MetaObjectFlag::DynamicMetaObject);
        builder->setClassName(T::staticMetaObject.className());
        builder->setSuperClass(&T::staticMetaObject);
        metaType->propertyOffset = T::staticMetaObject.propertyCount();
        metaType->signalOffset = T::staticMetaObject.methodCount();
    }

    static void addProperty(
            QMetaObjectBuilder *builder, int propertyId, const QByteArray &propertyName,
            const QByteArray &propertyType, bool isWritable)
    {
        builder->addSignal("__" + QByteArray::number(propertyId) + "()");
        QMetaPropertyBuilder property = builder->addProperty(
                propertyName, propertyType, propertyId);
        property.setWritable(isWritable);
    }

    V4_DEFINE_EXTENSION(QQmlAdaptorModelEngineData, get)
};

QT_END_NAMESPACE

#endif // QQMLADAPTORMODELENGINEDATA_P_H
