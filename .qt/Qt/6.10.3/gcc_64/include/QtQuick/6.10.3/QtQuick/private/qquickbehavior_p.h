// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKBEHAVIOR_H
#define QQUICKBEHAVIOR_H

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

#include <private/qtquickglobal_p.h>

#include <private/qqmlpropertyvalueinterceptor_p.h>
#include <private/qqmlengine_p.h>
#include <qqml.h>
#include <private/qqmlfinalizer_p.h>

QT_BEGIN_NAMESPACE

class QQuickAbstractAnimation;
class QQuickBehaviorPrivate;
class Q_QUICK_EXPORT QQuickBehavior : public QObject, public QQmlPropertyValueInterceptor, public QQmlFinalizerHook
{
    Q_OBJECT
    Q_INTERFACES(QQmlFinalizerHook)
    Q_DECLARE_PRIVATE(QQuickBehavior)

    Q_INTERFACES(QQmlPropertyValueInterceptor)
    Q_CLASSINFO("DefaultProperty", "animation")
    Q_PROPERTY(QQuickAbstractAnimation *animation READ animation WRITE setAnimation)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QVariant targetValue READ targetValue NOTIFY targetValueChanged REVISION(2, 13))
    Q_PROPERTY(QQmlProperty targetProperty READ targetProperty NOTIFY targetPropertyChanged REVISION(2, 15))
    Q_CLASSINFO("DeferredPropertyNames", "animation")
    QML_NAMED_ELEMENT(Behavior)
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickBehavior(QObject *parent=nullptr);
    ~QQuickBehavior();

    void setTarget(const QQmlProperty &) override;
    void write(const QVariant &value) override;
    bool bindable(QUntypedBindable *untypedBindable, QUntypedBindable target) override;

    QQuickAbstractAnimation *animation();
    void setAnimation(QQuickAbstractAnimation *);

    bool enabled() const;
    void setEnabled(bool enabled);

    QVariant targetValue() const;

    QQmlProperty targetProperty() const;

    void componentFinalized() override;

Q_SIGNALS:
    void enabledChanged();
    void targetValueChanged();
    void targetPropertyChanged();
};

QT_END_NAMESPACE

#endif // QQUICKBEHAVIOR_H
