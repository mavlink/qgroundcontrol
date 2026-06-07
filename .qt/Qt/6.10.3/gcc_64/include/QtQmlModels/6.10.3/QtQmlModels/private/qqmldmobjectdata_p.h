// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLDMOBJECTDATA_P_H
#define QQMLDMOBJECTDATA_P_H

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

#include <private/qqmladaptormodelenginedata_p.h>
#include <private/qqmldelegatemodel_p_p.h>

#include <private/qobject_p.h>
#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

class VDMObjectDelegateDataType;
class QQmlDMObjectData : public QQmlDelegateModelItem
{
    Q_OBJECT
    Q_PROPERTY(QObject *modelData READ modelData NOTIFY modelDataChanged)
    QT_ANONYMOUS_PROPERTY(QObject * READ modelData NOTIFY modelDataChanged FINAL)
public:
    QQmlDMObjectData(
            const QQmlRefPointer<QQmlDelegateModelItemMetaType> &metaType,
            VDMObjectDelegateDataType *dataType,
            int index, int row, int column,
            QObject *object);

    void setModelData(QObject *modelData)
    {
        if (modelData == object)
            return;

        object = modelData;
        emit modelDataChanged();
    }

    QObject *modelData() const { return object; }
    QQmlRefPointer<QQmlContextData> initProxy() final;

    QPointer<QObject> object;

Q_SIGNALS:
    void modelDataChanged();
};

class VDMObjectDelegateDataType final
    : public QQmlRefCounted<VDMObjectDelegateDataType>,
      public QQmlAdaptorModel::Accessors
{
public:
    QMetaObjectBuilder builder;
    QQmlAdaptorModel *model = nullptr;
    int propertyOffset = 0;
    int signalOffset = 0;
    bool shared = false;

    VDMObjectDelegateDataType(QQmlAdaptorModel *model)
        : model(model)
        , shared(true)
    {
    }

    VDMObjectDelegateDataType(const VDMObjectDelegateDataType &type)
        : builder(type.metaObject.data(), QMetaObjectBuilder::Properties
                | QMetaObjectBuilder::Signals
                | QMetaObjectBuilder::SuperClass
                | QMetaObjectBuilder::ClassName)
        , model(type.model)
        , propertyOffset(type.propertyOffset)
        , signalOffset(type.signalOffset)
    {
        builder.setFlags(MetaObjectFlag::DynamicMetaObject);
    }

    int rowCount(const QQmlAdaptorModel &model) const override
    {
        return model.list.count();
    }

    int columnCount(const QQmlAdaptorModel &) const override
    {
        return 1;
    }

    QVariant value(const QQmlAdaptorModel &model, int index, const QString &role) const override
    {
        if (QObject *object = model.list.at(index).value<QObject *>())
            return object->property(role.toUtf8());
        return QVariant();
    }

    QQmlDelegateModelItem *createItem(
            QQmlAdaptorModel &model,
            const QQmlRefPointer<QQmlDelegateModelItemMetaType> &metaType,
            int index, int row, int column) override
    {
        if (!metaObject)
            initializeMetaObject();
        return index >= 0 && index < model.list.count()
                ? new QQmlDMObjectData(metaType, this, index, row, column, qvariant_cast<QObject *>(model.list.at(index)))
                : nullptr;
    }

    void initializeMetaObject()
    {
        QQmlAdaptorModelEngineData::setModelDataType<QQmlDMObjectData>(&builder, this);

        metaObject.reset(builder.toMetaObject());
        // Note: ATM we cannot create a shared property cache for this class, since each model
        // object can have different properties. And to make those properties available to the
        // delegate, QQmlDMObjectData makes use of a QAbstractDynamicMetaObject subclass
        // (QQmlDMObjectDataMetaObject), which we cannot represent in a QQmlPropertyCache.
        // By not having a shared property cache, revisioned properties in QQmlDelegateModelItem
        // will always be available to the delegate, regardless of the import version.
    }

    void cleanup(QQmlAdaptorModel &) const override
    {
        release();
    }

    bool notify(const QQmlAdaptorModel &model, const QList<QQmlDelegateModelItem *> &items, int index, int count, const QVector<int> &) const override
    {
        for (auto modelItem : items) {
            const int modelItemIndex = modelItem->modelIndex();
            if (modelItemIndex < index || modelItemIndex >= index + count)
                continue;

            auto objectModelItem = static_cast<QQmlDMObjectData *>(modelItem);
            QObject *updatedModelData = qvariant_cast<QObject *>(
                    model.list.at(objectModelItem->modelIndex()));
            objectModelItem->setModelData(updatedModelData);
        }
        return true;
    }
};

