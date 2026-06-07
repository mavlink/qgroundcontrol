// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPROPERTY_P_H
#define QPROPERTY_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <private/qglobal_p.h>
#include <qproperty.h>

#include <qmetaobject.h>
#include <qscopedvaluerollback.h>
#include <qvariant.h>
#include <vector>
#include <QtCore/QVarLengthArray>

#include <memory>

QT_BEGIN_NAMESPACE

namespace QtPrivate {
    Q_CORE_EXPORT bool isAnyBindingEvaluating();
    struct QBindingStatusAccessToken {};

    namespace BindableWarnings {
    Q_CORE_EXPORT void printSignalArgumentsWithCustomGetter();
    }
}


/*!
    \internal
    Similar to \c QPropertyBindingPrivatePtr, but stores a
    \c QPropertyObserver * linking to the QPropertyBindingPrivate*
    instead of the QPropertyBindingPrivate* itself
 */
struct QBindingObserverPtr
{
private:
    QPropertyObserver *d = nullptr;
public:
    QBindingObserverPtr() = default;
    Q_DISABLE_COPY(QBindingObserverPtr)
    void swap(QBindingObserverPtr &other) noexcept
    { qt_ptr_swap(d, other.d); }
    QBindingObserverPtr(QBindingObserverPtr &&other) noexcept : d(std::exchange(other.d, nullptr)) {}
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(QBindingObserverPtr)


    inline QBindingObserverPtr(QPropertyObserver *observer) noexcept;
    inline ~QBindingObserverPtr();
    inline QPropertyBindingPrivate *binding() const noexcept;
    inline QPropertyObserver *operator ->();
};

using PendingBindingObserverList = QVarLengthArray<QPropertyBindingPrivatePtr>;

// Keep all classes related to QProperty in one compilation unit. Performance of this code is crucial and
// we need to allow the compiler to inline where it makes sense.

// This is a helper "namespace"
struct QPropertyBindingDataPointer
{
    const QtPrivate::QPropertyBindingData *ptr = nullptr;

    QPropertyBindingPrivate *binding() const
    {
        return ptr->binding();
    }

    void setObservers(QPropertyObserver *observer)
    {
        auto &d = ptr->d_ref();
        observer->prev = reinterpret_cast<QPropertyObserver**>(&d);
        d = reinterpret_cast<quintptr>(observer);
    }
    static void fixupAfterMove(QtPrivate::QPropertyBindingData *ptr);
    Q_ALWAYS_INLINE void addObserver(QPropertyObserver *observer);
    inline void setFirstObserver(QPropertyObserver *observer);
    inline QPropertyObserverPointer firstObserver() const;
    static QPropertyProxyBindingData *proxyData(QtPrivate::QPropertyBindingData *ptr);

    inline int observerCount() const;

    template <typename T>
    static QPropertyBindingDataPointer get(QProperty<T> &property)
    {
        return QPropertyBindingDataPointer{&property.bindingData()};
    }
};

struct QPropertyObserverNodeProtector
{
    Q_DISABLE_COPY_MOVE(QPropertyObserverNodeProtector)

    QPropertyObserverBase m_placeHolder;
    Q_NODISCARD_CTOR
    QPropertyObserverNodeProtector(QPropertyObserver *observer)
    {
        // insert m_placeholder after observer into the linked list
        QPropertyObserver *next = observer->next.data();
        m_placeHolder.next = next;
        observer->next = static_cast<QPropertyObserver *>(&m_placeHolder);
        if (next)
            next->prev = &m_placeHolder.next;
        m_placeHolder.prev = &observer->next;
        m_placeHolder.next.setTag(QPropertyObserver::ObserverIsPlaceholder);
    }

    QPropertyObserver *next() const { return m_placeHolder.next.data(); }

    ~QPropertyObserverNodeProtector();
};

// This is a helper "namespace"
struct QPropertyObserverPointer
{
    QPropertyObserver *ptr = nullptr;

    void unlink()
    {
        unlink_common();
#if QT_DEPRECATED_SINCE(6, 6)
        QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED
        if (ptr->next.tag() == QPropertyObserver::ObserverIsAlias)
            ptr->aliasData = nullptr;
        QT_WARNING_POP
#endif
    }

    void unlink_fast()
    {
#if QT_DEPRECATED_SINCE(6, 6)
        QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED
        Q_ASSERT(ptr->next.tag() != QPropertyObserver::ObserverIsAlias);
        QT_WARNING_POP
#endif
        unlink_common();
    }

    void setBindingToNotify(QPropertyBindingPrivate *binding)
    {
        Q_ASSERT(ptr->next.tag() != QPropertyObserver::ObserverIsPlaceholder);
        ptr->binding = binding;
        ptr->next.setTag(QPropertyObserver::ObserverNotifiesBinding);
    }

