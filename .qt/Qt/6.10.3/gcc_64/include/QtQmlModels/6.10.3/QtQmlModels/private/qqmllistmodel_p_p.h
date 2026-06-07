// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLLISTMODEL_P_P_H
#define QQMLLISTMODEL_P_P_H

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

#include "qqmllistmodel_p.h"
#include <private/qtqmlmodelsglobal_p.h>
#include <private/qqmlengine_p.h>
#include <private/qqmlopenmetaobject_p.h>
#include <private/qv4qobjectwrapper_p.h>
#include <qqml.h>

QT_REQUIRE_CONFIG(qml_list_model);

QT_BEGIN_NAMESPACE


class DynamicRoleModelNode;

class DynamicRoleModelNodeMetaObject : public QQmlOpenMetaObject
{
public:
    DynamicRoleModelNodeMetaObject(DynamicRoleModelNode *object);
    ~DynamicRoleModelNodeMetaObject();

    bool m_enabled;

protected:
    void propertyWrite(int index) override;
    void propertyWritten(int index) override;

private:
    DynamicRoleModelNode *m_owner;
};

class DynamicRoleModelNode : public QObject
{
    Q_OBJECT
public:
    DynamicRoleModelNode(QQmlListModel *owner, int uid);

    static DynamicRoleModelNode *create(const QVariantMap &obj, QQmlListModel *owner);

    void updateValues(const QVariantMap &object, QVector<int> &roles);

    QVariant getValue(const QString &name) const
    {
        return m_meta->value(name.toUtf8());
    }

    bool setValue(const QByteArray &name, const QVariant &val)
    {
        return m_meta->setValue(name, val);
    }

    void setNodeUpdatesEnabled(bool enable)
    {
        m_meta->m_enabled = enable;
    }

    int getUid() const
    {
        return m_uid;
    }

    static QVector<int> sync(DynamicRoleModelNode *src, DynamicRoleModelNode *target);

private:
    QQmlListModel *m_owner;
    int m_uid;
    DynamicRoleModelNodeMetaObject *m_meta;

    friend class DynamicRoleModelNodeMetaObject;
};

class ModelNodeMetaObject : public QQmlOpenMetaObject
{
public:
    ModelNodeMetaObject(QObject *object, QQmlListModel *model, int elementIndex);
    ~ModelNodeMetaObject();

#if QT_VERSION >= QT_VERSION_CHECK(7, 0, 0)
    const QMetaObject *toDynamicMetaObject(QObject *object) const override;
#else
    QMetaObject *toDynamicMetaObject(QObject *object) override;
#endif

    static ModelNodeMetaObject *get(QObject *obj);

    bool m_enabled;
    QQmlListModel *m_model;
    int m_elementIndex;

    void updateValues();
    void updateValues(const QVector<int> &roles);

    bool initialized() const { return m_initialized; }

protected:
    void propertyWritten(int index) override;

private:
    using QQmlOpenMetaObject::setValue;

    void emitDirectNotifies(const int *changedRoles, int roleCount);

    void initialize();
    mutable bool m_initialized;
};

namespace QV4 {

namespace Heap {

struct ModelObject : public QObjectWrapper {
    void init(QObject *object, QQmlListModel *model)
    {
        QObjectWrapper::init(object);
        m_model = model;
    }

    void destroy()
    {
        m_model.destroy();
        QObjectWrapper::destroy();
    }

    int elementIndex() const {
        if (const QObject *o = object()) {
            const QObjectPrivate *op = QObjectPrivate::get(o);
            return static_cast<ModelNodeMetaObject *>(op->metaObject)->m_elementIndex;
        }
        return -1;
    }

    QV4QPointer<QQmlListModel> m_model;
};

}

struct ModelObject : public QObjectWrapper
{
    V4_OBJECT2(ModelObject, QObjectWrapper)
    V4_NEEDS_DESTROY

protected:
    static bool virtualPut(Managed *m, PropertyKey id, const Value& value, Value *receiver);
    static ReturnedValue virtualGet(const Managed *m, PropertyKey id, const Value *receiver, bool *hasProperty);
    static ReturnedValue virtualResolveLookupGetter(const Object *object, ExecutionEngine *engine, Lookup *lookup);
    static ReturnedValue lookupGetter(Lookup *l, ExecutionEngine *engine, const Value &object);
    static OwnPropertyKeyIterator *virtualOwnPropertyKeys(const Object *m, Value *target);
};

} // namespace QV4

class ListLayout
{
public:
    ListLayout() : currentBlock(0), currentBlockOffset(0) {}
    ListLayout(const ListLayout *other);
    ~ListLayout();

