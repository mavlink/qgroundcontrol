// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPROPERTYPRIVATE_H
#define QPROPERTYPRIVATE_H

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

#include <QtCore/qglobal.h>
#include <QtCore/qtaggedpointer.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qcontainerfwd.h>
#include <QtCore/qttypetraits.h>

#include <functional>

QT_BEGIN_NAMESPACE

class QBindingStorage;

template<typename Class, typename T, auto Offset, auto Setter, auto Signal, auto Getter>
class QObjectCompatProperty;

class QPropertyBindingPrivatePtr;
using PendingBindingObserverList = QVarLengthArray<QPropertyBindingPrivatePtr>;

namespace QtPrivate {
// QPropertyBindingPrivatePtr operates on a RefCountingMixin solely so that we can inline
// the constructor and copy constructor
struct RefCounted {

    int refCount() const { return ref; }
    void addRef() { ++ref; }
    bool deref() { return --ref != 0; }

private:
    int ref = 0;
};
}

class QQmlPropertyBinding;
class QPropertyBindingPrivate;
class QPropertyBindingPrivatePtr
{
public:
    using T = QtPrivate::RefCounted;
    T &operator*() const { return *d; }
    T *operator->() noexcept { return d; }
    T *operator->() const noexcept { return d; }
    explicit operator T *() { return d; }
    explicit operator const T *() const noexcept { return d; }
    T *data() const noexcept { return d; }
    T *get() const noexcept { return d; }
    const T *constData() const noexcept { return d; }
    T *take() noexcept { T *x = d; d = nullptr; return x; }

    QPropertyBindingPrivatePtr() noexcept : d(nullptr) { }
    ~QPropertyBindingPrivatePtr()
    {
        if (d && !d->deref())
            destroyAndFreeMemory();
    }
    Q_CORE_EXPORT void destroyAndFreeMemory();

    explicit QPropertyBindingPrivatePtr(T *data) noexcept : d(data) { if (d) d->addRef(); }
    QPropertyBindingPrivatePtr(const QPropertyBindingPrivatePtr &o) noexcept
        : d(o.d) { if (d) d->addRef(); }

    void reset(T *ptr = nullptr) noexcept;

    QPropertyBindingPrivatePtr &operator=(const QPropertyBindingPrivatePtr &o) noexcept
    {
        reset(o.d);
        return *this;
    }
    QPropertyBindingPrivatePtr &operator=(T *o) noexcept
    {
        reset(o);
        return *this;
    }
    QPropertyBindingPrivatePtr(QPropertyBindingPrivatePtr &&o) noexcept : d(std::exchange(o.d, nullptr)) {}
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(QPropertyBindingPrivatePtr)

    operator bool () const noexcept { return d != nullptr; }
    bool operator!() const noexcept { return d == nullptr; }

    void swap(QPropertyBindingPrivatePtr &other) noexcept
    { qt_ptr_swap(d, other.d); }

private:
    friend bool comparesEqual(const QPropertyBindingPrivatePtr &lhs,
                              const QPropertyBindingPrivatePtr &rhs) noexcept
    { return lhs.d == rhs.d; }
    Q_DECLARE_EQUALITY_COMPARABLE(QPropertyBindingPrivatePtr)
    friend bool comparesEqual(const QPropertyBindingPrivatePtr &lhs,
                              const T *rhs) noexcept
    { return lhs.d == rhs; }
    Q_DECLARE_EQUALITY_COMPARABLE(QPropertyBindingPrivatePtr, T*)
    friend bool comparesEqual(const QPropertyBindingPrivatePtr &lhs,
                              std::nullptr_t) noexcept
    { return !lhs; }
    Q_DECLARE_EQUALITY_COMPARABLE(QPropertyBindingPrivatePtr, std::nullptr_t)

    QtPrivate::RefCounted *d;
};

class QUntypedPropertyBinding;
class QPropertyBindingPrivate;
struct QPropertyBindingDataPointer;
class QPropertyObserver;
struct QPropertyObserverPointer;

class QUntypedPropertyData
{
};

namespace QtPrivate {
template <typename T>
using IsUntypedPropertyData = std::enable_if_t<std::is_base_of_v<QUntypedPropertyData, T>, bool>;
}

template <typename T>
class QPropertyData;

// Used for grouped property evaluations
namespace QtPrivate {
class QPropertyBindingData;
}
struct QPropertyDelayedNotifications;
struct QPropertyProxyBindingData
{
    // acts as QPropertyBindingData::d_ptr
    quintptr d_ptr;
    /*
        The two members below store the original binding data and property
        data pointer of the property which gets proxied.
        They are set in QPropertyDelayedNotifications::addProperty
    */
    const QtPrivate::QPropertyBindingData *originalBindingData;
    QUntypedPropertyData *propertyData;
};

