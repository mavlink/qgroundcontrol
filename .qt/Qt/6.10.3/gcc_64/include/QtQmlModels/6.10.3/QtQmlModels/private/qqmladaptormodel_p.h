// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLADAPTORMODEL_P_H
#define QQMLADAPTORMODEL_P_H

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

#include <QtCore/qabstractitemmodel.h>

#include <private/qqmldelegatemodel_p.h>
#include <private/qqmlguard_p.h>
#include <private/qqmllistaccessor_p.h>
#include <private/qqmlnullablevalue_p.h>
#include <private/qqmlpropertycache_p.h>
#include <private/qtqmlglobal_p.h>
#include <private/qtqmlmodelsglobal_p.h>

QT_REQUIRE_CONFIG(qml_delegate_model);

QT_BEGIN_NAMESPACE

class QQmlEngine;

class QQmlDelegateModelItem;
class QQmlDelegateModelItemMetaType;

class Q_QMLMODELS_EXPORT QQmlAdaptorModel : public QQmlGuard<QObject>
{
public:
    class Accessors
    {
    public:
        inline Accessors() {}
        virtual ~Accessors();
        virtual int rowCount(const QQmlAdaptorModel &) const { return 0; }
        virtual int columnCount(const QQmlAdaptorModel &) const { return 0; }
        virtual void cleanup(QQmlAdaptorModel &) const {}

        virtual QVariant value(const QQmlAdaptorModel &, int, const QString &) const {
            return QVariant(); }

        virtual QQmlDelegateModelItem *createItem(
                QQmlAdaptorModel &,
                const QQmlRefPointer<QQmlDelegateModelItemMetaType> &,
                int, int, int) { return nullptr; }

        virtual bool notify(
                const QQmlAdaptorModel &,
                const QList<QQmlDelegateModelItem *> &,
                int,
                int,
                const QVector<int> &) const { return false; }
        virtual void replaceWatchedRoles(
                QQmlAdaptorModel &,
                const QList<QByteArray> &,
                const QList<QByteArray> &) const {}
        virtual QVariant parentModelIndex(const QQmlAdaptorModel &) const {
            return QVariant(); }
        virtual QVariant modelIndex(const QQmlAdaptorModel &, int) const {
            return QVariant(); }
        virtual bool canFetchMore(const QQmlAdaptorModel &) const { return false; }
        virtual void fetchMore(QQmlAdaptorModel &) const {}

        QScopedPointer<QMetaObject, QScopedPointerPodDeleter> metaObject;
        QQmlPropertyCache::ConstPtr propertyCache;
    };

    Accessors *accessors;
    QPersistentModelIndex rootIndex;
    QQmlListAccessor list;
    // we need to ensure that a JS created model does not get gced, but cannot
    // arbitrarily set the parent  (using QQmlStrongJSQObjectReference)  of QObject based models,
    // as that causes issues with singletons
    QV4::PersistentValue modelStrongReference;

    QTypeRevision modelItemRevision = QTypeRevision::zero();
    QQmlDelegateModel::DelegateModelAccess delegateModelAccess = QQmlDelegateModel::Qt5ReadWrite;

    QQmlAdaptorModel();
    ~QQmlAdaptorModel();

    inline QVariant model() const { return list.list(); }
    void setModel(const QVariant &variant);
    void invalidateModel();

    bool isValid() const;
    int count() const;
    int rowCount() const;
    int columnCount() const;
    int rowAt(int index) const;
    int columnAt(int index) const;
    int indexAt(int row, int column) const;

    void useImportVersion(QTypeRevision revision);

    inline bool adaptsAim() const { return qobject_cast<QAbstractItemModel *>(object()); }
    inline QAbstractItemModel *aim() { return static_cast<QAbstractItemModel *>(object()); }
    inline const QAbstractItemModel *aim() const { return static_cast<const QAbstractItemModel *>(object()); }

    inline QVariant value(int index, const QString &role) const {
        return accessors->value(*this, index, role); }
    inline QQmlDelegateModelItem *createItem(
            const QQmlRefPointer<QQmlDelegateModelItemMetaType> &metaType, int index)
    {
        return accessors->createItem(*this, metaType, index, rowAt(index), columnAt(index));
    }
    inline bool hasProxyObject() const {
        switch (list.type()) {
        case QQmlListAccessor::Instance:
        case QQmlListAccessor::ListProperty:
        case QQmlListAccessor::ObjectList:
        case QQmlListAccessor::ObjectSequence:
            return true;
        default:
            break;
        }
        return false;
    }

    inline bool notify(
            const QList<QQmlDelegateModelItem *> &items,
            int index,
            int count,
            const QVector<int> &roles) const {
        return accessors->notify(*this, items, index, count, roles); }
    inline void replaceWatchedRoles(
            const QList<QByteArray> &oldRoles, const QList<QByteArray> &newRoles) {
        accessors->replaceWatchedRoles(*this, oldRoles, newRoles); }

    inline QVariant modelIndex(int index) const { return accessors->modelIndex(*this, index); }
    inline QVariant parentModelIndex() const { return accessors->parentModelIndex(*this); }
    inline bool canFetchMore() const { return accessors->canFetchMore(*this); }
    inline void fetchMore() { return accessors->fetchMore(*this); }

private:
    static void objectDestroyedImpl(QQmlGuardImpl *);

    Accessors m_nullAccessors;
};

QT_END_NAMESPACE

#endif
