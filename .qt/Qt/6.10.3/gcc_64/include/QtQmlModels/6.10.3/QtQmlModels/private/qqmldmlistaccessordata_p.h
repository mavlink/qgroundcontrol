// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLDMLISTACCESSORDATA_P_H
#define QQMLDMLISTACCESSORDATA_P_H

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

class VDMListDelegateDataType;

class QQmlDMListAccessorData : public QQmlDelegateModelItem
{
    Q_OBJECT
    Q_PROPERTY(QVariant modelData READ modelData WRITE setModelData NOTIFY modelDataChanged)
    QT_ANONYMOUS_PROPERTY(QVariant READ modelData WRITE setModelData NOTIFY modelDataChanged FINAL)
public:
    QQmlDMListAccessorData(
            const QQmlRefPointer<QQmlDelegateModelItemMetaType> &metaType,
            VDMListDelegateDataType *dataType, int index, int row, int column,
            const QVariant &value);
    ~QQmlDMListAccessorData();

    QVariant modelData() const
    {
        return cachedData;
    }

    void setModelData(const QVariant &data);

    static QV4::ReturnedValue get_modelData(const QV4::FunctionObject *b, const QV4::Value *thisObject, const QV4::Value *, int)
    {
        QV4::ExecutionEngine *v4 = b->engine();
        const QQmlDelegateModelItemObject *o = thisObject->as<QQmlDelegateModelItemObject>();
        if (!o)
            return v4->throwTypeError(QStringLiteral("Not a valid DelegateModel object"));

        return v4->fromVariant(static_cast<QQmlDMListAccessorData *>(o->d()->item)->cachedData);
    }

    static QV4::ReturnedValue set_modelData(const QV4::FunctionObject *b, const QV4::Value *thisObject, const QV4::Value *argv, int argc)
    {
        QV4::ExecutionEngine *v4 = b->engine();
        const QQmlDelegateModelItemObject *o = thisObject->as<QQmlDelegateModelItemObject>();
        if (!o)
            return v4->throwTypeError(QStringLiteral("Not a valid DelegateModel object"));
        if (!argc)
            return v4->throwTypeError();

        static_cast<QQmlDMListAccessorData *>(o->d()->item)->setModelData(
                    QV4::ExecutionEngine::toVariant(argv[0], QMetaType {}));
        return QV4::Encode::undefined();
    }

    QV4::ReturnedValue get() override
    {
        QV4::Scope scope(metaType->v4Engine);
        QQmlAdaptorModelEngineData *data = QQmlAdaptorModelEngineData::get(scope.engine);
        QV4::ScopedObject o(
                scope, scope.engine->memoryManager->allocate<QQmlDelegateModelItemObject>(this));
        QV4::ScopedObject p(scope, data->listItemProto.value());
        o->setPrototypeOf(p);
        ++scriptRef;
        return o.asReturnedValue();
    }

    void setValue(const QString &role, const QVariant &value) override;
    bool resolveIndex(const QQmlAdaptorModel &model, int idx) override;

Q_SIGNALS:
    void modelDataChanged();

private:
    friend class VDMListDelegateDataType;
    QVariant cachedData;

    // Gets cleaned when the metaobject has processed it.
    bool cachedDataClean = false;
};


