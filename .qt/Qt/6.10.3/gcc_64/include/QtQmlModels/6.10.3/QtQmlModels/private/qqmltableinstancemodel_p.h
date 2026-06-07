// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLTABLEINSTANCEMODEL_P_H
#define QQMLTABLEINSTANCEMODEL_P_H

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

#include <QtQmlModels/private/qqmldelegatemodel_p.h>
#include <QtQmlModels/private/qqmldelegatemodel_p_p.h>

#include <QtCore/qpointer.h>

QT_REQUIRE_CONFIG(qml_table_model);

QT_BEGIN_NAMESPACE

class QQmlTableInstanceModel;
class QQmlAbstractDelegateComponent;

class QQmlTableInstanceModelIncubationTask : public QQDMIncubationTask
{
public:
    QQmlTableInstanceModelIncubationTask(
            QQmlTableInstanceModel *tableInstanceModel
            , QQmlDelegateModelItem* modelItemToIncubate
            , IncubationMode mode)
        : QQDMIncubationTask(nullptr, mode)
        , modelItemToIncubate(modelItemToIncubate)
        , tableInstanceModel(tableInstanceModel) {
        clear();
    }

    void statusChanged(Status status) override;
    void setInitialState(QObject *object) override;

    QQmlDelegateModelItem *modelItemToIncubate = nullptr;
    QQmlTableInstanceModel *tableInstanceModel = nullptr;
};

class Q_QMLMODELS_EXPORT QQmlTableInstanceModel : public QQmlInstanceModel
{
    Q_OBJECT

public:
    QQmlTableInstanceModel(QQmlContext *qmlContext, QObject *parent = nullptr);
    ~QQmlTableInstanceModel() override;

    void useImportVersion(QTypeRevision version);

    int count() const override { return m_adaptorModel.count(); }
    int rows() const { return m_adaptorModel.rowCount(); }
    int columns() const { return m_adaptorModel.columnCount(); }

    bool isValid() const override { return true; }

    bool canFetchMore() const { return m_adaptorModel.canFetchMore(); }
    void fetchMore() { m_adaptorModel.fetchMore(); }

    QVariant model() const;
    void setModel(const QVariant &model);

    QQmlComponent *delegate() const;
    void setDelegate(QQmlComponent *);

    QQmlDelegateModel::DelegateModelAccess delegateModelAccess() const
    {
        return m_adaptorModel.delegateModelAccess;
    }
    void setDelegateModelAccess(QQmlDelegateModel::DelegateModelAccess delegateModelAccess)
    {
        m_adaptorModel.delegateModelAccess = delegateModelAccess;
    }

    const QAbstractItemModel *abstractItemModel() const override;

    QObject *object(int index, QQmlIncubator::IncubationMode incubationMode = QQmlIncubator::AsynchronousIfNested) override;
    ReleaseFlags release(QObject *object, ReusableFlag reusable = NotReusable) override;
    void dispose(QObject *object);
    void cancel(int) override;

    void drainReusableItemsPool(int maxPoolTime) override;
    int poolSize() override { return m_reusableItemsPool.size(); }
    void reuseItem(QQmlDelegateModelItem *item, int newModelIndex);

    QQmlIncubator::Status incubationStatus(int index) override;

    bool setRequiredProperty(int index, const QString &name, const QVariant &value) final;

    QVariant variantValue(int, const QString &) override { Q_UNREACHABLE_RETURN(QVariant()); }
    void setWatchedRoles(const QList<QByteArray> &) override { Q_UNREACHABLE(); }
    int indexOf(QObject *, QObject *) const override { Q_UNREACHABLE_RETURN(0); }

    QQmlDelegateModelItem *getModelItem(int index);

signals:
    void modelChanged();

private:
    enum DestructionMode {
        Deferred,
        Immediate
    };

    QQmlComponent *resolveDelegate(int index);

    QQmlAdaptorModel m_adaptorModel;
    QQmlAbstractDelegateComponent *m_delegateChooser = nullptr;
    QQmlComponent *m_delegate = nullptr;
    QPointer<QQmlContext> m_qmlContext;
    QQmlRefPointer<QQmlDelegateModelItemMetaType> m_metaType;

    QHash<int, QQmlDelegateModelItem *> m_modelItems;
    QQmlReusableDelegateModelItemsPool m_reusableItemsPool;
    QList<QQmlIncubator *> m_finishedIncubationTasks;

    void incubateModelItem(QQmlDelegateModelItem *modelItem, QQmlIncubator::IncubationMode incubationMode);
    void incubatorStatusChanged(QQmlTableInstanceModelIncubationTask *dmIncubationTask, QQmlIncubator::Status status);
    void deleteIncubationTaskLater(QQmlIncubator *incubationTask);
    void deleteAllFinishedIncubationTasks();
    QQmlDelegateModelItem *resolveModelItem(int index);
    void destroyModelItem(QQmlDelegateModelItem *modelItem, DestructionMode mode);

    void dataChangedCallback(const QModelIndex &begin, const QModelIndex &end, const QVector<int> &roles);
    void modelAboutToBeResetCallback();
    void forceSetModel(const QVariant &model);

    static bool isDoneIncubating(QQmlDelegateModelItem *modelItem);
    static void deleteModelItemLater(QQmlDelegateModelItem *modelItem);

    friend class QQmlTableInstanceModelIncubationTask;
};

QT_END_NAMESPACE

#endif // QQMLTABLEINSTANCEMODEL_P_H