    class Role
    {
    public:

        Role() : type(Invalid), blockIndex(-1), blockOffset(-1), index(-1), subLayout(0) {}
        explicit Role(const Role *other);
        ~Role();

        // This enum must be kept in sync with the roleTypeNames variable in qqmllistmodel.cpp
        enum DataType
        {
            Invalid = -1,

            String,
            Number,
            Bool,
            List,
            QObject,
            VariantMap,
            DateTime,
            Url,
            Function,

            MaxDataType
        };

        QString name;
        DataType type;
        int blockIndex;
        int blockOffset;
        int index;
        ListLayout *subLayout;
    };

    const Role *getRoleOrCreate(const QString &key, const QVariant &data);
    const Role &getRoleOrCreate(QV4::String *key, Role::DataType type);
    const Role &getRoleOrCreate(const QString &key, Role::DataType type);

    const Role &getExistingRole(int index) const { return *roles.at(index); }
    const Role *getExistingRole(const QString &key) const;
    const Role *getExistingRole(QV4::String *key) const;

    int roleCount() const { return roles.size(); }

    static void sync(ListLayout *src, ListLayout *target);

private:
    const Role &createRole(const QString &key, Role::DataType type);

    int currentBlock;
    int currentBlockOffset;
    QVector<Role *> roles;
    QStringHash<Role *> roleHash;
};

struct StringOrTranslation
{
    ~StringOrTranslation();
    bool isSet() const { return binding || arrayData; }
    bool isTranslation() const { return binding && !arrayData; }
    void setString(const QString &s);
    void setTranslation(const QV4::CompiledData::Binding *binding);
    QString toString(const QQmlListModel *owner) const;
    QString asString() const;
private:
    void clear();

    union {
        char16_t *stringData = nullptr;
        const QV4::CompiledData::Binding *binding;
    };

    QTypedArrayData<char16_t> *arrayData = nullptr;
    uint stringSize = 0;
};

/*!
\internal
*/
class ListElement
{
public:
    enum ObjectIndestructible { Indestructible = 1, ExplicitlySet = 2 };
    enum { BLOCK_SIZE = 64 - sizeof(int) - sizeof(ListElement *) - sizeof(ModelNodeMetaObject *) };

    ListElement();
    ListElement(int existingUid);
    ~ListElement();

    static QVector<int> sync(ListElement *src, ListLayout *srcLayout, ListElement *target, ListLayout *targetLayout);

private:

    void destroy(ListLayout *layout);

    int setVariantProperty(const ListLayout::Role &role, const QVariant &d);

    int setJsProperty(const ListLayout::Role &role, const QV4::Value &d, QV4::ExecutionEngine *eng);

    int setStringProperty(const ListLayout::Role &role, const QString &s);
    int setDoubleProperty(const ListLayout::Role &role, double n);
    int setBoolProperty(const ListLayout::Role &role, bool b);
    int setListProperty(const ListLayout::Role &role, ListModel *m);
    int setQObjectProperty(const ListLayout::Role &role, QV4::QObjectWrapper *o);
    int setVariantMapProperty(const ListLayout::Role &role, QV4::Object *o);
    int setVariantMapProperty(const ListLayout::Role &role, QVariantMap *m);
    int setDateTimeProperty(const ListLayout::Role &role, const QDateTime &dt);
    int setUrlProperty(const ListLayout::Role &role, const QUrl &url);
    int setFunctionProperty(const ListLayout::Role &role, const QJSValue &f);
    int setTranslationProperty(const ListLayout::Role &role, const QV4::CompiledData::Binding *b);

    void setStringPropertyFast(const ListLayout::Role &role, const QString &s);
    void setDoublePropertyFast(const ListLayout::Role &role, double n);
    void setBoolPropertyFast(const ListLayout::Role &role, bool b);
    void setQObjectPropertyFast(const ListLayout::Role &role, QV4::QObjectWrapper *o);
    void setListPropertyFast(const ListLayout::Role &role, ListModel *m);
    void setVariantMapFast(const ListLayout::Role &role, QV4::Object *o);
    void setDateTimePropertyFast(const ListLayout::Role &role, const QDateTime &dt);
    void setUrlPropertyFast(const ListLayout::Role &role, const QUrl &url);
    void setFunctionPropertyFast(const ListLayout::Role &role, const QJSValue &f);

    void clearProperty(const ListLayout::Role &role);

