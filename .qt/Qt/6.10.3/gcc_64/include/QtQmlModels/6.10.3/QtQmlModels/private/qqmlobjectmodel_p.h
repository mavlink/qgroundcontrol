// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLINSTANCEMODEL_P_H
#define QQMLINSTANCEMODEL_P_H

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

#include <private/qtqmlmodelsglobal_p.h>
#include <private/qqmlincubator_p.h>
#include <QtQml/qqml.h>
#include <QtCore/qobject.h>

QT_REQUIRE_CONFIG(qml_object_model);

QT_BEGIN_NAMESPACE

class QObject;
class QQmlChangeSet;
class QAbstractItemModel;

class Q_QMLMODELS_EXPORT QQmlInstanceModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int count READ count NOTIFY countChanged)
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(2, 0)

public:
    enum ReusableFlag {
        NotReusable,
        Reusable
    };

    enum ReleaseFlag { Referenced = 0x01, Destroyed = 0x02, Pooled = 0x04 };
    Q_DECLARE_FLAGS(ReleaseFlags, ReleaseFlag)

    virtual int count() const = 0;
    virtual bool isValid() const = 0;
    virtual QObject *object(int index, QQmlIncubator::IncubationMode incubationMode = QQmlIncubator::AsynchronousIfNested) = 0;
    virtual ReleaseFlags release(QObject *object, ReusableFlag reusableFlag = NotReusable) = 0;
    virtual void cancel(int) {}
    QString stringValue(int index, const QString &role) { return variantValue(index, role).toString(); }
    virtual QVariant variantValue(int, const QString &) = 0;
    virtual void setWatchedRoles(const QList<QByteArray> &roles) = 0;
    virtual QQmlIncubator::Status incubationStatus(int index) = 0;

    virtual void drainReusableItemsPool(int maxPoolTime) { Q_UNUSED(maxPoolTime); }
    virtual int poolSize() { return 0; }

    virtual int indexOf(QObject *object, QObject *objectContext) const = 0;
    virtual const QAbstractItemModel *abstractItemModel() const { return nullptr; }

    virtual bool setRequiredProperty(int index, const QString &name, const QVariant &value);

Q_SIGNALS:
    void countChanged();
    void modelUpdated(const QQmlChangeSet &changeSet, bool reset);
    void createdItem(int index, QObject *object);
    void initItem(int index, QObject *object);
    void destroyingItem(QObject *object);
    Q_REVISION(2, 15) void itemPooled(int index, QObject *object);
    Q_REVISION(2, 15) void itemReused(int index, QObject *object);

protected:
    QQmlInstanceModel(QObjectPrivate &dd, QObject *parent = nullptr)
        : QObject(dd, parent) {}

private:
    Q_DISABLE_COPY(QQmlInstanceModel)
};

class QQmlObjectModelAttached;
class QQmlObjectModelPrivate;
class Q_QMLMODELS_EXPORT QQmlObjectModel : public QQmlInstanceModel
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQmlObjectModel)

    Q_PROPERTY(QQmlListProperty<QObject> children READ children NOTIFY childrenChanged DESIGNABLE false)
    Q_CLASSINFO("qt_QmlJSWrapperFactoryMethod", "_q_createJSWrapper(QQmlV4ExecutionEnginePtr)")
    Q_CLASSINFO("DefaultProperty", "children")
    QML_NAMED_ELEMENT(ObjectModel)
    QML_ADDED_IN_VERSION(2, 1)
    QML_ATTACHED(QQmlObjectModelAttached)

public:
    QQmlObjectModel(QObject *parent=nullptr);
    ~QQmlObjectModel();

    int count() const override;
    bool isValid() const override;
    QObject *object(int index, QQmlIncubator::IncubationMode incubationMode = QQmlIncubator::AsynchronousIfNested) override;
    ReleaseFlags release(QObject *object, ReusableFlag reusable = NotReusable) override;
    QVariant variantValue(int index, const QString &role) override;
    void setWatchedRoles(const QList<QByteArray> &) override {}
    QQmlIncubator::Status incubationStatus(int index) override;

    int indexOf(QObject *object, QObject *objectContext) const override;

    QQmlListProperty<QObject> children();

    static QQmlObjectModelAttached *qmlAttachedProperties(QObject *obj);

    Q_REVISION(2, 3) Q_INVOKABLE QObject *get(int index) const;
    Q_REVISION(2, 3) Q_INVOKABLE void append(QObject *object);
    Q_REVISION(2, 3) Q_INVOKABLE void insert(int index, QObject *object);
    Q_REVISION(2, 3) Q_INVOKABLE void move(int from, int to, int n = 1);
    Q_REVISION(2, 3) Q_INVOKABLE void remove(int index, int n = 1);

public Q_SLOTS:
    Q_REVISION(2, 3) void clear();

Q_SIGNALS:
    void childrenChanged();

private:
    Q_PRIVATE_SLOT(d_func(), quint64 _q_createJSWrapper(QQmlV4ExecutionEnginePtr))
    Q_DISABLE_COPY(QQmlObjectModel)
};

class QQmlObjectModelAttached : public QObject
{
    Q_OBJECT

public:
    QQmlObjectModelAttached(QObject *parent)
        : QObject(parent), m_index(-1) {}

    Q_PROPERTY(int index READ index NOTIFY indexChanged FINAL)
    int index() const { return m_index; }
    void setIndex(int idx) {
        if (m_index != idx) {
            m_index = idx;
            Q_EMIT indexChanged();
        }
    }

Q_SIGNALS:
    void indexChanged();

public:
    int m_index;
};


QT_END_NAMESPACE

#endif // QQMLINSTANCEMODEL_P_H
