// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLDMABSTRACTITEMMODELDATA_P_H
#define QQMLDMABSTRACTITEMMODELDATA_P_H

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

QT_BEGIN_NAMESPACE

class VDMAbstractItemModelDataType;
class QQmlDMAbstractItemModelData : public QQmlDelegateModelItem
{
    Q_OBJECT
    Q_PROPERTY(bool hasModelChildren READ hasModelChildren CONSTANT)
    Q_PROPERTY(QVariant modelData READ modelData WRITE setModelData NOTIFY modelDataChanged)
    QT_ANONYMOUS_PROPERTY(QVariant READ modelData NOTIFY modelDataChanged FINAL)

public:
    QQmlDMAbstractItemModelData(
            const QQmlRefPointer<QQmlDelegateModelItemMetaType> &metaType,
            VDMAbstractItemModelDataType *dataType,
            int index, int row, int column);

    int metaCall(QMetaObject::Call call, int id, void **arguments);
    bool hasModelChildren() const;

    QV4::ReturnedValue get() override;
    void setValue(const QString &role, const QVariant &value) override;
    bool resolveIndex(const QQmlAdaptorModel &model, int idx) override;

    static QV4::ReturnedValue get_property(
            const QV4::FunctionObject *b, const QV4::Value *thisObject,
            const QV4::Value *argv, int argc);
    static QV4::ReturnedValue set_property(
            const QV4::FunctionObject *b, const QV4::Value *thisObject,
            const QV4::Value *argv, int argc);

    static QV4::ReturnedValue get_modelData(
            const QV4::FunctionObject *b, const QV4::Value *thisObject,
            const QV4::Value *argv, int argc);
    static QV4::ReturnedValue set_modelData(
            const QV4::FunctionObject *b, const QV4::Value *thisObject,
            const QV4::Value *argv, int argc);

    QVariant modelData() const;
    void setModelData(const QVariant &modelData);

    const VDMAbstractItemModelDataType *type() const { return m_type; }

Q_SIGNALS:
    void modelDataChanged();

private:
    QVariant value(int role) const;
    void setValue(int role, const QVariant &value);

    VDMAbstractItemModelDataType *m_type;
    QVector<QVariant> m_cachedData;
};