class QQmlDMObjectDataMetaObject : public QAbstractDynamicMetaObject
{
public:
    QQmlDMObjectDataMetaObject(QQmlDMObjectData *data, VDMObjectDelegateDataType *type)
        : m_data(data)
        , m_type(type)
    {
        QObjectPrivate *op = QObjectPrivate::get(m_data);
        *static_cast<QMetaObject *>(this) = *type->metaObject;
        op->metaObject = this;
        m_type->addref();
    }

    ~QQmlDMObjectDataMetaObject()
    {
        m_type->release();
    }

    int metaCall(QObject *o, QMetaObject::Call call, int id, void **arguments) override
    {
        Q_ASSERT(o == m_data);
        Q_UNUSED(o);

        static const int objectPropertyOffset = QObject::staticMetaObject.propertyCount();
        if (id >= m_type->propertyOffset
                && (call == QMetaObject::ReadProperty
                || call == QMetaObject::WriteProperty
                || call == QMetaObject::ResetProperty)) {
            if (m_data->object)
                QMetaObject::metacall(m_data->object, call, id - m_type->propertyOffset + objectPropertyOffset, arguments);
            return -1;
        } else if (id >= m_type->signalOffset && call == QMetaObject::InvokeMetaMethod) {
            QMetaObject::activate(m_data, this, id - m_type->signalOffset, nullptr);
            return -1;
        } else {
            return m_data->qt_metacall(call, id, arguments);
        }
    }

    int createProperty(const char *name, const char *) override
    {
        if (!m_data->object)
            return -1;
        const QMetaObject *metaObject = m_data->object->metaObject();
        static const int objectPropertyOffset = QObject::staticMetaObject.propertyCount();

        const int previousPropertyCount = propertyCount() - propertyOffset();
        int propertyIndex = metaObject->indexOfProperty(name);
        if (propertyIndex == -1)
            return -1;
        if (previousPropertyCount + objectPropertyOffset == metaObject->propertyCount())
            return propertyIndex + m_type->propertyOffset - objectPropertyOffset;

        if (m_type->shared) {
            VDMObjectDelegateDataType *type = m_type;
            m_type = new VDMObjectDelegateDataType(*m_type);
            type->release();
        }

        const int previousMethodCount = methodCount();
        int notifierId = previousMethodCount - methodOffset();
        for (int propertyId = previousPropertyCount; propertyId < metaObject->propertyCount() - objectPropertyOffset; ++propertyId) {
            QMetaProperty property = metaObject->property(propertyId + objectPropertyOffset);
            QMetaPropertyBuilder propertyBuilder;
            if (property.hasNotifySignal()) {
                m_type->builder.addSignal("__" + QByteArray::number(propertyId) + "()");
                propertyBuilder = m_type->builder.addProperty(property.name(), property.typeName(), notifierId);
                ++notifierId;
            } else {
                propertyBuilder = m_type->builder.addProperty(property.name(), property.typeName());
            }
            const bool modelWritable = m_type->model->delegateModelAccess != QQmlDelegateModel::ReadOnly;
            propertyBuilder.setWritable(modelWritable && property.isWritable());
            propertyBuilder.setResettable(modelWritable && property.isResettable());
            propertyBuilder.setConstant(property.isConstant());
        }

        m_type->metaObject.reset(m_type->builder.toMetaObject());
        *static_cast<QMetaObject *>(this) = *m_type->metaObject;

        notifierId = previousMethodCount;
        for (int i = previousPropertyCount; i < metaObject->propertyCount() - objectPropertyOffset; ++i) {
            QMetaProperty property = metaObject->property(i + objectPropertyOffset);
            if (property.hasNotifySignal()) {
                QQmlPropertyPrivate::connect(
                        m_data->object, property.notifySignalIndex(), m_data, notifierId);
                ++notifierId;
            }
        }
        return propertyIndex + m_type->propertyOffset - objectPropertyOffset;
    }

    QQmlDMObjectData *m_data;
    VDMObjectDelegateDataType *m_type;
};

QT_END_NAMESPACE

#endif // QQMLDMOBJECTDATA_P_H
