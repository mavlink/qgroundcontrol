// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLPROPERTYTOPROPERTYBINDINDING_P_H
#define QQMLPROPERTYTOPROPERTYBINDINDING_P_H

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

#include <private/qqmlabstractbinding_p.h>
#include <private/qqmlengine_p.h>
#include <private/qqmlnotifier_p.h>
#include <private/qqmlvaluetype_p.h>
#include <private/qv4jscall_p.h>

#include <QtCore/qproperty.h>

QT_BEGIN_NAMESPACE

// Different forms of property-to-property binding
// unbindable -> unbindable: QQmlAbstractBinding     + QQmlNotifierEndpoint
// unbindable -> bindable:   QPropertyBindingPrivate + QQmlNotifierEndpoint
// bindable   -> unbindable: QQmlAbstractBinding     + QPropertyObserver
// bindable   -> bindable:   QPropertyBindingPrivate only

class QQmlPropertyToPropertyBinding
{
public:
    static Q_QML_EXPORT QQmlAnyBinding create(
            QQmlEngine *engine, const QQmlProperty &source, const QQmlProperty &target);

private:
    friend class QQmlPropertyToUnbindablePropertyBinding;
    friend class QQmlUnbindableToUnbindablePropertyBinding;
    friend class QQmlBindableToUnbindablePropertyBinding;
    friend class QQmlPropertyToBindablePropertyBinding;
    friend class QQmlUnbindableToBindablePropertyBinding;
    friend class QQmlBindableToBindablePropertyBinding;

    QQmlPropertyToPropertyBinding(QQmlEngine *engine, const QQmlProperty &source);

    template<typename Capture>
    QVariant readSourceValue(Capture &&capture) const
    {
        const QMetaObject *sourceMetaObject = sourceObject->metaObject();
        const QMetaProperty property
                = sourceMetaObject->property(sourcePropertyIndex.coreIndex());
        if (!property.isConstant())
            capture(sourceMetaObject, property);

        QVariant value;

        const int valueTypeIndex = sourcePropertyIndex.valueTypeIndex();
        if (valueTypeIndex == -1) {
            value = property.read(sourceObject);
        } else {
            QQmlGadgetPtrWrapper *wrapper
                    = QQmlEnginePrivate::get(engine)->valueTypeInstance(property.metaType());
            wrapper->read(sourceObject, sourcePropertyIndex.coreIndex());
            value = wrapper->readOnGadget(wrapper->property(valueTypeIndex));
        }

        return value;
    }

    void doConnectNotify(QQmlNotifierEndpoint *endpoint, const QMetaProperty &property)
    {
        const int notifyIndex = QMetaObjectPrivate::signalIndex(property.notifySignal());

        // We cannot capture non-bindable properties without signals
        if (notifyIndex == -1)
            return;

        if (endpoint->isConnected(sourceObject, notifyIndex))
            endpoint->cancelNotify();
        else
            endpoint->connect(sourceObject, notifyIndex, engine, true);
    }

    QQmlEngine *engine = nullptr;
    QObject *sourceObject = nullptr;
    QQmlPropertyIndex sourcePropertyIndex;
};

class QQmlPropertyToUnbindablePropertyBinding : public QQmlAbstractBinding
{
public:
    Kind kind() const final;
    void setEnabled(bool e, QQmlPropertyData::WriteFlags flags) final;
    void update(QQmlPropertyData::WriteFlags flags = QQmlPropertyData::DontRemoveBinding);

protected:
    QQmlPropertyToUnbindablePropertyBinding(
            QQmlEngine *engine, const QQmlProperty &source, const QQmlProperty &target);

    virtual void captureProperty(
            const QMetaObject *sourceMetaObject, const QMetaProperty &sourceProperty) = 0;

    QQmlPropertyToPropertyBinding m_binding;
};

class QQmlUnbindableToUnbindablePropertyBinding
    : public QQmlNotifierEndpoint, public QQmlPropertyToUnbindablePropertyBinding
{
public:
    QQmlUnbindableToUnbindablePropertyBinding(
            QQmlEngine *engine, const QQmlProperty &source, const QQmlProperty &target);
protected:
    void captureProperty(
            const QMetaObject *sourceMetaObject, const QMetaProperty &sourceProperty) final;
};

class QQmlBindableToUnbindablePropertyBinding
    : public QPropertyObserver, public QQmlPropertyToUnbindablePropertyBinding
{
public:
    QQmlBindableToUnbindablePropertyBinding(
            QQmlEngine *engine, const QQmlProperty &source, const QQmlProperty &target);

    static void update(QPropertyObserver *observer, QUntypedPropertyData *);

protected:
    void captureProperty(
            const QMetaObject *sourceMetaObject, const QMetaProperty &sourceProperty) final;
private:
    bool m_isObserving = false;
};

class QQmlPropertyToBindablePropertyBinding : public QPropertyBindingPrivate
{
public:
    QQmlPropertyToBindablePropertyBinding(
            QQmlEngine *engine, const QQmlProperty &source, const QQmlProperty &target,
            const QtPrivate::BindingFunctionVTable *vtable);

    template<typename Concrete>
    static bool update(QMetaType metaType, QUntypedPropertyData *dataPtr, void *f)
    {
        Concrete *self = reinterpret_cast<Concrete *>(
                // Address of QPropertyBindingPrivate subobject
                static_cast<std::byte *>(f) - QPropertyBindingPrivate::getSizeEnsuringAlignment());

        const QVariant value = self->m_binding.readSourceValue(
                [self](const QMetaObject *metaObject, const QMetaProperty &property) {
            self->captureProperty(metaObject, property);
        });

        QV4::coerce(
                self->m_binding.engine->handle(), value.metaType(), value.constData(),
                metaType, dataPtr);
        return true;
    }

protected:
    QQmlPropertyToPropertyBinding m_binding;
};

class QQmlUnbindableToBindablePropertyBinding
    : public QQmlPropertyToBindablePropertyBinding, public QQmlNotifierEndpoint
{
public:
    QQmlUnbindableToBindablePropertyBinding(
            QQmlEngine *engine, const QQmlProperty &source, const QQmlProperty &target);

    void update();

    void captureProperty(const QMetaObject *, const QMetaProperty &property)
    {
        m_binding.doConnectNotify(this, property);
    }
};

class QQmlBindableToBindablePropertyBinding : public QQmlPropertyToBindablePropertyBinding
{
public:
    QQmlBindableToBindablePropertyBinding(
            QQmlEngine *engine, const QQmlProperty &source, const QQmlProperty &target);

    void captureProperty(const QMetaObject *, const QMetaProperty &) {}
};

void QQmlUnbindableToUnbindableGuard_callback(QQmlNotifierEndpoint *e, void **);
void QQmlUnbindableToBindableGuard_callback(QQmlNotifierEndpoint *e, void **);

QT_END_NAMESPACE

#endif
