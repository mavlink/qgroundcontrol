// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPROPERTY_H
#define QPROPERTY_H

#include <QtCore/qglobal.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qstring.h>
#include <QtCore/qttypetraits.h>
#include <QtCore/qbindingstorage.h>

#include <type_traits>

#include <QtCore/qpropertyprivate.h>

#if __has_include(<source_location>) && __cplusplus >= 202002L && !defined(Q_QDOC)
#include <source_location>
#if defined(__cpp_lib_source_location)
#define QT_SOURCE_LOCATION_NAMESPACE std
#define QT_PROPERTY_COLLECT_BINDING_LOCATION
#if defined(Q_CC_MSVC)
/* MSVC runs into an issue with constexpr with source location (error C7595)
   so use the factory function as a workaround */
#  define QT_PROPERTY_DEFAULT_BINDING_LOCATION QPropertyBindingSourceLocation::fromStdSourceLocation(std::source_location::current())
#else
/* some versions of gcc in turn run into
   expression ‘std::source_location::current()’ is not a constant expression
   so don't use the workaround there */
#  define QT_PROPERTY_DEFAULT_BINDING_LOCATION QPropertyBindingSourceLocation(std::source_location::current())
#endif
#endif
#endif

#if __has_include(<experimental/source_location>) && !defined(Q_QDOC)
#include <experimental/source_location>
#if !defined(QT_PROPERTY_COLLECT_BINDING_LOCATION)
#if defined(__cpp_lib_experimental_source_location)
#define QT_SOURCE_LOCATION_NAMESPACE std::experimental
#define QT_PROPERTY_COLLECT_BINDING_LOCATION
#define QT_PROPERTY_DEFAULT_BINDING_LOCATION QPropertyBindingSourceLocation(std::experimental::source_location::current())
#endif // defined(__cpp_lib_experimental_source_location)
#endif
#endif

#if !defined(QT_PROPERTY_COLLECT_BINDING_LOCATION)
#define QT_PROPERTY_DEFAULT_BINDING_LOCATION QPropertyBindingSourceLocation()
#endif

QT_BEGIN_NAMESPACE

namespace Qt {
Q_CORE_EXPORT void beginPropertyUpdateGroup();
Q_CORE_EXPORT void endPropertyUpdateGroup();
}

class QScopedPropertyUpdateGroup
{
    Q_DISABLE_COPY_MOVE(QScopedPropertyUpdateGroup)
public:
    Q_NODISCARD_CTOR
    QScopedPropertyUpdateGroup()
    { Qt::beginPropertyUpdateGroup(); }
    ~QScopedPropertyUpdateGroup() noexcept(false)
    { Qt::endPropertyUpdateGroup(); }
};

template <typename T>
class QPropertyData : public QUntypedPropertyData
{
protected:
    mutable T val = T();
private:
    class DisableRValueRefs {};
protected:
    static constexpr bool UseReferences = !(std::is_arithmetic_v<T> || std::is_enum_v<T> || std::is_pointer_v<T>);
public:
    using value_type = T;
    using parameter_type = std::conditional_t<UseReferences, const T &, T>;
    using rvalue_ref = typename std::conditional_t<UseReferences, T &&, DisableRValueRefs>;
    using arrow_operator_result = std::conditional_t<std::is_pointer_v<T>, const T &,
                                        std::conditional_t<QTypeTraits::is_dereferenceable_v<T>, const T &, void>>;

    QPropertyData() = default;
    QPropertyData(parameter_type t) : val(t) {}
    QPropertyData(rvalue_ref t) : val(std::move(t)) {}
    ~QPropertyData() = default;

    parameter_type valueBypassingBindings() const { return val; }
    void setValueBypassingBindings(parameter_type v) { val = v; }
    void setValueBypassingBindings(rvalue_ref v) { val = std::move(v); }
};

// ### Qt 7: un-export
struct Q_CORE_EXPORT QPropertyBindingSourceLocation
{
    const char *fileName = nullptr;
    const char *functionName = nullptr;
    quint32 line = 0;
    quint32 column = 0;
    QPropertyBindingSourceLocation() = default;
#ifdef __cpp_lib_source_location
    constexpr QPropertyBindingSourceLocation(const std::source_location &cppLocation)
    {
        fileName = cppLocation.file_name();
        functionName = cppLocation.function_name();
        line = cppLocation.line();
        column = cppLocation.column();
    }
    QT_POST_CXX17_API_IN_EXPORTED_CLASS
    static consteval QPropertyBindingSourceLocation
    fromStdSourceLocation(const std::source_location &cppLocation)
    {
        return cppLocation;
    }
#endif
#ifdef __cpp_lib_experimental_source_location
    constexpr QPropertyBindingSourceLocation(const std::experimental::source_location &cppLocation)
    {
        fileName = cppLocation.file_name();
        functionName = cppLocation.function_name();
        line = cppLocation.line();
        column = cppLocation.column();
    }
#endif
};

template <typename Functor> class QPropertyChangeHandler;
class QPropertyBindingErrorPrivate;

class Q_CORE_EXPORT QPropertyBindingError
{
public:
    enum Type {
        NoError,
        BindingLoop,
        EvaluationError,
        UnknownError
    };

    QPropertyBindingError();
    QPropertyBindingError(Type type, const QString &description = QString());

    QPropertyBindingError(const QPropertyBindingError &other);
    QPropertyBindingError &operator=(const QPropertyBindingError &other);
    QPropertyBindingError(QPropertyBindingError &&other);
    QPropertyBindingError &operator=(QPropertyBindingError &&other);
    ~QPropertyBindingError();

    bool hasError() const { return d.get() != nullptr; }
    Type type() const;
    QString description() const;

private:
    QSharedDataPointer<QPropertyBindingErrorPrivate> d;
};

class Q_CORE_EXPORT QUntypedPropertyBinding
{
public:
    // writes binding result into dataPtr
    using BindingFunctionVTable = QtPrivate::BindingFunctionVTable;

    QUntypedPropertyBinding();
    QUntypedPropertyBinding(QMetaType metaType, const BindingFunctionVTable *vtable, void *function, const QPropertyBindingSourceLocation &location);