    void setBindingToNotify_unsafe(QPropertyBindingPrivate *binding);
    void setChangeHandler(QPropertyObserver::ChangeHandler changeHandler);

    enum class Notify {Everything, OnlyChangeHandlers};

    void notify(QUntypedPropertyData *propertyDataPtr);
#ifndef QT_NO_DEBUG
    void noSelfDependencies(QPropertyBindingPrivate *binding);
#else
    void noSelfDependencies(QPropertyBindingPrivate *) {}
#endif
    void evaluateBindings(PendingBindingObserverList &bindingObservers, QBindingStatus *status);
    void observeProperty(QPropertyBindingDataPointer property);

    explicit operator bool() const { return ptr != nullptr; }

    QPropertyObserverPointer nextObserver() const { return {ptr->next.data()}; }

    QPropertyBindingPrivate *binding() const
    {
        Q_ASSERT(ptr->next.tag() == QPropertyObserver::ObserverNotifiesBinding);
        return ptr->binding;
    }

private:
    void unlink_common()
    {
        if (ptr->next)
            ptr->next->prev = ptr->prev;
        if (ptr->prev)
            ptr->prev.setPointer(ptr->next.data());
        ptr->next = nullptr;
        ptr->prev.clear();
    }
};

class QPropertyBindingErrorPrivate : public QSharedData
{
public:
    QPropertyBindingError::Type type = QPropertyBindingError::NoError;
    QString description;
};

namespace QtPrivate {

struct BindingEvaluationState
{
    BindingEvaluationState(QPropertyBindingPrivate *binding, QBindingStatus *status);
    ~BindingEvaluationState()
    {
        *currentState = previousState;
    }

    QPropertyBindingPrivate *binding;
    BindingEvaluationState *previousState = nullptr;
    BindingEvaluationState **currentState = nullptr;
    QVarLengthArray<const QPropertyBindingData *, 8> alreadyCaptureProperties;
};

/*!
 * \internal
 * CompatPropertySafePoint needs to be constructed before the setter of
 * a QObjectCompatProperty runs. It prevents spurious binding dependencies
 * caused by reads of properties inside the compat property setter.
 * Moreover, it ensures that we don't destroy bindings when using operator=
 */
struct CompatPropertySafePoint
{
    Q_CORE_EXPORT CompatPropertySafePoint(QBindingStatus *status, QUntypedPropertyData *property);
    ~CompatPropertySafePoint()
    {
        *currentState = previousState;
        *currentlyEvaluatingBindingList = bindingState;
    }
    QUntypedPropertyData *property;
    CompatPropertySafePoint *previousState = nullptr;
    CompatPropertySafePoint **currentState = nullptr;
    QtPrivate::BindingEvaluationState **currentlyEvaluatingBindingList = nullptr;
    QtPrivate::BindingEvaluationState *bindingState = nullptr;
};

/*!
 * \internal
 * While the regular QProperty notification for a compat property runs we
 * don't want to have any currentCompatProperty set. This would be a _different_
 * one than the one we are current evaluating. Therefore it's misleading and
 * prevents the registering of actual dependencies.
 */
struct CurrentCompatPropertyThief
{
    Q_DISABLE_COPY_MOVE(CurrentCompatPropertyThief)
    QScopedValueRollback<CompatPropertySafePoint *> m_guard;
public:
    Q_NODISCARD_CTOR
    CurrentCompatPropertyThief(QBindingStatus *status)
        : m_guard(status->currentCompatProperty, nullptr)
    {
    }
};

}

class Q_CORE_EXPORT QPropertyBindingPrivate : public QtPrivate::RefCounted
{
private:
    friend struct QPropertyBindingDataPointer;
    friend class QPropertyBindingPrivatePtr;

    using ObserverArray = std::array<QPropertyObserver, 4>;

private:

    // used to detect binding loops for lazy evaluated properties
    bool updating = false;
    bool hasStaticObserver = false;
    bool pendingNotify = false;
    bool hasBindingWrapper:1;
    // used to detect binding loops for eagerly evaluated properties
    bool isQQmlPropertyBinding:1;
    /* a sticky binding does not get removed in removeBinding
       this is used to support QQmlPropertyData::DontRemoveBinding
       in qtdeclarative
    */
    bool m_sticky:1;

    const QtPrivate::BindingFunctionVTable *vtable;

    union {
        QtPrivate::QPropertyObserverCallback staticObserverCallback = nullptr;
        QtPrivate::QPropertyBindingWrapper staticBindingWrapper;
    };
    ObserverArray inlineDependencyObservers; // for things we are observing

    QPropertyObserverPointer firstObserver; // list of observers observing us
    std::unique_ptr<std::vector<QPropertyObserver>> heapObservers; // for things we are observing

protected:
    QUntypedPropertyData *propertyDataPtr = nullptr;

