// Copyright (C) 2016 Research In Motion.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLINSTANTIATOR_P_H
#define QQMLINSTANTIATOR_P_H

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

#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlparserstatus.h>

#include <private/qqmldelegatemodel_p.h>
#include <private/qtqmlmodelsglobal_p.h>

QT_REQUIRE_CONFIG(qml_object_model);

QT_BEGIN_NAMESPACE

class QQmlInstantiatorPrivate;
class Q_QMLMODELS_EXPORT QQmlInstantiator : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(bool active READ isActive WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(bool asynchronous READ isAsync WRITE setAsync NOTIFY asynchronousChanged)
    Q_PROPERTY(QVariant model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QQmlComponent *delegate READ delegate WRITE setDelegate NOTIFY delegateChanged)
    Q_PROPERTY(QObject *object READ object NOTIFY objectChanged)
#if QT_CONFIG(qml_delegate_model)
    Q_PROPERTY(QQmlDelegateModel::DelegateModelAccess delegateModelAccess READ delegateModelAccess
            WRITE setDelegateModelAccess NOTIFY delegateModelAccessChanged REVISION(6, 10) FINAL)
#endif
    Q_CLASSINFO("DefaultProperty", "delegate")
    QML_NAMED_ELEMENT(Instantiator)
    QML_ADDED_IN_VERSION(2, 1)

public:
    QQmlInstantiator(QObject *parent = nullptr);
    ~QQmlInstantiator();

    bool isActive() const;
    void setActive(bool newVal);

    bool isAsync() const;
    void setAsync(bool newVal);

    int count() const;

    QQmlComponent* delegate();
    void setDelegate(QQmlComponent* c);

    QVariant model() const;
    void setModel(const QVariant &v);

#if QT_CONFIG(qml_delegate_model)
    QQmlDelegateModel::DelegateModelAccess delegateModelAccess() const;
    void setDelegateModelAccess(QQmlDelegateModel::DelegateModelAccess delegateModelAccess);
#endif

    QObject *object() const;

    Q_INVOKABLE QObject *objectAt(int index) const;

    void classBegin() override;
    void componentComplete() override;

Q_SIGNALS:
    void modelChanged();
    void delegateChanged();
    void countChanged();
    void objectChanged();
    void activeChanged();
    void asynchronousChanged();

    void objectAdded(int index, QObject* object);
    void objectRemoved(int index, QObject* object);

#if QT_CONFIG(qml_delegate_model)
    Q_REVISION(6, 10) void delegateModelAccessChanged();
#endif

private:
    Q_DISABLE_COPY(QQmlInstantiator)
    Q_DECLARE_PRIVATE(QQmlInstantiator)
    Q_PRIVATE_SLOT(d_func(), void _q_createdItem(int, QObject *))
    Q_PRIVATE_SLOT(d_func(), void _q_modelUpdated(const QQmlChangeSet &, bool))
};

QT_END_NAMESPACE

#endif // QQMLCREATOR_P_H