    template<typename Functor>
    QUntypedPropertyBinding(QMetaType metaType, Functor &&f, const QPropertyBindingSourceLocation &location)
        : QUntypedPropertyBinding(metaType, &QtPrivate::bindingFunctionVTable<std::remove_reference_t<Functor>>, &f, location)
    {}

    QUntypedPropertyBinding(QUntypedPropertyBinding &&other);
    QUntypedPropertyBinding(const QUntypedPropertyBinding &other);
    QUntypedPropertyBinding &operator=(const QUntypedPropertyBinding &other);
    QUntypedPropertyBinding &operator=(QUntypedPropertyBinding &&other);
    ~QUntypedPropertyBinding();

    bool isNull() const;

    QPropertyBindingError error() const;

    QMetaType valueMetaType() const;

    explicit QUntypedPropertyBinding(QPropertyBindingPrivate *priv);
private:
    friend class QtPrivate::QPropertyBindingData;
    friend class QPropertyBindingPrivate;
    template <typename> friend class QPropertyBinding;
    QPropertyBindingPrivatePtr d;
};

template <typename PropertyType>
class QPropertyBinding : public QUntypedPropertyBinding
{

public:
    QPropertyBinding() = default;

    template<typename Functor>
    QPropertyBinding(Functor &&f, const QPropertyBindingSourceLocation &location)
        : QUntypedPropertyBinding(QMetaType::fromType<PropertyType>(), &QtPrivate::bindingFunctionVTable<std::remove_reference_t<Functor>, PropertyType>, &f, location)
    {}


    // Internal
    explicit QPropertyBinding(const QUntypedPropertyBinding &binding)
        : QUntypedPropertyBinding(binding)
    {}
};

namespace Qt {
    template <typename Functor>
    auto makePropertyBinding(Functor &&f, const QPropertyBindingSourceLocation &location = QT_PROPERTY_DEFAULT_BINDING_LOCATION,
                             std::enable_if_t<std::is_invocable_v<Functor>> * = nullptr)
    {
        return QPropertyBinding<std::invoke_result_t<Functor>>(std::forward<Functor>(f), location);
    }
}

struct QPropertyObserverPrivate;
struct QPropertyObserverPointer;
class QPropertyObserver;

class QPropertyObserverBase
{
public:
    // Internal
    enum ObserverTag {
        ObserverNotifiesBinding, // observer was installed to notify bindings that obsverved property changed
        ObserverNotifiesChangeHandler, // observer is a change handler, which runs on every change
        ObserverIsPlaceholder,  // the observer before this one is currently evaluated in QPropertyObserver::notifyObservers.
#if QT_DEPRECATED_SINCE(6, 6)
        ObserverIsAlias QT_DEPRECATED_VERSION_X_6_6("Use QProperty and add a binding to the target.")
#endif
    };
protected:
    using ChangeHandler = void (*)(QPropertyObserver*, QUntypedPropertyData *);

private:
    friend struct QPropertyDelayedNotifications;
    friend struct QPropertyObserverNodeProtector;
    friend class QPropertyObserver;
    friend struct QPropertyObserverPointer;
    friend struct QPropertyBindingDataPointer;
    friend class QPropertyBindingPrivate;

    QTaggedPointer<QPropertyObserver, ObserverTag> next;
    // prev is a pointer to the "next" element within the previous node, or to the "firstObserverPtr" if it is the
    // first node.
    QtPrivate::QTagPreservingPointerToPointer<QPropertyObserver, ObserverTag> prev;

    union {
        QPropertyBindingPrivate *binding = nullptr;
        ChangeHandler changeHandler;
        QUntypedPropertyData *aliasData;
    };
};

class Q_CORE_EXPORT QPropertyObserver : public QPropertyObserverBase
{
public:
    constexpr QPropertyObserver() = default;
    QPropertyObserver(QPropertyObserver &&other) noexcept;
    QPropertyObserver &operator=(QPropertyObserver &&other) noexcept;
    ~QPropertyObserver();

    template <typename Property, QtPrivate::IsUntypedPropertyData<Property> = true>
    void setSource(const Property &property)
    { setSource(property.bindingData()); }
    void setSource(const QtPrivate::QPropertyBindingData &property);

protected:
    QPropertyObserver(ChangeHandler changeHandler);
#if QT_DEPRECATED_SINCE(6, 6)
    QT_DEPRECATED_VERSION_X_6_6("This constructor was only meant for internal use. Use QProperty and add a binding to the target.")
    QPropertyObserver(QUntypedPropertyData *aliasedPropertyPtr);
#endif

    QUntypedPropertyData *aliasedProperty() const
    {
        return aliasData;
    }

private:

    QPropertyObserver(const QPropertyObserver &) = delete;
    QPropertyObserver &operator=(const QPropertyObserver &) = delete;

};

template <typename Functor>
class QPropertyChangeHandler : public QPropertyObserver
{
    Functor m_handler;
public:
    Q_NODISCARD_CTOR
    QPropertyChangeHandler(Functor handler)
        : QPropertyObserver([](QPropertyObserver *self, QUntypedPropertyData *) {
              auto This = static_cast<QPropertyChangeHandler<Functor>*>(self);
              This->m_handler();
          })
        , m_handler(std::move(handler))
    {
    }

    template <typename Property, QtPrivate::IsUntypedPropertyData<Property> = true>
    Q_NODISCARD_CTOR
    QPropertyChangeHandler(const Property &property, Functor handler)
        : QPropertyObserver([](QPropertyObserver *self, QUntypedPropertyData *) {
              auto This = static_cast<QPropertyChangeHandler<Functor>*>(self);
              This->m_handler();
          })
        , m_handler(std::move(handler))
    {
        setSource(property);
    }
};

class QPropertyNotifier : public QPropertyObserver
{
    std::function<void()> m_handler;
public:
    Q_NODISCARD_CTOR
    QPropertyNotifier() = default;
    template<typename Functor>
    Q_NODISCARD_CTOR
    QPropertyNotifier(Functor handler)
        : QPropertyObserver([](QPropertyObserver *self, QUntypedPropertyData *) {
            auto This = static_cast<QPropertyNotifier *>(self);
            This->m_handler();
        })
        , m_handler(std::move(handler))
    {
    }