    /* For bindings set up from C++, location stores where the binding was created in the C++ source
       For QQmlPropertyBinding that information does not make sense, and the location in the QML  file
       is stored somewhere else. To make efficient use of the space, we instead provide a scratch space
       for QQmlPropertyBinding (which stores further binding information there).
       Anything stored in the union must be trivially destructible.
       (checked in qproperty.cpp)
    */
    using DeclarativeErrorCallback = void(*)(QPropertyBindingPrivate *);
    union {
        QPropertyBindingSourceLocation location;
        struct {
            std::byte declarativeExtraData[sizeof(QPropertyBindingSourceLocation) - sizeof(DeclarativeErrorCallback)];
            DeclarativeErrorCallback errorCallBack;
        };
    };
private:
    QPropertyBindingError m_error;

    QMetaType metaType;

public:
    static constexpr size_t getSizeEnsuringAlignment() {
        constexpr auto align = alignof (std::max_align_t) - 1;
        constexpr size_t sizeEnsuringAlignment = (sizeof(QPropertyBindingPrivate) + align) & ~align;
        static_assert (sizeEnsuringAlignment % alignof (std::max_align_t) == 0,
                   "Required for placement new'ing the function behind it.");
        return sizeEnsuringAlignment;
    }


    // public because the auto-tests access it, too.
    size_t dependencyObserverCount = 0;

    bool isUpdating() {return updating;}
    void setSticky(bool keep = true) {m_sticky = keep;}
    bool isSticky() {return m_sticky;}
    void scheduleNotify() {pendingNotify = true;}

    QPropertyBindingPrivate(QMetaType metaType, const QtPrivate::BindingFunctionVTable *vtable,
                            const QPropertyBindingSourceLocation &location, bool isQQmlPropertyBinding=false)
        : hasBindingWrapper(false)
        , isQQmlPropertyBinding(isQQmlPropertyBinding)
        , m_sticky(false)
        , vtable(vtable)
        , location(location)
        , metaType(metaType)
    {}
    ~QPropertyBindingPrivate();


    void setProperty(QUntypedPropertyData *propertyPtr) { propertyDataPtr = propertyPtr; }
    void setStaticObserver(QtPrivate::QPropertyObserverCallback callback, QtPrivate::QPropertyBindingWrapper bindingWrapper)
    {
        Q_ASSERT(!(callback && bindingWrapper));
        if (callback) {
            hasStaticObserver = true;
            hasBindingWrapper = false;
            staticObserverCallback = callback;
        } else if (bindingWrapper) {
            hasStaticObserver = false;
            hasBindingWrapper = true;
            staticBindingWrapper = bindingWrapper;
        } else {
            hasStaticObserver = false;
            hasBindingWrapper = false;
            staticObserverCallback = nullptr;
        }
    }
    void prependObserver(QPropertyObserverPointer observer)
    {
        observer.ptr->prev = const_cast<QPropertyObserver **>(&firstObserver.ptr);
        firstObserver = observer;
    }

    QPropertyObserverPointer takeObservers()
    {
        auto observers = firstObserver;
        firstObserver.ptr = nullptr;
        return observers;
    }

    void clearDependencyObservers();

    Q_ALWAYS_INLINE QPropertyObserverPointer allocateDependencyObserver() {
        if (dependencyObserverCount < inlineDependencyObservers.size()) {
            ++dependencyObserverCount;
            return {&inlineDependencyObservers[dependencyObserverCount - 1]};
        }
        return allocateDependencyObserver_slow();
    }

    QPropertyObserverPointer allocateDependencyObserver_slow();

    QPropertyBindingSourceLocation sourceLocation() const
    {
        if (!hasCustomVTable())
            return location;
        QPropertyBindingSourceLocation result;
        constexpr auto msg = "Custom location";
        result.fileName = msg;
        return result;
    }
    QPropertyBindingError bindingError() const { return m_error; }
    QMetaType valueMetaType() const { return metaType; }

    void unlinkAndDeref();

    bool evaluateRecursive(PendingBindingObserverList &bindingObservers, QBindingStatus *status = nullptr);

    Q_ALWAYS_INLINE bool evaluateRecursive_inline(PendingBindingObserverList &bindingObservers, QBindingStatus *status);

    void notifyNonRecursive(const PendingBindingObserverList &bindingObservers);
    enum NotificationState : bool { Delayed, Sent };
    NotificationState notifyNonRecursive();

    static QPropertyBindingPrivate *get(const QUntypedPropertyBinding &binding)
    { return static_cast<QPropertyBindingPrivate *>(binding.d.data()); }

    void setError(QPropertyBindingError &&e)
    { m_error = std::move(e); }

    void detachFromProperty()
    {
        hasStaticObserver = false;
        hasBindingWrapper = false;
        propertyDataPtr = nullptr;
        clearDependencyObservers();
    }

    static QPropertyBindingPrivate *currentlyEvaluatingBinding();

    bool hasCustomVTable() const
    {
        return vtable->size == 0;
    }

