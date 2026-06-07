// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLABSTRACTBINDING_P_H
#define QQMLABSTRACTBINDING_P_H

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

#include <QtCore/qsharedpointer.h>
#include <QtCore/qshareddata.h>
#include <private/qtqmlglobal_p.h>
#include <private/qqmlproperty_p.h>

QT_BEGIN_NAMESPACE

class QQmlObjectCreator;
class QQmlAnyBinding;

class Q_QML_EXPORT QQmlAbstractBinding
{
    friend class QQmlAnyBinding;
protected:
    QQmlAbstractBinding();
public:
    enum Kind {
        ValueTypeProxy,
        QmlBinding,
        PropertyToPropertyBinding,
    };

    virtual ~QQmlAbstractBinding();

    typedef QExplicitlySharedDataPointer<QQmlAbstractBinding> Ptr;

    virtual QString expression() const;

    virtual Kind kind() const = 0;

    // Should return the encoded property index for the binding.  Should return this value
    // even if the binding is not enabled or added to an object.
    // Encoding is:  coreIndex | (valueTypeIndex << 16)
    QQmlPropertyIndex targetPropertyIndex() const { return m_targetIndex; }

    // Should return the object for the binding.  Should return this object even if the
    // binding is not enabled or added to the object.
    QObject *targetObject() const { return m_target.data(); }

    void setTarget(const QQmlProperty &);
    bool setTarget(QObject *, const QQmlPropertyData &, const QQmlPropertyData *valueType);
    bool setTarget(QObject *, int coreIndex, bool coreIsAlias, int valueTypeIndex);

    virtual void setEnabled(bool e, QQmlPropertyData::WriteFlags f = QQmlPropertyData::DontRemoveBinding) = 0;

    void addToObject();
    void removeFromObject();

    virtual void printBindingLoopError(const QQmlProperty &prop);

    inline QQmlAbstractBinding *nextBinding() const;

    inline bool canUseAccessor() const
    { return m_target.tag().testFlag(CanUseAccessor); }
    void setCanUseAccessor(bool canUseAccessor)
    { m_target.setTag(m_target.tag().setFlag(CanUseAccessor, canUseAccessor)); }

    bool isSticky() const { return m_target.tag().testFlag(IsSticky); }
    void setSticky(bool isSticky) { m_target.setTag(m_target.tag().setFlag(IsSticky, isSticky)); }

    struct RefCount {
        RefCount() {}
        int refCount = 0;
        void ref() { ++refCount; }
        int deref() { return --refCount; }
        operator int() const { return refCount; }
    };
    RefCount ref;

    // A binding can only be enabled if it's added to an object,
    // and it can only be updating if it's enabled.
    enum State {
        Disabled        = 0,
        AddedToObject   = 1,
        BindingEnabled  = 2,
        UpdatingBinding = 3,
    };

    enum TargetTag {
        NoTargetTag     = 0x0,
        CanUseAccessor  = 0x1,
        IsSticky        = 0x2,
    };
    Q_DECLARE_FLAGS(TargetTags, TargetTag)

protected:
    friend class QQmlData;
    friend class QQmlValueTypeProxyBinding;
    friend class QQmlObjectCreator;

    inline void setAddedToObject(bool v);
    inline bool isAddedToObject() const;

    inline void setNextBinding(QQmlAbstractBinding *);

    void getPropertyData(
            const QQmlPropertyData **propertyData, QQmlPropertyData *valueTypeData) const;

    inline bool updatingFlag() const;
    inline void setUpdatingFlag(bool);
    inline bool enabledFlag() const;
    inline void setEnabledFlag(bool);
    void updateCanUseAccessor();

    QQmlPropertyIndex m_targetIndex;

    // Pointer is the target object to which the binding binds
    QTaggedPointer<QObject, TargetTags> m_target;

    // Pointer to the next binding in the linked list of bindings.
    QTaggedPointer<QQmlAbstractBinding, State> m_nextBinding;

private:
    void setState(State state, bool enabled)
    {
        if (enabled) {
            m_nextBinding.setTag(std::max(m_nextBinding.tag(), state));
            return;
        }

        Q_ASSERT(state > 0);
        m_nextBinding.setTag(std::min(m_nextBinding.tag(), State(state - 1)));
    }

    bool hasState(State state) const
    {
        return m_nextBinding.tag() >= state;
    }
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QQmlAbstractBinding::TargetTags)

void QQmlAbstractBinding::setAddedToObject(bool v)
{
    setState(AddedToObject, v);
}

bool QQmlAbstractBinding::isAddedToObject() const
{
    return hasState(AddedToObject);
}

QQmlAbstractBinding *QQmlAbstractBinding::nextBinding() const
{
    return m_nextBinding.data();
}

void QQmlAbstractBinding::setNextBinding(QQmlAbstractBinding *b)
{
    if (b)
        b->ref.ref();
    if (m_nextBinding.data() && !m_nextBinding->ref.deref())
        delete m_nextBinding.data();
    m_nextBinding = b;
}

bool QQmlAbstractBinding::updatingFlag() const
{
    return hasState(UpdatingBinding);
}

void QQmlAbstractBinding::setUpdatingFlag(bool v)
{
    setState(UpdatingBinding, v);
}

bool QQmlAbstractBinding::enabledFlag() const
{
    return hasState(BindingEnabled);
}

void QQmlAbstractBinding::setEnabledFlag(bool v)
{
    setState(BindingEnabled, v);
}

QT_END_NAMESPACE

#endif // QQMLABSTRACTBINDING_P_H