class VDMListDelegateDataType final
    : public QQmlRefCounted<VDMListDelegateDataType>
    , public QQmlAdaptorModel::Accessors
    , public QAbstractDynamicMetaObject
{
public:
    VDMListDelegateDataType(QQmlAdaptorModel *model)
        : model(model)
    {
        QQmlAdaptorModelEngineData::setModelDataType<QQmlDMListAccessorData>(&builder, this);
        metaObject.reset(builder.toMetaObject());
        *static_cast<QMetaObject *>(this) = *metaObject.data();
    }

    void cleanup(QQmlAdaptorModel &) const override
    {
        release();
    }

    int rowCount(const QQmlAdaptorModel &model) const override
    {
        return model.list.count();
    }

    int columnCount(const QQmlAdaptorModel &model) const override
    {
        switch (model.list.type()) {
        case QQmlListAccessor::Invalid:
            return 0;
        case QQmlListAccessor::StringList:
        case QQmlListAccessor::UrlList:
        case QQmlListAccessor::Integer:
            return 1;
        default:
            break;
        }

        // If there are no properties, we can get modelData itself.
        return std::max(1, propertyCount() - propertyOffset);
    }

    static const QMetaObject *metaObjectFromType(QMetaType type)
    {
        if (const QMetaObject *metaObject = type.metaObject())
            return metaObject;

        // NB: This acquires the lock on QQmlMetaTypeData. If we had a QQmlEngine here,
        //     we could use QQmlGadgetPtrWrapper::instance() to avoid this.
        if (const QQmlValueType *valueType = QQmlMetaType::valueType(type))
            return valueType->staticMetaObject();

        return nullptr;
    }

    template<typename String>
    static QString toQString(const String &string)
    {
        if constexpr (std::is_same_v<String, QString>)
            return string;
        else if constexpr (std::is_same_v<String, QByteArray>)
            return QString::fromUtf8(string);
        else if constexpr (std::is_same_v<String, const char *>)
            return QString::fromUtf8(string);
        Q_UNREACHABLE_RETURN(QString());
    }

    template<typename String>
    static QByteArray toUtf8(const String &string)
    {
        if constexpr (std::is_same_v<String, QString>)
            return string.toUtf8();
        else if constexpr (std::is_same_v<String, QByteArray>)
            return string;
        else if constexpr (std::is_same_v<String, const char *>)
            return QByteArray::fromRawData(string, qstrlen(string));
        Q_UNREACHABLE_RETURN(QByteArray());
    }

    template<typename String>
    static QVariant value(const QVariant *row, const String &role)
    {
        const QMetaType type = row->metaType();
        if (type == QMetaType::fromType<QVariantMap>())
            return row->toMap().value(toQString(role));

        if (type == QMetaType::fromType<QVariantHash>())
            return row->toHash().value(toQString(role));

        const QMetaType::TypeFlags typeFlags = type.flags();
        if (typeFlags & QMetaType::PointerToQObject)
            return row->value<QObject *>()->property(toUtf8(role));

        if (const QMetaObject *metaObject = metaObjectFromType(type)) {
            const int propertyIndex = metaObject->indexOfProperty(toUtf8(role));
            if (propertyIndex >= 0)
                return metaObject->property(propertyIndex).readOnGadget(row->constData());
        }

        return QVariant();
    }

    template<typename String>
    void createPropertyIfMissing(const String &string)
    {
        for (int i = 0, end = propertyCount(); i < end; ++i) {
            if (QAnyStringView(property(i).name()) == QAnyStringView(string))
                return;
        }

        createProperty(toUtf8(string), nullptr);
    }

    void createMissingProperties(const QVariant *row)
    {
        const QMetaType type = row->metaType();
        if (type == QMetaType::fromType<QVariantMap>()) {
            const QVariantMap map = row->toMap();
            for (auto it = map.keyBegin(), end = map.keyEnd(); it != end; ++it)
                createPropertyIfMissing(*it);
        } else if (type == QMetaType::fromType<QVariantHash>()) {
            const QVariantHash map = row->toHash();
            for (auto it = map.keyBegin(), end = map.keyEnd(); it != end; ++it)
                createPropertyIfMissing(*it);
        } else if (type.flags() & QMetaType::PointerToQObject) {
            const QMetaObject *metaObject = row->value<QObject *>()->metaObject();
            for (int i = 0, end = metaObject->propertyCount(); i < end; ++i)
                createPropertyIfMissing(metaObject->property(i).name());
        } else if (const QMetaObject *metaObject = metaObjectFromType(type)) {
            for (int i = 0, end = metaObject->propertyCount(); i < end; ++i)
                createPropertyIfMissing(metaObject->property(i).name());
        }
    }

    template<typename String>
    static void setValue(QVariant *row, const String &role, const QVariant &value)
    {
        const QMetaType type = row->metaType();
        if (type == QMetaType::fromType<QVariantMap>()) {
            static_cast<QVariantMap *>(row->data())->insert(toQString(role), value);
        } else if (type == QMetaType::fromType<QVariantHash>()) {
            static_cast<QVariantHash *>(row->data())->insert(toQString(role), value);
        } else if (type.flags() & QMetaType::PointerToQObject) {
            row->value<QObject *>()->setProperty(toUtf8(role), value);
        } else if (const QMetaObject *metaObject = metaObjectFromType(type)) {
            const int propertyIndex = metaObject->indexOfProperty(toUtf8(role));
            if (propertyIndex >= 0)
                metaObject->property(propertyIndex).writeOnGadget(row->data(), value);
        }
    }

    QVariant value(const QQmlAdaptorModel &model, int index, const QString &role) const override
    {
        const QVariant entry = model.list.at(index);
        if (role == QLatin1String("modelData") || role.isEmpty())
            return entry;

        return value(&entry, role);
    }

    QQmlDelegateModelItem *createItem(
            QQmlAdaptorModel &model,
            const QQmlRefPointer<QQmlDelegateModelItemMetaType> &metaType,
            int index, int row, int column) override
    {
        const QVariant value = (index >= 0 && index < model.list.count())
                ? model.list.at(index)
                : QVariant();
        return new QQmlDMListAccessorData(metaType, this, index, row, column, value);
    }

    bool notify(const QQmlAdaptorModel &model, const QList<QQmlDelegateModelItem *> &items, int index, int count, const QVector<int> &) const override
    {
        for (auto modelItem : items) {
            const int modelItemIndex = modelItem->modelIndex();
            if (modelItemIndex < index || modelItemIndex >= index + count)
                continue;

            auto listModelItem = static_cast<QQmlDMListAccessorData *>(modelItem);
            QVariant updatedModelData = model.list.at(listModelItem->modelIndex());
            listModelItem->setModelData(updatedModelData);
        }
        return true;
    }

    void emitAllSignals(QQmlDMListAccessorData *accessor) const;

    int metaCall(QObject *object, QMetaObject::Call call, int id, void **arguments) final;
    int createProperty(const char *name, const char *) final;
#if QT_VERSION >= QT_VERSION_CHECK(7, 0, 0)
    const QMetaObject *toDynamicMetaObject(QObject *accessors) const final;
#else
    QMetaObject *toDynamicMetaObject(QObject *accessors) final;
#endif

    QMetaObjectBuilder builder;
    QQmlAdaptorModel *model = nullptr;
    int propertyOffset = 0;
    int signalOffset = 0;
};

QT_END_NAMESPACE

#endif // QQMLDMLISTACCESSORDATA_P_H