    static void destroyAndFreeMemory(QPropertyBindingPrivate *priv) {
        if (priv->hasCustomVTable()) {
            // special hack for QQmlPropertyBinding which has a
            // different memory layout than normal QPropertyBindings
            priv->vtable->destroy(priv);
        } else{
            priv->~QPropertyBindingPrivate();
            delete[] reinterpret_cast<std::byte *>(priv);
        }
    }
};

inline void QPropertyBindingDataPointer::setFirstObserver(QPropertyObserver *observer)
{
    if (auto *b = binding()) {
        b->firstObserver.ptr = observer;
        return;
    }
    auto &d = ptr->d_ref();
    d = reinterpret_cast<quintptr>(observer);
}

inline void QPropertyBindingDataPointer::fixupAfterMove(QtPrivate::QPropertyBindingData *ptr)
{
    auto &d = ptr->d_ref();
    if (ptr->isNotificationDelayed()) {
        QPropertyProxyBindingData *proxy = ptr->proxyData();
        Q_ASSERT(proxy);
        proxy->originalBindingData = ptr;
    }
    // If QPropertyBindingData has been moved, and it has an observer
    // we have to adjust the firstObserver's prev pointer to point to
    // the moved to QPropertyBindingData's d_ptr
    if (d & QtPrivate::QPropertyBindingData::BindingBit)
        return; // nothing to do if the observer is stored in the binding
    if (auto observer = reinterpret_cast<QPropertyObserver *>(d))
        observer->prev = reinterpret_cast<QPropertyObserver **>(&d);
}

inline QPropertyObserverPointer QPropertyBindingDataPointer::firstObserver() const
{
    if (auto *b = binding())
        return b->firstObserver;
    return { reinterpret_cast<QPropertyObserver *>(ptr->d()) };
}

/*!
    \internal
    Returns the proxy data of \a ptr, or \c nullptr if \a ptr has no delayed notification
 */
inline QPropertyProxyBindingData *QPropertyBindingDataPointer::proxyData(QtPrivate::QPropertyBindingData *ptr)
{
    if (!ptr->isNotificationDelayed())
        return nullptr;
    return ptr->proxyData();
}

inline int QPropertyBindingDataPointer::observerCount() const
{
    int count = 0;
    for (auto observer = firstObserver(); observer; observer = observer.nextObserver())
        ++count;
    return count;
}

namespace QtPrivate {
    Q_CORE_EXPORT bool isPropertyInBindingWrapper(const QUntypedPropertyData *property);
    void Q_CORE_EXPORT initBindingStatusThreadId();
}

template<typename Class, typename T, auto Offset, auto Setter, auto Signal = nullptr,
         auto Getter = nullptr>
class QObjectCompatProperty : public QPropertyData<T>
{
    template<typename Property, typename>
    friend class QtPrivate::QBindableInterfaceForProperty;

    using ThisType = QObjectCompatProperty<Class, T, Offset, Setter, Signal, Getter>;
    using SignalTakesValue = std::is_invocable<decltype(Signal), Class, T>;
    Class *owner()
    {
        char *that = reinterpret_cast<char *>(this);
        return reinterpret_cast<Class *>(that - QtPrivate::detail::getOffset(Offset));
    }
    const Class *owner() const
    {
        char *that = const_cast<char *>(reinterpret_cast<const char *>(this));
        return reinterpret_cast<Class *>(that - QtPrivate::detail::getOffset(Offset));
    }

    static bool bindingWrapper(QMetaType type, QUntypedPropertyData *dataPtr, QtPrivate::QPropertyBindingFunction binding)
    {
        auto *thisData = static_cast<ThisType *>(dataPtr);
        QBindingStorage *storage = qGetBindingStorage(thisData->owner());
        QPropertyData<T> copy;
        {
            QtPrivate::CurrentCompatPropertyThief thief(storage->bindingStatus);
            binding.vtable->call(type, &copy, binding.functor);
            if constexpr (QTypeTraits::has_operator_equal_v<T>)
                if (copy.valueBypassingBindings() == thisData->valueBypassingBindings())
                    return false;
        }
        // ensure value and setValue know we're currently evaluating our binding
        QtPrivate::CompatPropertySafePoint guardThis(storage->bindingStatus, thisData);
        (thisData->owner()->*Setter)(copy.valueBypassingBindings());
        return true;
    }
    bool inBindingWrapper(const QBindingStorage *storage) const
    {
        return storage->bindingStatus && storage->bindingStatus->currentCompatProperty
            && QtPrivate::isPropertyInBindingWrapper(this);
    }

    inline static T getPropertyValue(const QUntypedPropertyData *d) {
        auto prop = static_cast<const ThisType *>(d);
        if constexpr (std::is_null_pointer_v<decltype(Getter)>)
            return prop->value();
        else
            return (prop->owner()->*Getter)();
    }