    template <typename Functor, typename Property,
              QtPrivate::IsUntypedPropertyData<Property> = true>
    Q_NODISCARD_CTOR
    QPropertyNotifier(const Property &property, Functor handler)
        : QPropertyObserver([](QPropertyObserver *self, QUntypedPropertyData *) {
            auto This = static_cast<QPropertyNotifier *>(self);
            This->m_handler();
        })
        , m_handler(std::move(handler))
    {
        setSource(property);
    }
};

template <typename T>
class QProperty : public QPropertyData<T>
{
    QtPrivate::QPropertyBindingData d;
    bool is_equal(const T &v)
    {
        if constexpr (QTypeTraits::has_operator_equal_v<T>) {
            if (v == this->val)
                return true;
        }
        return false;
    }

    template <typename U, typename = void>
    struct has_operator_equal_to : std::false_type{};

    template <typename U>
    struct has_operator_equal_to<U, std::void_t<decltype(bool(std::declval<const T&>() == std::declval<const U&>()))>>
        : std::true_type{};

    template <typename U>
    static constexpr bool has_operator_equal_to_v =
            !std::is_same_v<U, T> && has_operator_equal_to<U>::value;

public:
    using value_type = typename QPropertyData<T>::value_type;
    using parameter_type = typename QPropertyData<T>::parameter_type;
    using rvalue_ref = typename QPropertyData<T>::rvalue_ref;
    using arrow_operator_result = typename QPropertyData<T>::arrow_operator_result;

    QProperty() = default;
    explicit QProperty(parameter_type initialValue) : QPropertyData<T>(initialValue) {}
    explicit QProperty(rvalue_ref initialValue) : QPropertyData<T>(std::move(initialValue)) {}
    explicit QProperty(const QPropertyBinding<T> &binding)
        : QProperty()
    { setBinding(binding); }
#ifndef Q_QDOC
    template <typename Functor>
    explicit QProperty(Functor &&f, const QPropertyBindingSourceLocation &location = QT_PROPERTY_DEFAULT_BINDING_LOCATION,
                       typename std::enable_if_t<std::is_invocable_r_v<T, Functor&>> * = nullptr)
        : QProperty(QPropertyBinding<T>(std::forward<Functor>(f), location))
    {}
#else
    template <typename Functor>
    explicit QProperty(Functor &&f);
#endif
    ~QProperty() = default;

    QT_DECLARE_EQUALITY_OPERATORS_HELPER(QProperty, QProperty, /* non-constexpr */, noexcept(false), template <typename Ty = T, std::enable_if_t<QTypeTraits::has_operator_equal_v<Ty>>* = nullptr>)
    QT_DECLARE_EQUALITY_OPERATORS_HELPER(QProperty, T, /* non-constexpr */, noexcept(false), template <typename Ty = T, std::enable_if_t<QTypeTraits::has_operator_equal_v<Ty>>* = nullptr>)
    QT_DECLARE_EQUALITY_OPERATORS_REVERSED_HELPER(QProperty, T, /* non-constexpr */, noexcept(false), template <typename Ty = T, std::enable_if_t<QTypeTraits::has_operator_equal_v<Ty>>* = nullptr>)

    QT_DECLARE_EQUALITY_OPERATORS_HELPER(QProperty, U, /* non-constexpr */, noexcept(false), template <typename U, std::enable_if_t<has_operator_equal_to_v<U>>* = nullptr>)
    QT_DECLARE_EQUALITY_OPERATORS_REVERSED_HELPER(QProperty, U, /* non-constexpr */, noexcept(false), template <typename U, std::enable_if_t<has_operator_equal_to_v<U>>* = nullptr>)

    // Explicitly delete op==(QProperty<T>, QProperty<U>) for different T & U.
    // We do not want implicit conversions here!
    // However, GCC complains about using a default template argument in a
    // friend declaration, while Clang and MSVC are fine. So, skip GCC here.
#if !defined(Q_CC_GNU) || defined(Q_CC_CLANG)
#define QPROPERTY_DECL_DELETED_EQ_OP \
    Q_DECL_EQ_DELETE_X("Call .value() on one of the properties explicitly.")
    template <typename U, std::enable_if_t<!std::is_same_v<T, U>>* = nullptr>
    friend void operator==(const QProperty &, const QProperty<U> &) QPROPERTY_DECL_DELETED_EQ_OP;
    template <typename U, std::enable_if_t<!std::is_same_v<T, U>>* = nullptr>
    friend void operator!=(const QProperty &, const QProperty<U> &) QPROPERTY_DECL_DELETED_EQ_OP;
#undef QPROPERTY_DECL_DELETED_EQ_OP
#endif // !defined(Q_CC_GNU) || defined(Q_CC_CLANG)