    QVariant getProperty(const ListLayout::Role &role, const QQmlListModel *owner, QV4::ExecutionEngine *eng);
    ListModel *getListProperty(const ListLayout::Role &role);
    StringOrTranslation *getStringProperty(const ListLayout::Role &role);
    QV4::QObjectWrapper *getQObjectProperty(const ListLayout::Role &role);
    QV4::PersistentValue *getGuardProperty(const ListLayout::Role &role);
    QVariantMap *getVariantMapProperty(const ListLayout::Role &role);
    QDateTime *getDateTimeProperty(const ListLayout::Role &role);
    QUrl *getUrlProperty(const ListLayout::Role &role);
    QJSValue *getFunctionProperty(const ListLayout::Role &role);

    inline char *getPropertyMemory(const ListLayout::Role &role);

    int getUid() const { return uid; }

    ModelNodeMetaObject *objectCache();

    char data[BLOCK_SIZE];
    ListElement *next;

    int uid;
    QObject *m_objectCache;

    friend class ListModel;
};

/*!
\internal
*/
class ListModel
{
public:

    ListModel(ListLayout *layout, QQmlListModel *modelCache);

    void destroy();

    int setOrCreateProperty(int elementIndex, const QString &key, const QVariant &data);
    int setExistingProperty(int uid, const QString &key, const QV4::Value &data, QV4::ExecutionEngine *eng);

    QVariant getProperty(int elementIndex, int roleIndex, const QQmlListModel *owner, QV4::ExecutionEngine *eng);
    ListModel *getListProperty(int elementIndex, const ListLayout::Role &role);

    void updateTranslations();

    int roleCount() const
    {
        return m_layout->roleCount();
    }

    const ListLayout::Role &getExistingRole(int index) const
    {
        return m_layout->getExistingRole(index);
    }

    const ListLayout::Role *getExistingRole(QV4::String *key) const
    {
        return m_layout->getExistingRole(key);
    }

    const ListLayout::Role &getOrCreateListRole(const QString &name)
    {
        return m_layout->getRoleOrCreate(name, ListLayout::Role::List);
    }

    int elementCount() const
    {
        return elements.count();
    }

    enum class SetElement {WasJustInserted, IsCurrentlyUpdated};

    void set(int elementIndex, QV4::Object *object, QVector<int> *roles);
    void set(int elementIndex, QV4::Object *object, SetElement reason = SetElement::IsCurrentlyUpdated);

    int append(QV4::Object *object);
    void insert(int elementIndex, QV4::Object *object);

    Q_REQUIRED_RESULT QVector<std::function<void()>> remove(int index, int count);

    int appendElement();
    void insertElement(int index);

    void move(int from, int to, int n);

    static bool sync(ListModel *src, ListModel *target);

    QObject *getOrCreateModelObject(QQmlListModel *model, int elementIndex);

private:
    QPODVector<ListElement *, 4> elements;
    ListLayout *m_layout;

    QQmlListModel *m_modelCache;

    struct ElementSync
    {
        ListElement *src = nullptr;
        ListElement *target = nullptr;
        int srcIndex = -1;
        int targetIndex = -1;
        QVector<int> changedRoles;
    };

    void newElement(int index);

    void updateCacheIndices(int start = 0, int end = -1);

    template<typename ArrayLike>
    ListModel *resolveSubModel(QV4::ScopedObject *o, const ListLayout::Role &r, ArrayLike *a)
    {
        ListModel *subModel = new ListModel(r.subLayout, nullptr);

        for (qint64 j = 0, arrayLength = a->getLength(); j < arrayLength; ++j) {
            *o = a->get(j);
            subModel->append(*o);
        }

        return subModel;
    }

    template<typename ArrayLike>
    void setArrayLikeFast(
            QV4::ScopedObject *o, QV4::String *propertyName, ListElement *e, ArrayLike *a)
    {
        const ListLayout::Role &r = m_layout->getRoleOrCreate(propertyName, ListLayout::Role::List);
        if (r.type == ListLayout::Role::List)
            e->setListPropertyFast(r, resolveSubModel(o, r, a));
    }

    template<typename ArrayLike>
    int setArrayLike(
            QV4::ScopedObject *o, QV4::String *propertyName, ListElement *e, ArrayLike *a)
    {
        const ListLayout::Role &r = m_layout->getRoleOrCreate(propertyName, ListLayout::Role::List);
        return e->setListProperty(r, resolveSubModel(o, r, a));
    }

    friend class ListElement;
    friend class QQmlListModelWorkerAgent;
    friend class QQmlListModelParser;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(ListModel *);

#endif // QQUICKLISTMODEL_P_P_H