    inline static T getPropertyValueBypassingBindings(const QUntypedPropertyData *d) {
        auto prop = static_cast<const ThisType *>(d);
        if constexpr (std::is_null_pointer_v<decltype(Getter)>)
            return prop->valueBypassingBindings();
        else
            return (prop->owner()->*Getter)();
    }

    inline static void warnIfSignalWithArgumentAndCustomGetter()
    {
        if constexpr (!std::is_null_pointer_v<decltype(Signal)>
                      && SignalTakesValue::value
                      && !std::is_null_pointer_v<decltype(Getter)>) {
            QtPrivate::BindableWarnings::printSignalArgumentsWithCustomGetter();
        }
    }

public:
    using value_type = typename QPropertyData<T>::value_type;
    using parameter_type = typename QPropertyData<T>::parameter_type;
    using arrow_operator_result = typename QPropertyData<T>::arrow_operator_result;

    QObjectCompatProperty() { warnIfSignalWithArgumentAndCustomGetter(); }
    explicit QObjectCompatProperty(const T &initialValue) : QPropertyData<T>(initialValue)
    {
        warnIfSignalWithArgumentAndCustomGetter();
    }
    explicit QObjectCompatProperty(T &&initialValue) : QPropertyData<T>(std::move(initialValue))
    {
        warnIfSignalWithArgumentAndCustomGetter();
    }

    parameter_type value() const
    {
        const QBindingStorage *storage = qGetBindingStorage(owner());
        // make sure we don't register this binding as a dependency to itself
        if (storage->bindingStatus && storage->bindingStatus->currentlyEvaluatingBinding && !inBindingWrapper(storage))
            storage->registerDependency_helper(this);
        return this->val;
    }

    arrow_operator_result operator->() const
    {
        if constexpr (QTypeTraits::is_dereferenceable_v<T>) {
            return value();
        } else if constexpr (std::is_pointer_v<T>) {
            value();
            return this->val;
        } else {
            return;
        }
    }

    parameter_type operator*() const
    {
        return value();
    }

    operator parameter_type() const
    {
        return value();
    }

    void setValue(parameter_type t)
    {
        QBindingStorage *storage = qGetBindingStorage(owner());
        if (auto *bd = storage->bindingData(this)) {
            // make sure we don't remove the binding if called from the bindingWrapper
            if (bd->hasBinding() && !inBindingWrapper(storage))
                bd->removeBinding_helper();
        }
        this->val = t;
    }

    QObjectCompatProperty &operator=(parameter_type newValue)
    {
        setValue(newValue);
        return *this;
    }

    QPropertyBinding<T> setBinding(const QPropertyBinding<T> &newBinding)
    {
        QtPrivate::QPropertyBindingData *bd = qGetBindingStorage(owner())->bindingData(this, true);
        QUntypedPropertyBinding oldBinding(bd->setBinding(newBinding, this, nullptr, bindingWrapper));
        // notification is already handled in QPropertyBindingData::setBinding
        return static_cast<QPropertyBinding<T> &>(oldBinding);
    }

    bool setBinding(const QUntypedPropertyBinding &newBinding)
    {
        if (!newBinding.isNull() && newBinding.valueMetaType() != QMetaType::fromType<T>())
            return false;
        setBinding(static_cast<const QPropertyBinding<T> &>(newBinding));
        return true;
    }

#ifndef Q_QDOC
    template <typename Functor>
    QPropertyBinding<T> setBinding(Functor &&f,
                                   const QPropertyBindingSourceLocation &location = QT_PROPERTY_DEFAULT_BINDING_LOCATION,
                                   std::enable_if_t<std::is_invocable_v<Functor>> * = nullptr)
    {
        return setBinding(Qt::makePropertyBinding(std::forward<Functor>(f), location));
    }
#else
    template <typename Functor>
    QPropertyBinding<T> setBinding(Functor f);
#endif

    bool hasBinding() const {
        auto *bd = qGetBindingStorage(owner())->bindingData(this);
        return bd && bd->binding() != nullptr;
    }

    void removeBindingUnlessInWrapper()
    {
        QBindingStorage *storage = qGetBindingStorage(owner());
        if (auto *bd = storage->bindingData(this)) {
            // make sure we don't remove the binding if called from the bindingWrapper
            if (bd->hasBinding() && !inBindingWrapper(storage))
                bd->removeBinding_helper();
        }
    }