    parameter_type value() const
    {
        d.registerWithCurrentlyEvaluatingBinding();
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

    void setValue(rvalue_ref newValue)
    {
        d.removeBinding();
        if (is_equal(newValue))
            return;
        this->val = std::move(newValue);
        notify();
    }

    void setValue(parameter_type newValue)
    {
        d.removeBinding();
        if (is_equal(newValue))
            return;
        this->val = newValue;
        notify();
    }

    QProperty<T> &operator=(rvalue_ref newValue)
    {
        setValue(std::move(newValue));
        return *this;
    }

    QProperty<T> &operator=(parameter_type newValue)
    {
        setValue(newValue);
        return *this;
    }

    QPropertyBinding<T> setBinding(const QPropertyBinding<T> &newBinding)
    {
        return QPropertyBinding<T>(d.setBinding(newBinding, this));
    }

    bool setBinding(const QUntypedPropertyBinding &newBinding)
    {
        if (!newBinding.isNull() && newBinding.valueMetaType().id() != qMetaTypeId<T>())
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

    bool hasBinding() const { return d.hasBinding(); }

    QPropertyBinding<T> binding() const
    {
        return QPropertyBinding<T>(QUntypedPropertyBinding(d.binding()));
    }

    QPropertyBinding<T> takeBinding()
    {
        return QPropertyBinding<T>(d.setBinding(QUntypedPropertyBinding(), this));
    }

    template<typename Functor>
    QPropertyChangeHandler<Functor> onValueChanged(Functor f)
    {
        static_assert(std::is_invocable_v<Functor>, "Functor callback must be callable without any parameters");
        return QPropertyChangeHandler<Functor>(*this, std::move(f));
    }

    template<typename Functor>
    QPropertyChangeHandler<Functor> subscribe(Functor f)
    {
        static_assert(std::is_invocable_v<Functor>, "Functor callback must be callable without any parameters");
        f();
        return onValueChanged(std::move(f));
    }

    template<typename Functor>
    QPropertyNotifier addNotifier(Functor f)
    {
        static_assert(std::is_invocable_v<Functor>, "Functor callback must be callable without any parameters");
        return QPropertyNotifier(*this, std::move(f));
    }

    const QtPrivate::QPropertyBindingData &bindingData() const { return d; }
private:
    template <typename Ty = T, std::enable_if_t<QTypeTraits::has_operator_equal_v<Ty>>* = nullptr>
    friend bool comparesEqual(const QProperty &lhs, const QProperty &rhs)
    {
        return lhs.value() == rhs.value();
    }

    template <typename Ty = T, std::enable_if_t<QTypeTraits::has_operator_equal_v<Ty>>* = nullptr>
    friend bool comparesEqual(const QProperty &lhs, const T &rhs)
    {
        return lhs.value() == rhs;
    }

    template <typename U, std::enable_if_t<has_operator_equal_to_v<U>>* = nullptr>
    friend bool comparesEqual(const QProperty &lhs, const U &rhs)
    {
        return lhs.value() == rhs;
    }

    void notify()
    {
        d.notifyObservers(this);
    }

    Q_DISABLE_COPY_MOVE(QProperty)
};

namespace Qt {
    template <typename PropertyType>
    QPropertyBinding<PropertyType> makePropertyBinding(const QProperty<PropertyType> &otherProperty,
                                                       const QPropertyBindingSourceLocation &location =
                                                       QT_PROPERTY_DEFAULT_BINDING_LOCATION)
    {
        return Qt::makePropertyBinding([&otherProperty]() -> PropertyType { return otherProperty; }, location);
    }
}


namespace QtPrivate
{

struct QBindableInterface
{
    using Getter = void (*)(const QUntypedPropertyData *d, void *value);
    using Setter = void (*)(QUntypedPropertyData *d, const void *value);
    using BindingGetter = QUntypedPropertyBinding (*)(const QUntypedPropertyData *d);
    using BindingSetter = QUntypedPropertyBinding (*)(QUntypedPropertyData *d, const QUntypedPropertyBinding &binding);
    using MakeBinding = QUntypedPropertyBinding (*)(const QUntypedPropertyData *d, const QPropertyBindingSourceLocation &location);
    using SetObserver = void (*)(const QUntypedPropertyData *d, QPropertyObserver *observer);
    using GetMetaType = QMetaType (*)();
    Getter getter;
    Setter setter;
    BindingGetter getBinding;
    BindingSetter setBinding;
    MakeBinding makeBinding;
    SetObserver setObserver;
    GetMetaType metaType;

    static constexpr quintptr MetaTypeAccessorFlag = 0x1;
};

template<typename Property, typename = void>
class QBindableInterfaceForProperty
{
    using T = typename Property::value_type;
public:
    // interface for computed properties. Those do not have a binding()/setBinding() method, but one can
    // install observers on them.
    static constexpr QBindableInterface iface = {
        [](const QUntypedPropertyData *d, void *value) -> void
        { *static_cast<T*>(value) = static_cast<const Property *>(d)->value(); },
        nullptr,
        nullptr,
        nullptr,
        [](const QUntypedPropertyData *d, const QPropertyBindingSourceLocation &location) -> QUntypedPropertyBinding
        { return Qt::makePropertyBinding([d]() -> T { return static_cast<const Property *>(d)->value(); }, location); },
        [](const QUntypedPropertyData *d, QPropertyObserver *observer) -> void
        { observer->setSource(static_cast<const Property *>(d)->bindingData()); },
        []() { return QMetaType::fromType<T>(); }
    };
};

template<typename Property>
class QBindableInterfaceForProperty<const Property, std::void_t<decltype(std::declval<Property>().binding())>>
{
    using T = typename Property::value_type;
public:
    // A bindable created from a const property results in a read-only interface, too.
    static constexpr QBindableInterface iface = {

        [](const QUntypedPropertyData *d, void *value) -> void
        { *static_cast<T*>(value) = static_cast<const Property *>(d)->value(); },
        /*setter=*/nullptr,
        [](const QUntypedPropertyData *d) -> QUntypedPropertyBinding
        { return static_cast<const Property *>(d)->binding(); },
        /*setBinding=*/nullptr,
        [](const QUntypedPropertyData *d, const QPropertyBindingSourceLocation &location) -> QUntypedPropertyBinding
        { return Qt::makePropertyBinding([d]() -> T { return static_cast<const Property *>(d)->value(); }, location); },
        [](const QUntypedPropertyData *d, QPropertyObserver *observer) -> void
        { observer->setSource(static_cast<const Property *>(d)->bindingData()); },
        []() { return QMetaType::fromType<T>(); }
    };
};

template<typename Property>
class QBindableInterfaceForProperty<Property, std::void_t<decltype(std::declval<Property>().binding())>>
{
    using T = typename Property::value_type;
public:
    static constexpr QBindableInterface iface = {
        [](const QUntypedPropertyData *d, void *value) -> void
        { *static_cast<T*>(value) = static_cast<const Property *>(d)->value(); },
        [](QUntypedPropertyData *d, const void *value) -> void
        { static_cast<Property *>(d)->setValue(*static_cast<const T*>(value)); },
        [](const QUntypedPropertyData *d) -> QUntypedPropertyBinding
        { return static_cast<const Property *>(d)->binding(); },
        [](QUntypedPropertyData *d, const QUntypedPropertyBinding &binding) -> QUntypedPropertyBinding
        { return static_cast<Property *>(d)->setBinding(static_cast<const QPropertyBinding<T> &>(binding)); },
        [](const QUntypedPropertyData *d, const QPropertyBindingSourceLocation &location) -> QUntypedPropertyBinding
        { return Qt::makePropertyBinding([d]() -> T { return static_cast<const Property *>(d)->value(); }, location); },
        [](const QUntypedPropertyData *d, QPropertyObserver *observer) -> void
        { observer->setSource(static_cast<const Property *>(d)->bindingData()); },
        []() { return QMetaType::fromType<T>(); }
    };
};

}

namespace QtPrivate {
// used in Q(Untyped)Bindable to print warnings about various binding errors
namespace BindableWarnings {
enum Reason { InvalidInterface, NonBindableInterface, ReadOnlyInterface };
Q_CORE_EXPORT void printUnsuitableBindableWarning(QAnyStringView prefix, Reason reason);
Q_CORE_EXPORT void printMetaTypeMismatch(QMetaType actual, QMetaType expected);
}

namespace PropertyAdaptorSlotObjectHelpers {
Q_CORE_EXPORT void getter(const QUntypedPropertyData *d, void *value);
Q_CORE_EXPORT void setter(QUntypedPropertyData *d, const void *value);
Q_CORE_EXPORT QUntypedPropertyBinding getBinding(const QUntypedPropertyData *d);
Q_CORE_EXPORT bool bindingWrapper(QMetaType type, QUntypedPropertyData *d,
                                  QtPrivate::QPropertyBindingFunction binding,
                                  QUntypedPropertyData *temp, void *value);
Q_CORE_EXPORT QUntypedPropertyBinding setBinding(QUntypedPropertyData *d,
                                                 const QUntypedPropertyBinding &binding,
                                                 QPropertyBindingWrapper wrapper);
Q_CORE_EXPORT void setObserver(const QUntypedPropertyData *d, QPropertyObserver *observer);

template<typename T>
bool bindingWrapper(QMetaType type, QUntypedPropertyData *d,
                    QtPrivate::QPropertyBindingFunction binding)
{
    struct Data : QPropertyData<T>
    {
        void *data() { return &this->val; }
    } temp;
    return bindingWrapper(type, d, binding, &temp, temp.data());
}

template<typename T>
QUntypedPropertyBinding setBinding(QUntypedPropertyData *d, const QUntypedPropertyBinding &binding)
{
    return setBinding(d, binding, &bindingWrapper<T>);
}

template<typename T>
QUntypedPropertyBinding makeBinding(const QUntypedPropertyData *d,
                                    const QPropertyBindingSourceLocation &location)
{
    return Qt::makePropertyBinding(
            [d]() -> T {
                T r;
                getter(d, &r);
                return r;
            },
            location);
}

template<class T>
inline constexpr QBindableInterface iface = {
    &getter,
    &setter,
    &getBinding,
    &setBinding<T>,
    &makeBinding<T>,
    &setObserver,
    &QMetaType::fromType<T>,
};
}
}

class QUntypedBindable
{
    friend struct QUntypedBindablePrivate; // allows access to internal data
protected:
    QUntypedPropertyData *data = nullptr;
    const QtPrivate::QBindableInterface *iface = nullptr;
    constexpr QUntypedBindable(QUntypedPropertyData *d, const QtPrivate::QBindableInterface *i)
        : data(d), iface(i)
    {}

    Q_CORE_EXPORT explicit QUntypedBindable(QObject* obj, const QMetaProperty &property, const QtPrivate::QBindableInterface *i);
    Q_CORE_EXPORT explicit QUntypedBindable(QObject* obj, const char* property, const QtPrivate::QBindableInterface *i);

public:
    constexpr QUntypedBindable() = default;
    template<typename Property>
    QUntypedBindable(Property *p)
        : data(const_cast<std::remove_cv_t<Property> *>(p)),
          iface(&QtPrivate::QBindableInterfaceForProperty<Property>::iface)
    { Q_ASSERT(data && iface); }

    bool isValid() const { return data != nullptr; }
    bool isBindable() const { return iface && iface->getBinding; }
    bool isReadOnly() const { return !(iface && iface->setBinding && iface->setObserver); }

    QUntypedPropertyBinding makeBinding(const QPropertyBindingSourceLocation &location = QT_PROPERTY_DEFAULT_BINDING_LOCATION) const
    {
        return iface ? iface->makeBinding(data, location) : QUntypedPropertyBinding();
    }

    QUntypedPropertyBinding takeBinding()
    {
        if (!iface)
            return QUntypedPropertyBinding {};
        // We do not have a dedicated takeBinding function pointer in the interface
        // therefore we synthesize takeBinding by retrieving the binding with binding
        // and calling setBinding with a default constructed QUntypedPropertyBinding
        // afterwards.
        if (!(iface->getBinding && iface->setBinding))
            return QUntypedPropertyBinding {};
        QUntypedPropertyBinding binding = iface->getBinding(data);
        iface->setBinding(data, QUntypedPropertyBinding{});
        return binding;
    }

    void observe(QPropertyObserver *observer) const
    {
        if (iface)
            iface->setObserver(data, observer);
#ifndef QT_NO_DEBUG
        else
            QtPrivate::BindableWarnings::printUnsuitableBindableWarning("observe:",
                                                                        QtPrivate::BindableWarnings::InvalidInterface);
#endif
    }

    template<typename Functor>
    QPropertyChangeHandler<Functor> onValueChanged(Functor f) const
    {
        QPropertyChangeHandler<Functor> handler(std::move(f));
        observe(&handler);
        return handler;
    }

    template<typename Functor>
    QPropertyChangeHandler<Functor> subscribe(Functor f) const
    {
        f();
        return onValueChanged(std::move(f));
    }

    template<typename Functor>
    QPropertyNotifier addNotifier(Functor f)
    {
        QPropertyNotifier handler(std::move(f));
        observe(&handler);
        return handler;
    }

    QUntypedPropertyBinding binding() const
    {
        if (!isBindable()) {
#ifndef QT_NO_DEBUG
            QtPrivate::BindableWarnings::printUnsuitableBindableWarning("binding: ",
                                                                        QtPrivate::BindableWarnings::NonBindableInterface);
#endif
            return QUntypedPropertyBinding();
        }
        return iface->getBinding(data);
    }
    bool setBinding(const QUntypedPropertyBinding &binding)
    {
        if (isReadOnly()) {
#ifndef QT_NO_DEBUG
            const auto errorType = iface ? QtPrivate::BindableWarnings::ReadOnlyInterface :
                                           QtPrivate::BindableWarnings::InvalidInterface;
            QtPrivate::BindableWarnings::printUnsuitableBindableWarning("setBinding: Could not set binding via bindable interface.", errorType);
#endif
            return false;
        }
        if (!binding.isNull() && binding.valueMetaType() != metaType()) {
#ifndef QT_NO_DEBUG
            QtPrivate::BindableWarnings::printMetaTypeMismatch(metaType(), binding.valueMetaType());
#endif
            return false;
        }
        iface->setBinding(data, binding);
        return true;
    }
    bool hasBinding() const
    {
        return !binding().isNull();
    }

    QMetaType metaType() const
    {
        if (!(iface && data))
            return QMetaType();
        if (iface->metaType)
            return iface->metaType();
        // ### Qt 7: Change the metatype function to take data as its argument
        // special casing for QML's proxy bindable: allow multiplexing in the getter
        // function to retrieve the metatype from data
        Q_ASSERT(iface->getter);
        QMetaType result;
        iface->getter(data, reinterpret_cast<void *>(quintptr(&result) | QtPrivate::QBindableInterface::MetaTypeAccessorFlag));
        return result;
    }

};

template<typename T>
class QBindable : public QUntypedBindable
{
    template<typename U>
    friend class QPropertyAlias;
    constexpr QBindable(QUntypedPropertyData *d, const QtPrivate::QBindableInterface *i)
        : QUntypedBindable(d, i)
    {}
public:
    using QUntypedBindable::QUntypedBindable;
    explicit QBindable(const QUntypedBindable &b) : QUntypedBindable(b)
    {
        if (iface && metaType() != QMetaType::fromType<T>()) {
            data = nullptr;
            iface = nullptr;
        }
    }

    explicit QBindable(QObject *obj, const QMetaProperty &property)
        : QUntypedBindable(obj, property, &QtPrivate::PropertyAdaptorSlotObjectHelpers::iface<T>) {}

    explicit QBindable(QObject *obj, const char *property)
        : QUntypedBindable(obj, property, &QtPrivate::PropertyAdaptorSlotObjectHelpers::iface<T>) {}

    QPropertyBinding<T> makeBinding(const QPropertyBindingSourceLocation &location = QT_PROPERTY_DEFAULT_BINDING_LOCATION) const
    {
        return static_cast<QPropertyBinding<T> &&>(QUntypedBindable::makeBinding(location));
    }
    QPropertyBinding<T> binding() const
    {
        return static_cast<QPropertyBinding<T> &&>(QUntypedBindable::binding());
    }

    QPropertyBinding<T> takeBinding()
    {
        return static_cast<QPropertyBinding<T> &&>(QUntypedBindable::takeBinding());
    }

    using QUntypedBindable::setBinding;
    QPropertyBinding<T> setBinding(const QPropertyBinding<T> &binding)
    {
        Q_ASSERT(!iface || binding.isNull() || binding.valueMetaType() == metaType());

        if (iface && iface->setBinding)
            return static_cast<QPropertyBinding<T> &&>(iface->setBinding(data, binding));
#ifndef QT_NO_DEBUG
        if (!iface)
            QtPrivate::BindableWarnings::printUnsuitableBindableWarning("setBinding", QtPrivate::BindableWarnings::InvalidInterface);
        else
            QtPrivate::BindableWarnings::printUnsuitableBindableWarning("setBinding: Could not set binding via bindable interface.", QtPrivate::BindableWarnings::ReadOnlyInterface);
#endif
        return QPropertyBinding<T>();
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

    T value() const
    {
        if (iface) {
            T result;
            iface->getter(data, &result);
            return result;
        }
        return T{};
    }

    void setValue(const T &value)
    {
        if (iface && iface->setter)
            iface->setter(data, &value);
    }
};

#if QT_DEPRECATED_SINCE(6, 6)
template<typename T>
class QT_DEPRECATED_VERSION_X_6_6("Class was only meant for internal use, use a QProperty and add a binding to the target")
QPropertyAlias : public QPropertyObserver
{
    Q_DISABLE_COPY_MOVE(QPropertyAlias)
    const QtPrivate::QBindableInterface *iface = nullptr;

public:
    QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED
    QPropertyAlias(QProperty<T> *property)
        : QPropertyObserver(property),
          iface(&QtPrivate::QBindableInterfaceForProperty<QProperty<T>>::iface)
    {
        if (iface)
            iface->setObserver(aliasedProperty(), this);
    }

    template <typename Property, QtPrivate::IsUntypedPropertyData<Property> = true>
    QPropertyAlias(Property *property)
        : QPropertyObserver(property),
          iface(&QtPrivate::QBindableInterfaceForProperty<Property>::iface)
    {
        if (iface)
            iface->setObserver(aliasedProperty(), this);
    }

    QPropertyAlias(QPropertyAlias<T> *alias)
        : QPropertyObserver(alias->aliasedProperty()),
          iface(alias->iface)
    {
        if (iface)
            iface->setObserver(aliasedProperty(), this);
    }

    QPropertyAlias(const QBindable<T> &property)
        : QPropertyObserver(property.data),
          iface(property.iface)
    {
        if (iface)
            iface->setObserver(aliasedProperty(), this);
    }

    T value() const
    {
        T t = T();
        if (auto *p = aliasedProperty())
            iface->getter(p, &t);
        return t;
    }

    operator T() const { return value(); }

    void setValue(const T &newValue)
    {
        if (auto *p = aliasedProperty())
            iface->setter(p, &newValue);
    }

    QPropertyAlias<T> &operator=(const T &newValue)
    {
        setValue(newValue);
        return *this;
    }

    QPropertyBinding<T> setBinding(const QPropertyBinding<T> &newBinding)
    {
        return QBindable<T>(aliasedProperty(), iface).setBinding(newBinding);
    }

    bool setBinding(const QUntypedPropertyBinding &newBinding)
    {
        return QBindable<T>(aliasedProperty(), iface).setBinding(newBinding);
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

    bool hasBinding() const
    {
        return QBindable<T>(aliasedProperty(), iface).hasBinding();
    }

    QPropertyBinding<T> binding() const
    {
        return QBindable<T>(aliasedProperty(), iface).binding();
    }

    QPropertyBinding<T> takeBinding()
    {
        return QBindable<T>(aliasedProperty(), iface).takeBinding();
    }

    template<typename Functor>
    QPropertyChangeHandler<Functor> onValueChanged(Functor f)
    {
        return QBindable<T>(aliasedProperty(), iface).onValueChanged(std::move(f));
    }

    template<typename Functor>
    QPropertyChangeHandler<Functor> subscribe(Functor f)
    {
        return QBindable<T>(aliasedProperty(), iface).subscribe(std::move(f));
    }

    template<typename Functor>
    QPropertyNotifier addNotifier(Functor f)
    {
        return QBindable<T>(aliasedProperty(), iface).addNotifier(std::move(f));
    }

    bool isValid() const
    {
        return aliasedProperty() != nullptr;
    }
    QT_WARNING_POP
};
#endif // QT_DEPRECATED_SINCE(6, 6)

template<typename Class, typename T, auto Offset, auto Signal = nullptr>
class QObjectBindableProperty : public QPropertyData<T>
{
    using ThisType = QObjectBindableProperty<Class, T, Offset, Signal>;
    static bool constexpr HasSignal = !std::is_same_v<decltype(Signal), std::nullptr_t>;
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
    static void signalCallBack(QUntypedPropertyData *o)
    {
        QObjectBindableProperty *that = static_cast<QObjectBindableProperty *>(o);
        if constexpr (HasSignal) {
            if constexpr (SignalTakesValue::value)
                (that->owner()->*Signal)(that->valueBypassingBindings());
            else
                (that->owner()->*Signal)();
        }
    }
public:
    using value_type = typename QPropertyData<T>::value_type;
    using parameter_type = typename QPropertyData<T>::parameter_type;
    using rvalue_ref = typename QPropertyData<T>::rvalue_ref;
    using arrow_operator_result = typename QPropertyData<T>::arrow_operator_result;

    QObjectBindableProperty() = default;
    explicit QObjectBindableProperty(const T &initialValue) : QPropertyData<T>(initialValue) {}
    explicit QObjectBindableProperty(T &&initialValue) : QPropertyData<T>(std::move(initialValue)) {}
    explicit QObjectBindableProperty(const QPropertyBinding<T> &binding)
        : QObjectBindableProperty()
    { setBinding(binding); }
#ifndef Q_QDOC
    template <typename Functor>
    explicit QObjectBindableProperty(Functor &&f, const QPropertyBindingSourceLocation &location = QT_PROPERTY_DEFAULT_BINDING_LOCATION,
                       typename std::enable_if_t<std::is_invocable_r_v<T, Functor&>> * = nullptr)
        : QObjectBindableProperty(QPropertyBinding<T>(std::forward<Functor>(f), location))
    {}
#else
    template <typename Functor>
    explicit QObjectBindableProperty(Functor &&f);
#endif

    QT_DECLARE_EQUALITY_OPERATORS_HELPER(QObjectBindableProperty, QObjectBindableProperty, /* non-constexpr */, noexcept(false), template <typename Ty = T, std::enable_if_t<QTypeTraits::has_operator_equal_v<Ty>>* = nullptr>)
    QT_DECLARE_EQUALITY_OPERATORS_HELPER(QObjectBindableProperty, T, /* non-constexpr */, noexcept(false), template <typename Ty = T, std::enable_if_t<QTypeTraits::has_operator_equal_v<Ty>>* = nullptr>)
    QT_DECLARE_EQUALITY_OPERATORS_REVERSED_HELPER(QObjectBindableProperty, T, /* non-constexpr */, noexcept(false), template <typename Ty = T, std::enable_if_t<QTypeTraits::has_operator_equal_v<Ty>>* = nullptr>)

    parameter_type value() const
    {
        qGetBindingStorage(owner())->registerDependency(this);
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
        auto *bd = qGetBindingStorage(owner())->bindingData(this);
        if (bd)
            bd->removeBinding();
        if (this->val == t)
            return;
        this->val = t;
        notify(bd);
    }

    void notify() {
        auto *bd = qGetBindingStorage(owner())->bindingData(this);
        notify(bd);
    }

    void setValue(rvalue_ref t)
    {
        auto *bd = qGetBindingStorage(owner())->bindingData(this);
        if (bd)
            bd->removeBinding();
        if (this->val == t)
            return;
        this->val = std::move(t);
        notify(bd);
    }

    QObjectBindableProperty &operator=(rvalue_ref newValue)
    {
        setValue(std::move(newValue));
        return *this;
    }

    QObjectBindableProperty &operator=(parameter_type newValue)
    {
        setValue(newValue);
        return *this;
    }

    QPropertyBinding<T> setBinding(const QPropertyBinding<T> &newBinding)
    {
        QtPrivate::QPropertyBindingData *bd = qGetBindingStorage(owner())->bindingData(this, true);
        QUntypedPropertyBinding oldBinding(bd->setBinding(newBinding, this, HasSignal ? &signalCallBack : nullptr));
        return static_cast<QPropertyBinding<T> &>(oldBinding);
    }

    bool setBinding(const QUntypedPropertyBinding &newBinding)
    {
        if (!newBinding.isNull() && newBinding.valueMetaType().id() != qMetaTypeId<T>())
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

    bool hasBinding() const
    {
        auto *bd = qGetBindingStorage(owner())->bindingData(this);
        return bd && bd->binding() != nullptr;
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
        return QPropertyChangeHandler<Functor>(*this, std::move(f));
    }

    template<typename Functor>
    QPropertyChangeHandler<Functor> subscribe(Functor f)
    {
        static_assert(std::is_invocable_v<Functor>, "Functor callback must be callable without any parameters");
        f();
        return onValueChanged(std::move(f));
    }

    template<typename Functor>
    QPropertyNotifier addNotifier(Functor f)
    {
        static_assert(std::is_invocable_v<Functor>, "Functor callback must be callable without any parameters");
        return QPropertyNotifier(*this, std::move(f));
    }

    const QtPrivate::QPropertyBindingData &bindingData() const
    {
        auto *storage = const_cast<QBindingStorage *>(qGetBindingStorage(owner()));
        return *storage->bindingData(const_cast<ThisType *>(this), true);
    }
private:
    template <typename Ty = T, std::enable_if_t<QTypeTraits::has_operator_equal_v<Ty>>* = nullptr>
    friend bool comparesEqual(const QObjectBindableProperty &lhs, const QObjectBindableProperty &rhs)
    {
        return lhs.value() == rhs.value();
    }

    template <typename Ty = T, std::enable_if_t<QTypeTraits::has_operator_equal_v<Ty>>* = nullptr>
    friend bool comparesEqual(const QObjectBindableProperty &lhs, const T &rhs)
    {
        return lhs.value() == rhs;
    }

    void notify(const QtPrivate::QPropertyBindingData *binding)
    {
        if (binding)
            binding->notifyObservers(this, qGetBindingStorage(owner()));
        if constexpr (HasSignal) {
            if constexpr (SignalTakesValue::value)
                (owner()->*Signal)(this->valueBypassingBindings());
            else
                (owner()->*Signal)();
        }
    }
};

#define QT_OBJECT_BINDABLE_PROPERTY_3(Class, Type, name) \
    static constexpr size_t _qt_property_##name##_offset() { \
        QT_WARNING_PUSH QT_WARNING_DISABLE_INVALID_OFFSETOF \
        return offsetof(Class, name); \
        QT_WARNING_POP \
    } \
    QObjectBindableProperty<Class, Type, Class::_qt_property_##name##_offset, nullptr> name;

#define QT_OBJECT_BINDABLE_PROPERTY_4(Class, Type, name, Signal) \
    static constexpr size_t _qt_property_##name##_offset() { \
        QT_WARNING_PUSH QT_WARNING_DISABLE_INVALID_OFFSETOF \
        return offsetof(Class, name); \
        QT_WARNING_POP \
    } \
    QObjectBindableProperty<Class, Type, Class::_qt_property_##name##_offset, Signal> name;

#define Q_OBJECT_BINDABLE_PROPERTY(...) \
    QT_WARNING_PUSH QT_WARNING_DISABLE_INVALID_OFFSETOF \
    QT_OVERLOADED_MACRO(QT_OBJECT_BINDABLE_PROPERTY, __VA_ARGS__) \
    QT_WARNING_POP

#define QT_OBJECT_BINDABLE_PROPERTY_WITH_ARGS_4(Class, Type, name, value)                          \
    static constexpr size_t _qt_property_##name##_offset()                                         \
    {                                                                                              \
        QT_WARNING_PUSH QT_WARNING_DISABLE_INVALID_OFFSETOF                                        \
        return offsetof(Class, name);                                                              \
        QT_WARNING_POP                                                                             \
    }                                                                                              \
    QObjectBindableProperty<Class, Type, Class::_qt_property_##name##_offset, nullptr> name =      \
            QObjectBindableProperty<Class, Type, Class::_qt_property_##name##_offset, nullptr>(    \
                    value);

#define QT_OBJECT_BINDABLE_PROPERTY_WITH_ARGS_5(Class, Type, name, value, Signal)                  \
    static constexpr size_t _qt_property_##name##_offset()                                         \
    {                                                                                              \
        QT_WARNING_PUSH QT_WARNING_DISABLE_INVALID_OFFSETOF                                        \
        return offsetof(Class, name);                                                              \
        QT_WARNING_POP                                                                             \
    }                                                                                              \
    QObjectBindableProperty<Class, Type, Class::_qt_property_##name##_offset, Signal> name =       \
            QObjectBindableProperty<Class, Type, Class::_qt_property_##name##_offset, Signal>(     \
                    value);

#define Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(...)                                                  \
    QT_WARNING_PUSH QT_WARNING_DISABLE_INVALID_OFFSETOF \
    QT_OVERLOADED_MACRO(QT_OBJECT_BINDABLE_PROPERTY_WITH_ARGS, __VA_ARGS__) \
    QT_WARNING_POP

template<typename Class, typename T, auto Offset, auto Getter>
class QObjectComputedProperty : public QUntypedPropertyData
{
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

public:
    using value_type = T;
    using parameter_type = T;