namespace QtPrivate {
struct BindingEvaluationState;

/*  used in BindingFunctionVTable::createFor; on all other compilers, void would work, but on
    MSVC this causes C2182 when compiling in C++20 mode. As we only need to provide some default
    value which gets ignored, we introduce this dummy type.
*/
struct MSVCWorkAround {};

struct BindingFunctionVTable
{
    using CallFn = bool(*)(QMetaType, QUntypedPropertyData *, void *);
    using DtorFn = void(*)(void *);
    using MoveCtrFn = void(*)(void *, void *);
    const CallFn call;
    const DtorFn destroy;
    const MoveCtrFn moveConstruct;
    const qsizetype size;

    template<typename Callable, typename PropertyType=MSVCWorkAround>
    static constexpr BindingFunctionVTable createFor()
    {
        static_assert (alignof(Callable) <= alignof(std::max_align_t), "Bindings do not support overaligned functors!");
        return {
            /*call=*/[](QMetaType metaType, QUntypedPropertyData *dataPtr, void *f){
                if constexpr (!std::is_invocable_v<Callable>) {
                    // we got an untyped callable
                    static_assert (std::is_invocable_r_v<bool, Callable, QMetaType, QUntypedPropertyData *> );
                    auto untypedEvaluationFunction = static_cast<Callable *>(f);
                    return std::invoke(*untypedEvaluationFunction, metaType, dataPtr);
                } else if constexpr (!std::is_same_v<PropertyType, MSVCWorkAround>) {
                    Q_UNUSED(metaType);
                    QPropertyData<PropertyType> *propertyPtr = static_cast<QPropertyData<PropertyType> *>(dataPtr);
                    // That is allowed by POSIX even if Callable is a function pointer
                    auto evaluationFunction = static_cast<Callable *>(f);
                    PropertyType newValue = std::invoke(*evaluationFunction);
                    if constexpr (QTypeTraits::has_operator_equal_v<PropertyType>) {
                        if (newValue == propertyPtr->valueBypassingBindings())
                            return false;
                    }
                    propertyPtr->setValueBypassingBindings(std::move(newValue));
                    return true;
                } else {
                    // Our code will never instantiate this
                    Q_UNREACHABLE_RETURN(false);
                }
            },
            /*destroy*/[](void *f){ static_cast<Callable *>(f)->~Callable(); },
            /*moveConstruct*/[](void *addr, void *other){
                new (addr) Callable(std::move(*static_cast<Callable *>(other)));
            },
            /*size*/sizeof(Callable)
        };
    }
};

template<typename Callable, typename PropertyType=MSVCWorkAround>
inline constexpr BindingFunctionVTable bindingFunctionVTable = BindingFunctionVTable::createFor<Callable, PropertyType>();


// writes binding result into dataPtr
struct QPropertyBindingFunction {
    const QtPrivate::BindingFunctionVTable *vtable;
    void *functor;
};

using QPropertyObserverCallback = void (*)(QUntypedPropertyData *);
using QPropertyBindingWrapper = bool(*)(QMetaType, QUntypedPropertyData *dataPtr, QPropertyBindingFunction);

/*!
    \internal
    A property normally consists of the actual property value and metadata for the binding system.
    QPropertyBindingData is the latter part. It stores a pointer to either
    - a (potentially empty) linked list of notifiers, in case there is no binding set,
    - an actual QUntypedPropertyBinding when the property has a binding,
    - or a pointer to QPropertyProxyBindingData when notifications occur inside a grouped update.

    \sa QPropertyDelayedNotifications, beginPropertyUpdateGroup
 */
class Q_CORE_EXPORT QPropertyBindingData
{
    // Mutable because the address of the observer of the currently evaluating binding is stored here, for
    // notification later when the value changes.
    mutable quintptr d_ptr = 0;
    friend struct QT_PREPEND_NAMESPACE(QPropertyBindingDataPointer);
    friend class QT_PREPEND_NAMESPACE(QQmlPropertyBinding);
    friend struct QT_PREPEND_NAMESPACE(QPropertyDelayedNotifications);

    template<typename Class, typename T, auto Offset, auto Setter, auto Signal, auto Getter>
    friend class QT_PREPEND_NAMESPACE(QObjectCompatProperty);

    Q_DISABLE_COPY(QPropertyBindingData)
public:
    QPropertyBindingData() = default;
    QPropertyBindingData(QPropertyBindingData &&other);
    QPropertyBindingData &operator=(QPropertyBindingData &&other) = delete;
    ~QPropertyBindingData();