    void notify()
    {
        QBindingStorage *storage = qGetBindingStorage(owner());
        if (auto bd = storage->bindingData(this, false)) {
            // This partly duplicates QPropertyBindingData::notifyObservers because we want to
            // check for inBindingWrapper() after checking for isNotificationDelayed() and
            // firstObserver. This is because inBindingWrapper() is the most expensive check.
            if (!bd->isNotificationDelayed()) {
                QPropertyBindingDataPointer d{bd};
                if (QPropertyObserverPointer observer = d.firstObserver()) {
                    if (!inBindingWrapper(storage)) {
                        PendingBindingObserverList bindingObservers;
                        if (bd->notifyObserver_helper(this, storage, observer, bindingObservers)
                                == QtPrivate::QPropertyBindingData::Evaluated) {
                            // evaluateBindings() can trash the observers.
                            // It can also reallocate binding data pointer.
                            // So, we need to re-fetch here.
                            bd = storage->bindingData(this, false);
                            QPropertyBindingDataPointer dd{bd};
                            if (QPropertyObserverPointer obs = dd.firstObserver())
                                obs.notify(this);
                            for (auto&& bindingPtr: bindingObservers) {
                                auto *binding = static_cast<QPropertyBindingPrivate *>(bindingPtr.get());
                                binding->notifyNonRecursive();
                            }
                        }
                    }
                }
            }
        }
        if constexpr (!std::is_null_pointer_v<decltype(Signal)>) {
            if constexpr (SignalTakesValue::value)
                (owner()->*Signal)(getPropertyValueBypassingBindings(this));
            else
                (owner()->*Signal)();
        }
    }

    QPropertyBinding<T> binding() const
    {
        auto *bd = qGetBindingStorage(owner())->bindingData(this);
        return static_cast<QPropertyBinding<T> &&>(QUntypedPropertyBinding(bd ? bd->binding() : nullptr));
    }

    QPropertyBinding<T> takeBinding()
    {
        return setBinding(QPropertyBinding<T>());
    }

    template<typename Functor>
    QPropertyChangeHandler<Functor> onValueChanged(Functor f)
    {
        static_assert(std::is_invocable_v<Functor>, "Functor callback must be callable without any parameters");
        return QPropertyChangeHandler<Functor>(*this, f);
    }

    template<typename Functor>
    QPropertyChangeHandler<Functor> subscribe(Functor f)
    {
        static_assert(std::is_invocable_v<Functor>, "Functor callback must be callable without any parameters");
        f();
        return onValueChanged(f);
    }

    template<typename Functor>
    QPropertyNotifier addNotifier(Functor f)
    {
        static_assert(std::is_invocable_v<Functor>, "Functor callback must be callable without any parameters");
        return QPropertyNotifier(*this, f);
    }

    QtPrivate::QPropertyBindingData &bindingData() const
    {
        auto *storage = const_cast<QBindingStorage *>(qGetBindingStorage(owner()));
        return *storage->bindingData(const_cast<QObjectCompatProperty *>(this), true);
    }
};

namespace QtPrivate {
template<typename Class, typename Ty, auto Offset, auto Setter, auto Signal, auto Getter>
class QBindableInterfaceForProperty<
        QObjectCompatProperty<Class, Ty, Offset, Setter, Signal, Getter>, std::void_t<Class>>
{
    using Property = QObjectCompatProperty<Class, Ty, Offset, Setter, Signal, Getter>;
    using T = typename Property::value_type;
public:
    static constexpr QBindableInterface iface = {
        [](const QUntypedPropertyData *d, void *value) -> void
        { *static_cast<T*>(value) = Property::getPropertyValue(d); },
        [](QUntypedPropertyData *d, const void *value) -> void
        {
            (static_cast<Property *>(d)->owner()->*Setter)(*static_cast<const T*>(value));
        },
        [](const QUntypedPropertyData *d) -> QUntypedPropertyBinding
        { return static_cast<const Property *>(d)->binding(); },
        [](QUntypedPropertyData *d, const QUntypedPropertyBinding &binding) -> QUntypedPropertyBinding
        { return static_cast<Property *>(d)->setBinding(static_cast<const QPropertyBinding<T> &>(binding)); },
        [](const QUntypedPropertyData *d, const QPropertyBindingSourceLocation &location) -> QUntypedPropertyBinding
        { return Qt::makePropertyBinding([d]() -> T { return Property::getPropertyValue(d); }, location); },
        [](const QUntypedPropertyData *d, QPropertyObserver *observer) -> void
        { observer->setSource(static_cast<const Property *>(d)->bindingData()); },
        []() { return QMetaType::fromType<T>(); }
    };
};
}

#define QT_OBJECT_COMPAT_PROPERTY_4(Class, Type, name,  setter) \
    static constexpr size_t _qt_property_##name##_offset() { \
        QT_WARNING_PUSH QT_WARNING_DISABLE_INVALID_OFFSETOF \
        return offsetof(Class, name); \
        QT_WARNING_POP \
    } \
    QObjectCompatProperty<Class, Type, Class::_qt_property_##name##_offset, setter> name;

#define QT_OBJECT_COMPAT_PROPERTY_5(Class, Type, name,  setter, signal) \
    static constexpr size_t _qt_property_##name##_offset() { \
        QT_WARNING_PUSH QT_WARNING_DISABLE_INVALID_OFFSETOF \
        return offsetof(Class, name); \
        QT_WARNING_POP \
    } \
    QObjectCompatProperty<Class, Type, Class::_qt_property_##name##_offset, setter, signal> name;