    QObjectComputedProperty() = default;

    parameter_type value() const
    {
        qGetBindingStorage(owner())->registerDependency(this);
        return (owner()->*Getter)();
    }

    std::conditional_t<QTypeTraits::is_dereferenceable_v<T>, parameter_type, void>
    operator->() const
    {
        if constexpr (QTypeTraits::is_dereferenceable_v<T>)
            return value();
        else
            return;
    }

    parameter_type operator*() const
    {
        return value();
    }

    operator parameter_type() const
    {
        return value();
    }

    constexpr bool hasBinding() const { return false; }

    template<typename Functor>
    QPropertyChangeHandler<Functor> onValueChanged(Functor f)
    {
        static_assert(std::is_invocable_v<Functor>, "Functor callback must be callable without any parameters");
        return QPropertyChangeHandler<Functor>(*this, std::move(f));
    }

    template<typename Functor>
    QPropertyChangeHandler<Functor> subscribe(Functor f)
    {
        static_assert(std::is_invocable_v<Functor>, "Functor callback must be callable without any parameters");
        f();
        return onValueChanged(std::move(f));
    }

    template<typename Functor>
    QPropertyNotifier addNotifier(Functor f)
    {
        static_assert(std::is_invocable_v<Functor>, "Functor callback must be callable without any parameters");
        return QPropertyNotifier(*this, std::move(f));
    }

    QtPrivate::QPropertyBindingData &bindingData() const
    {
        auto *storage = const_cast<QBindingStorage *>(qGetBindingStorage(owner()));
        return *storage->bindingData(const_cast<QObjectComputedProperty *>(this), true);
    }

    void notify() {
        // computed property can't store a binding, so there's nothing to mark
        auto *storage = const_cast<QBindingStorage *>(qGetBindingStorage(owner()));
        auto bd = storage->bindingData(const_cast<QObjectComputedProperty *>(this), false);
        if (bd)
            bd->notifyObservers(this, qGetBindingStorage(owner()));
    }
};

#define Q_OBJECT_COMPUTED_PROPERTY(Class, Type, name,  ...) \
    static constexpr size_t _qt_property_##name##_offset() { \
        QT_WARNING_PUSH QT_WARNING_DISABLE_INVALID_OFFSETOF \
        return offsetof(Class, name); \
        QT_WARNING_POP \
    } \
    QObjectComputedProperty<Class, Type, Class::_qt_property_##name##_offset, __VA_ARGS__> name;

#undef QT_SOURCE_LOCATION_NAMESPACE

QT_END_NAMESPACE

#endif // QPROPERTY_H