    // Is d_ptr pointing to a binding (1) or list of notifiers (0)?
    static inline constexpr quintptr BindingBit = 0x1;
    // Is d_ptr pointing to QPropertyProxyBindingData (1) or to an actual binding/list of notifiers?
    static inline constexpr quintptr DelayedNotificationBit = 0x2;

    bool hasBinding() const { return d_ptr & BindingBit; }
    bool isNotificationDelayed() const { return d_ptr & DelayedNotificationBit; }

    QUntypedPropertyBinding setBinding(const QUntypedPropertyBinding &newBinding,
                                       QUntypedPropertyData *propertyDataPtr,
                                       QPropertyObserverCallback staticObserverCallback = nullptr,
                                       QPropertyBindingWrapper bindingWrapper = nullptr);

    QPropertyBindingPrivate *binding() const
    {
        quintptr dd = d();
        if (dd & BindingBit)
            return reinterpret_cast<QPropertyBindingPrivate*>(dd - BindingBit);
        return nullptr;

    }

    void evaluateIfDirty(const QUntypedPropertyData *) const; // ### Kept for BC reasons, unused

    void removeBinding()
    {
        if (hasBinding())
            removeBinding_helper();
    }

    void registerWithCurrentlyEvaluatingBinding(QtPrivate::BindingEvaluationState *currentBinding) const
    {
        if (!currentBinding)
            return;
        registerWithCurrentlyEvaluatingBinding_helper(currentBinding);
    }
    void registerWithCurrentlyEvaluatingBinding() const;
    void notifyObservers(QUntypedPropertyData *propertyDataPtr) const;
    void notifyObservers(QUntypedPropertyData *propertyDataPtr, QBindingStorage *storage) const;
private:
    /*!
        \internal
        Returns a reference to d_ptr, except when d_ptr points to a proxy.
        In that case, a reference to proxy->d_ptr is returned instead.

        To properly support proxying, direct access to d_ptr only occurs when
        - a function actually deals with proxying (e.g.
          QPropertyDelayedNotifications::addProperty),
        - only the tag value is accessed (e.g. hasBinding) or
        - inside a constructor.
     */
    quintptr &d_ref() const
    {
        quintptr &d = d_ptr;
        if (isNotificationDelayed())
            return proxyData()->d_ptr;
        return d;
    }
    quintptr d() const { return d_ref(); }
    QPropertyProxyBindingData *proxyData() const
    {
        Q_ASSERT(isNotificationDelayed());
        return reinterpret_cast<QPropertyProxyBindingData *>(d_ptr & ~(BindingBit|DelayedNotificationBit));
    }
    void registerWithCurrentlyEvaluatingBinding_helper(BindingEvaluationState *currentBinding) const;
    void removeBinding_helper();

    enum NotificationResult { Delayed, Evaluated };
    NotificationResult notifyObserver_helper(
            QUntypedPropertyData *propertyDataPtr, QBindingStorage *storage,
            QPropertyObserverPointer observer,
            PendingBindingObserverList &bindingObservers) const;
};

template <typename T, typename Tag>
class QTagPreservingPointerToPointer
{
public:
    constexpr QTagPreservingPointerToPointer() = default;

    QTagPreservingPointerToPointer(T **ptr)
        : d(reinterpret_cast<quintptr*>(ptr))
    {}

    QTagPreservingPointerToPointer<T, Tag> &operator=(T **ptr)
    {
        d = reinterpret_cast<quintptr *>(ptr);
        return *this;
    }

    QTagPreservingPointerToPointer<T, Tag> &operator=(QTaggedPointer<T, Tag> *ptr)
    {
        d = reinterpret_cast<quintptr *>(ptr);
        return *this;
    }

    void clear()
    {
        d = nullptr;
    }

    void setPointer(T *ptr)
    {
        *d = reinterpret_cast<quintptr>(ptr) | (*d & QTaggedPointer<T, Tag>::tagMask());
    }

    T *get() const
    {
        return reinterpret_cast<T*>(*d & QTaggedPointer<T, Tag>::pointerMask());
    }

    explicit operator bool() const
    {
        return d != nullptr;
    }

private:
    quintptr *d = nullptr;
};

namespace detail {
    template <typename F>
    struct ExtractClassFromFunctionPointer;

    template<typename T, typename C>
    struct ExtractClassFromFunctionPointer<T C::*> { using Class = C; };

    constexpr size_t getOffset(size_t o)
    {
        return o;
    }
    constexpr size_t getOffset(size_t (*offsetFn)())
    {
        return offsetFn();
    }
}

} // namespace QtPrivate

QT_END_NAMESPACE

#endif // QPROPERTYPRIVATE_H