#define Q_OBJECT_COMPAT_PROPERTY(...) \
    QT_WARNING_PUSH QT_WARNING_DISABLE_INVALID_OFFSETOF \
    QT_OVERLOADED_MACRO(QT_OBJECT_COMPAT_PROPERTY, __VA_ARGS__) \
    QT_WARNING_POP

#define QT_OBJECT_COMPAT_PROPERTY_WITH_ARGS_5(Class, Type, name,  setter, value) \
    static constexpr size_t _qt_property_##name##_offset() { \
        QT_WARNING_PUSH QT_WARNING_DISABLE_INVALID_OFFSETOF \
        return offsetof(Class, name); \
        QT_WARNING_POP \
    } \
    QObjectCompatProperty<Class, Type, Class::_qt_property_##name##_offset, setter> name =         \
            QObjectCompatProperty<Class, Type, Class::_qt_property_##name##_offset, setter>(       \
                    value);

#define QT_OBJECT_COMPAT_PROPERTY_WITH_ARGS_6(Class, Type, name, setter, signal, value) \
    static constexpr size_t _qt_property_##name##_offset() { \
        QT_WARNING_PUSH QT_WARNING_DISABLE_INVALID_OFFSETOF \
        return offsetof(Class, name); \
        QT_WARNING_POP \
    } \
    QObjectCompatProperty<Class, Type, Class::_qt_property_##name##_offset, setter, signal> name = \
            QObjectCompatProperty<Class, Type, Class::_qt_property_##name##_offset, setter,        \
                                  signal>(value);

#define QT_OBJECT_COMPAT_PROPERTY_WITH_ARGS_7(Class, Type, name, setter, signal, getter, value) \
    static constexpr size_t _qt_property_##name##_offset() { \
        QT_WARNING_PUSH QT_WARNING_DISABLE_INVALID_OFFSETOF \
        return offsetof(Class, name); \
        QT_WARNING_POP \
    } \
    QObjectCompatProperty<Class, Type, Class::_qt_property_##name##_offset, setter, signal, getter>\
        name = QObjectCompatProperty<Class, Type, Class::_qt_property_##name##_offset, setter,     \
                                     signal, getter>(value);

#define Q_OBJECT_COMPAT_PROPERTY_WITH_ARGS(...) \
    QT_WARNING_PUSH QT_WARNING_DISABLE_INVALID_OFFSETOF \
    QT_OVERLOADED_MACRO(QT_OBJECT_COMPAT_PROPERTY_WITH_ARGS, __VA_ARGS__) \
    QT_WARNING_POP


namespace QtPrivate {
Q_CORE_EXPORT BindingEvaluationState *suspendCurrentBindingStatus();
Q_CORE_EXPORT void restoreBindingStatus(BindingEvaluationState *status);
}

struct QUntypedBindablePrivate
{
    static QtPrivate::QBindableInterface const *getInterface(const QUntypedBindable &bindable)
    {
        return bindable.iface;
    }

    static QUntypedPropertyData *getPropertyData(const QUntypedBindable &bindable)
    {
        return bindable.data;
    }
};

inline bool QPropertyBindingPrivate::evaluateRecursive_inline(PendingBindingObserverList &bindingObservers, QBindingStatus *status)
{
    if (updating) {
        m_error = QPropertyBindingError(QPropertyBindingError::BindingLoop);
        if (isQQmlPropertyBinding)
            errorCallBack(this);
        return false;
    }

    /*
     * Evaluating the binding might lead to the binding being broken. This can
     * cause ref to reach zero at the end of the function.  However, the
     * updateGuard's destructor will then still trigger, trying to set the
     * updating bool to its old value
     * To prevent this, we create a QPropertyBindingPrivatePtr which ensures
     * that the object is still alive when updateGuard's dtor runs.
     */
    QPropertyBindingPrivatePtr keepAlive {this};

    QScopedValueRollback<bool> updateGuard(updating, true);

    QtPrivate::BindingEvaluationState evaluationFrame(this, status);

    auto bindingFunctor =  reinterpret_cast<std::byte *>(this) +
            QPropertyBindingPrivate::getSizeEnsuringAlignment();
    bool changed = false;
    if (hasBindingWrapper) {
        changed = staticBindingWrapper(metaType, propertyDataPtr,
                                       {vtable, bindingFunctor});
    } else {
        changed = vtable->call(metaType, propertyDataPtr, bindingFunctor);
    }
    // If there was a change, we must set pendingNotify.
    // If there was not, we must not clear it, as that only should happen in notifyRecursive
    pendingNotify = pendingNotify || changed;
    if (!changed || !firstObserver)
        return changed;

    firstObserver.noSelfDependencies(this);
    firstObserver.evaluateBindings(bindingObservers, status);
    return true;
}

