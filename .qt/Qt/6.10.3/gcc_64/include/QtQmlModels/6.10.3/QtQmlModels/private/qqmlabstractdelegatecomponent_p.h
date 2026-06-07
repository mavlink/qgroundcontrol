// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLABSTRACTDELEGATECOMPONENT_P_H
#define QQMLABSTRACTDELEGATECOMPONENT_P_H

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
#include <private/qqmlcomponentattached_p.h>
#include <qqmlcomponent.h>

QT_REQUIRE_CONFIG(qml_delegate_model);

QT_BEGIN_NAMESPACE

// TODO: consider making QQmlAbstractDelegateComponent public API
class QQmlAdaptorModel;
class Q_QMLMODELS_EXPORT QQmlAbstractDelegateComponent : public QQmlComponent
{
    Q_OBJECT
    QML_NAMED_ELEMENT(AbstractDelegateComponent)
    QML_ADDED_IN_VERSION(2, 0)
    QML_UNCREATABLE("Cannot create instance of abstract class AbstractDelegateComponent.")

public:
    QQmlAbstractDelegateComponent(QObject *parent = nullptr);
    ~QQmlAbstractDelegateComponent() override;

    virtual QQmlComponent *delegate(QQmlAdaptorModel *adaptorModel, int row, int column = 0) const = 0;
    virtual QString role() const = 0;

Q_SIGNALS:
    void delegateChanged();

protected:
    QVariant value(QQmlAdaptorModel *adaptorModel,int row, int column, const QString &role) const;
};

QT_END_NAMESPACE

#endif // QQMLABSTRACTDELEGATECOMPONENT_P_H