class VDMAbstractItemModelDataType final
        : public QQmlRefCounted<VDMAbstractItemModelDataType>
        , public QQmlAdaptorModel::Accessors
        , public QAbstractDynamicMetaObject
{
public:
    VDMAbstractItemModelDataType(QQmlAdaptorModel *model)
        : model(model)
        , propertyOffset(0)
        , signalOffset(0)
    {
    }

    void notifyItem(
            const QQmlGuard<QQmlDMAbstractItemModelData> &item,
            const QVector<int> &indexes,
            QQmlDelegateModel::DelegateModelAccess access) const
    {
        for (const int index : indexes) {
            if (access == QQmlDelegateModel::DelegateModelAccess::ReadWrite) {
                QQmlDelegateModelReadOnlyMetaObject readOnly(item, index + propertyOffset);
                QMetaObject::activate(item, index + signalOffset, nullptr);
            } else {
                QMetaObject::activate(item, index + signalOffset, nullptr);
            }

            if (item.isNull())
                return;
        }
        emit item->modelDataChanged();
    }

    bool notify(
            const QQmlAdaptorModel &model,
            const QList<QQmlDelegateModelItem *> &items,
            int index,
            int count,
            const QVector<int> &roles) const override
    {
        bool changed = roles.isEmpty() && !watchedRoles.isEmpty();
        if (!changed && !watchedRoles.isEmpty() && watchedRoleIds.isEmpty()) {
            QList<int> roleIds;
            for (const QByteArray &r : watchedRoles) {
                QHash<QByteArray, int>::const_iterator it = roleNames.find(r);
                if (it != roleNames.end())
                    roleIds << it.value();
            }
            const_cast<VDMAbstractItemModelDataType *>(this)->watchedRoleIds = roleIds;
        }

        QVector<int> indexes;
        for (int i = 0; i < roles.size(); ++i) {
            const int role = roles.at(i);
            if (!changed && watchedRoleIds.contains(role))
                changed = true;

            int propertyId = propertyRoles.indexOf(role);
            if (propertyId != -1)
                indexes.append(propertyId);
        }
        if (roles.isEmpty()) {
            const int propertyRolesCount = propertyRoles.size();
            indexes.reserve(propertyRolesCount);
            for (int propertyId = 0; propertyId < propertyRolesCount; ++propertyId)
                indexes.append(propertyId);
        }

        QVarLengthArray<QQmlGuard<QQmlDMAbstractItemModelData>> guardedItems;
        for (const auto item : items) {
            Q_ASSERT(qobject_cast<QQmlDMAbstractItemModelData *>(item) == item);
            guardedItems.append(static_cast<QQmlDMAbstractItemModelData *>(item));
        }

        for (const auto &item : std::as_const(guardedItems)) {
            if (item.isNull())
                continue;

            const int idx = item->modelIndex();
            if (idx >= index && idx < index + count)
                notifyItem(item, indexes, model.delegateModelAccess);
        }
        return changed;
    }

    void replaceWatchedRoles(
            QQmlAdaptorModel &,
            const QList<QByteArray> &oldRoles,
            const QList<QByteArray> &newRoles) const override
    {
        VDMAbstractItemModelDataType *dataType = const_cast<VDMAbstractItemModelDataType *>(this);

        dataType->watchedRoleIds.clear();
        for (const QByteArray &oldRole : oldRoles)
            dataType->watchedRoles.removeOne(oldRole);
        dataType->watchedRoles += newRoles;
    }

    static QV4::ReturnedValue get_hasModelChildren(const QV4::FunctionObject *b, const QV4::Value *thisObject, const QV4::Value *, int)
    {
        QV4::Scope scope(b);
        QV4::Scoped<QQmlDelegateModelItemObject> o(scope, thisObject->as<QQmlDelegateModelItemObject>());
        if (!o)
            RETURN_RESULT(scope.engine->throwTypeError(QStringLiteral("Not a valid DelegateModel object")));

        const QQmlAdaptorModel *const model
                = static_cast<QQmlDMAbstractItemModelData *>(o->d()->item)->type()->model;
        if (o->d()->item->modelIndex() >= 0) {
            if (const QAbstractItemModel *const aim = model->aim())
                RETURN_RESULT(QV4::Encode(aim->hasChildren(
                        aim->index(o->d()->item->modelIndex(), 0, model->rootIndex))));
        }
        RETURN_RESULT(QV4::Encode(false));
    }


    void initializeConstructor(QQmlAdaptorModelEngineData *const data)
    {
        QV4::ExecutionEngine *v4 = data->v4;
        QV4::Scope scope(v4);
        QV4::ScopedObject proto(scope, v4->newObject());
        proto->defineAccessorProperty(QStringLiteral("index"), QQmlAdaptorModelEngineData::get_index, nullptr);
        proto->defineAccessorProperty(QStringLiteral("hasModelChildren"), get_hasModelChildren, nullptr);
        proto->defineAccessorProperty(QStringLiteral("modelData"),
                                      QQmlDMAbstractItemModelData::get_modelData,
                                      QQmlDMAbstractItemModelData::set_modelData);
        QV4::ScopedProperty p(scope);

        typedef QHash<QByteArray, int>::const_iterator iterator;
        for (iterator it = roleNames.constBegin(), end = roleNames.constEnd(); it != end; ++it) {
            const qsizetype propertyId = propertyRoles.indexOf(it.value());
            const QByteArray &propertyName = it.key();

            QV4::ScopedString name(scope, v4->newString(QString::fromUtf8(propertyName)));
            QV4::ScopedFunctionObject g(
                    scope,
                    v4->memoryManager->allocate<QV4::IndexedBuiltinFunction>(
                            v4, propertyId, QQmlDMAbstractItemModelData::get_property));
            QV4::ScopedFunctionObject s(
                    scope,
                    v4->memoryManager->allocate<QV4::IndexedBuiltinFunction>(
                            v4, propertyId, QQmlDMAbstractItemModelData::set_property));
            p->setGetter(g);
            p->setSetter(s);
            proto->insertMember(name, p, QV4::Attr_Accessor|QV4::Attr_NotEnumerable|QV4::Attr_NotConfigurable);
        }
        prototype.set(v4, proto);
    }

    // QAbstractDynamicMetaObject

    void objectDestroyed(QObject *) override
    {
        release();
    }

    int metaCall(QObject *object, QMetaObject::Call call, int id, void **arguments) override
    {
        return static_cast<QQmlDMAbstractItemModelData *>(object)->metaCall(call, id, arguments);
    }

    int rowCount(const QQmlAdaptorModel &model) const override
    {
        if (const QAbstractItemModel *aim = model.aim())
            return aim->rowCount(model.rootIndex);
        return 0;
    }

    int columnCount(const QQmlAdaptorModel &model) const override
    {
        if (const QAbstractItemModel *aim = model.aim())
            return aim->columnCount(model.rootIndex);
        return 0;
    }

    void cleanup(QQmlAdaptorModel &) const override
    {
        release();
    }

    QVariant value(const QQmlAdaptorModel &model, int index, const QString &role) const override
    {
        if (!metaObject) {
            VDMAbstractItemModelDataType *dataType = const_cast<VDMAbstractItemModelDataType *>(this);
            dataType->initializeMetaObject(model);
        }

        if (const QAbstractItemModel *aim = model.aim()) {
            const QModelIndex modelIndex
                    = aim->index(model.rowAt(index), model.columnAt(index), model.rootIndex);

            const auto it = roleNames.find(role.toUtf8()), end = roleNames.end();
            if (it != roleNames.end())
                return modelIndex.data(*it);

            if (role.isEmpty() || role == QLatin1String("modelData")) {
                if (roleNames.size() == 1)
                    return modelIndex.data(roleNames.begin().value());

                QVariantMap modelData;
                for (auto jt = roleNames.begin(); jt != end; ++jt)
                    modelData.insert(QString::fromUtf8(jt.key()), modelIndex.data(jt.value()));
                return modelData;
            }

            if (role == QLatin1String("hasModelChildren"))
                return QVariant(aim->hasChildren(modelIndex));
        }
        return QVariant();
    }

    QVariant parentModelIndex(const QQmlAdaptorModel &model) const override
    {
        if (const QAbstractItemModel *aim = model.aim())
            return QVariant::fromValue(aim->parent(model.rootIndex));
        return QVariant();
    }

    QVariant modelIndex(const QQmlAdaptorModel &model, int index) const override
    {
        if (const QAbstractItemModel *aim = model.aim())
            return QVariant::fromValue(aim->index(model.rowAt(index), model.columnAt(index),
                                                  model.rootIndex));
        return QVariant();
    }

    bool canFetchMore(const QQmlAdaptorModel &model) const override
    {
        if (const QAbstractItemModel *aim = model.aim())
            return aim->canFetchMore(model.rootIndex);
        return false;
    }

    void fetchMore(QQmlAdaptorModel &model) const override
    {
        if (QAbstractItemModel *aim = model.aim())
            aim->fetchMore(model.rootIndex);
    }

    QQmlDelegateModelItem *createItem(
            QQmlAdaptorModel &model,
            const QQmlRefPointer<QQmlDelegateModelItemMetaType> &metaType,
            int index, int row, int column) override
    {
        if (!metaObject)
            initializeMetaObject(model);
        return new QQmlDMAbstractItemModelData(metaType, this, index, row, column);
    }

    void initializeMetaObject(const QQmlAdaptorModel &model)
    {
        QMetaObjectBuilder builder;
        QQmlAdaptorModelEngineData::setModelDataType<QQmlDMAbstractItemModelData>(&builder, this);

        const QByteArray propertyType = QByteArrayLiteral("QVariant");
        const QAbstractItemModel *aim = model.aim();
        const QHash<int, QByteArray> names = aim ? aim->roleNames() : QHash<int, QByteArray>();
        for (QHash<int, QByteArray>::const_iterator it = names.begin(), cend = names.end(); it != cend; ++it) {
            const int propertyId = propertyRoles.size();
            propertyRoles.append(it.key());
            roleNames.insert(it.value(), it.key());
            QQmlAdaptorModelEngineData::addProperty(
                    &builder, propertyId, it.value(), propertyType,
                    model.delegateModelAccess != QQmlDelegateModel::ReadOnly);
        }

        metaObject.reset(builder.toMetaObject());
        *static_cast<QMetaObject *>(this) = *metaObject;
        propertyCache = QQmlPropertyCache::createStandalone(
                    metaObject.data(), model.modelItemRevision);
    }

    QV4::PersistentValue prototype;
    QList<int> propertyRoles;
    QList<int> watchedRoleIds;
    QList<QByteArray> watchedRoles;
    QHash<QByteArray, int> roleNames;
    QQmlAdaptorModel *model;
    int propertyOffset;
    int signalOffset;
};

QT_END_NAMESPACE

#endif // QQMLDMABSTRACTITEMMODELDATA_P_H