/*!
    \internal

    Walks through the list of property observers, and calls any ChangeHandler
    found there.
    It doesn't do anything with bindings, which are only handled in
    QPropertyBindingPrivate::evaluateRecursive.
 */
inline void QPropertyObserverPointer::notify(QUntypedPropertyData *propertyDataPtr)
{
    auto observer = const_cast<QPropertyObserver*>(ptr);
    /*
     * The basic idea of the loop is as follows: We iterate over all observers in the linked list,
     * and execute the functionality corresponding to their tag.
     * However, complication arise due to the fact that the triggered operations might modify the list,
     * which includes deletion and move of the current and next nodes.
     * Therefore, we take a few safety precautions:
     * 1. Before executing any action which might modify the list, we insert a placeholder node after the current node.
     *    As that one is stack allocated and owned by us, we can rest assured that it is
     *    still there after the action has executed, and placeHolder->next points to the actual next node in the list.
     *    Note that taking next at the beginning of the loop does not work, as the executed action might either move
     *    or delete that node.
     * 2. After the triggered action has finished, we can use the next pointer in the placeholder node as a safe way to
     *    retrieve the next node.
     * 3. Some care needs to be taken to avoid infinite recursion with change handlers, so we add an extra test there, that
     *    checks whether we're already have the same change handler in our call stack. This can be done by checking whether
     *    the node after the current one is a placeholder node.
     */
    while (observer) {
        QPropertyObserver *next = observer->next.data();
        switch (QPropertyObserver::ObserverTag(observer->next.tag())) {
        case QPropertyObserver::ObserverNotifiesChangeHandler:
        {
            auto handlerToCall = observer->changeHandler;
            // prevent recursion
            if (next && next->next.tag() == QPropertyObserver::ObserverIsPlaceholder) {
                observer = next->next.data();
                continue;
            }
            // handlerToCall might modify the list
            QPropertyObserverNodeProtector protector(observer);
            handlerToCall(observer, propertyDataPtr);
            next = protector.next();
            break;
        }
        case QPropertyObserver::ObserverNotifiesBinding:
            break;
        case QPropertyObserver::ObserverIsPlaceholder:
            // recursion is already properly handled somewhere else
            break;
#if QT_DEPRECATED_SINCE(6, 6)
        QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED
        case QPropertyObserver::ObserverIsAlias:
            break;
        QT_WARNING_POP
#endif
        default: Q_UNREACHABLE();
        }
        observer = next;
    }
}

inline QPropertyObserverNodeProtector::~QPropertyObserverNodeProtector()
{
    QPropertyObserverPointer d{static_cast<QPropertyObserver *>(&m_placeHolder)};
    d.unlink_fast();
}

QBindingObserverPtr::QBindingObserverPtr(QPropertyObserver *observer) noexcept : d(observer)
{
    Q_ASSERT(d);
    QPropertyObserverPointer{d}.binding()->addRef();
}

QBindingObserverPtr::~QBindingObserverPtr()
{
    if (!d)
        return;

    QPropertyBindingPrivate *bindingPrivate = binding();
    if (!bindingPrivate->deref())
        QPropertyBindingPrivate::destroyAndFreeMemory(bindingPrivate);
}

QPropertyBindingPrivate *QBindingObserverPtr::binding() const noexcept { return QPropertyObserverPointer{d}.binding(); }

QPropertyObserver *QBindingObserverPtr::operator->() { return d; }

namespace QtPrivate {
class QPropertyAdaptorSlotObject : public QUntypedPropertyData, public QSlotObjectBase
{
    QPropertyBindingData bindingData_;
    QObject *obj;
    QMetaProperty metaProperty_;

#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
    static void impl(int which, QSlotObjectBase *this_, QObject *r, void **a, bool *ret);
#else
    static void impl(QSlotObjectBase *this_, QObject *r, void **a, int which, bool *ret);
#endif

    QPropertyAdaptorSlotObject(QObject *o, const QMetaProperty& p);

public:
    static QPropertyAdaptorSlotObject *cast(QSlotObjectBase *ptr, int propertyIndex)
    {
        if (ptr->isImpl(&QPropertyAdaptorSlotObject::impl)) {
            auto p = static_cast<QPropertyAdaptorSlotObject *>(ptr);
            if (p->metaProperty_.propertyIndex() == propertyIndex)
                return p;
        }
        return nullptr;
    }

    inline const QPropertyBindingData &bindingData() const { return bindingData_; }
    inline QPropertyBindingData &bindingData() { return bindingData_; }
    inline QObject *object() const { return obj; }
    inline const QMetaProperty &metaProperty() const { return metaProperty_; }

    friend class QT_PREPEND_NAMESPACE(QUntypedBindable);
};
}

QT_END_NAMESPACE

#endif // QPROPERTY_P_H
