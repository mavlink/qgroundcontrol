// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOBJECTDEFS_H
#define QOBJECTDEFS_H

#if defined(__OBJC__) && !defined(__cplusplus)
#  warning "File built in Objective-C mode (.m), but using Qt requires Objective-C++ (.mm)"
#endif

#include <QtCore/qnamespace.h>
#include <QtCore/qobjectdefs_impl.h>
#include <QtCore/qtcoreexports.h>
#include <QtCore/qtmetamacros.h>

QT_BEGIN_NAMESPACE

class QByteArray;
struct QArrayData;

class QString;

#ifndef QT_NO_META_MACROS
// macro for onaming members
#ifdef METHOD
#undef METHOD
#endif
#ifdef SLOT
#undef SLOT
#endif
#ifdef SIGNAL
#undef SIGNAL
#endif
#endif // QT_NO_META_MACROS

Q_CORE_EXPORT const char *qFlagLocation(const char *method);

#ifndef QT_NO_META_MACROS
# define QMETHOD_CODE  0                        // member type codes
# define QSLOT_CODE    1
# define QSIGNAL_CODE  2
# define QT_PREFIX_CODE(code, a) QT_STRINGIFY(code) #a
# define QT_STRINGIFY_METHOD(a) QT_PREFIX_CODE(QMETHOD_CODE, a)
# define QT_STRINGIFY_SLOT(a) QT_PREFIX_CODE(QSLOT_CODE, a)
# define QT_STRINGIFY_SIGNAL(a) QT_PREFIX_CODE(QSIGNAL_CODE, a)
# ifndef QT_NO_DEBUG
#  define QLOCATION "\0" __FILE__ ":" QT_STRINGIFY(__LINE__)
#  ifndef QT_NO_KEYWORDS
#   define METHOD(a)   qFlagLocation(QT_STRINGIFY_METHOD(a) QLOCATION)
#  endif
#  define SLOT(a)     qFlagLocation(QT_STRINGIFY_SLOT(a) QLOCATION)
#  define SIGNAL(a)   qFlagLocation(QT_STRINGIFY_SIGNAL(a) QLOCATION)
# else
#  ifndef QT_NO_KEYWORDS
#   define METHOD(a)  QT_STRINGIFY_METHOD(a)
#  endif
#  define SLOT(a)     QT_STRINGIFY_SLOT(a)
#  define SIGNAL(a)   QT_STRINGIFY_SIGNAL(a)
# endif
#endif // QT_NO_META_MACROS

#define Q_ARG(Type, data)           QtPrivate::Invoke::argument<Type>(QT_STRINGIFY(Type), data)
#define Q_RETURN_ARG(Type, data)    QtPrivate::Invoke::returnArgument<Type>(QT_STRINGIFY(Type), data)

class QObject;
class QMetaMethod;
class QMetaEnum;
class QMetaProperty;
class QMetaClassInfo;

namespace QtPrivate {
class QMetaTypeInterface;
template<typename T> constexpr const QMetaTypeInterface *qMetaTypeInterfaceForType();
}

struct QMethodRawArguments
{
    void **arguments;
};

#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
class Q_CORE_EXPORT QGenericArgument
{
public:
    inline QGenericArgument(const char *aName = nullptr, const void *aData = nullptr)
        : _data(aData), _name(aName) {}
    inline void *data() const { return const_cast<void *>(_data); }
    inline const char *name() const { return _name; }

private:
    const void *_data;
    const char *_name;
};

class Q_CORE_EXPORT QGenericReturnArgument: public QGenericArgument
{
public:
    inline QGenericReturnArgument(const char *aName = nullptr, void *aData = nullptr)
        : QGenericArgument(aName, aData)
        {}
};

template <class T>
class QArgument: public QGenericArgument
{
public:
    inline QArgument(const char *aName, const T &aData)
        : QGenericArgument(aName, static_cast<const void *>(&aData))
        {}
};
template <class T>
class QArgument<T &>: public QGenericArgument
{
public:
    inline QArgument(const char *aName, T &aData)
        : QGenericArgument(aName, static_cast<const void *>(&aData))
        {}
};


template <typename T>
class QReturnArgument: public QGenericReturnArgument
{
public:
    inline QReturnArgument(const char *aName, T &aData)
        : QGenericReturnArgument(aName, static_cast<void *>(&aData))
        {}
};
#endif

struct QMetaMethodArgument
{
    const QtPrivate::QMetaTypeInterface *metaType;
    const char *name;
    const void *data;
};

struct QMetaMethodReturnArgument
{
    const QtPrivate::QMetaTypeInterface *metaType;
    const char *name;
    void *data;
};

template <typename T>
struct QTemplatedMetaMethodReturnArgument : QMetaMethodReturnArgument
{
    using Type = T;
};

namespace QtPrivate {
namespace Invoke {
#if QT_VERSION <= QT_VERSION_CHECK(7, 0, 0)
template <typename... Args>
using AreOldStyleArgs = std::disjunction<std::is_base_of<QGenericArgument, Args>...>;

template <typename T, typename... Args> using IfNotOldStyleArgs =
    std::enable_if_t<!AreOldStyleArgs<Args...>::value, T>;
#else
template <typename T, typename... Args> using IfNotOldStyleArgs = T;
#endif

template <typename T> inline QMetaMethodArgument argument(const char *name, const T &t)
{
    if constexpr ((std::is_lvalue_reference_v<T> && std::is_const_v<std::remove_reference_t<T>>) ||
            !std::is_reference_v<T>) {
        return { qMetaTypeInterfaceForType<T>(), name, std::addressof(t) };
    } else {
        return { nullptr, name, std::addressof(t) };
    }
}

template <typename T>
inline QTemplatedMetaMethodReturnArgument<T> returnArgument(const char *name, T &t)
{
    return { qMetaTypeInterfaceForType<T>(), name, std::addressof(t) };
}

template <typename T> inline const char *typenameHelper(const T &)
{
    return nullptr;
}
template <typename T> inline const void *dataHelper(const T &t)
{
    return std::addressof(t);
}
template <typename T> inline const QMetaTypeInterface *metaTypeHelper(const T &)
{
    return qMetaTypeInterfaceForType<T>();
}

inline const char *typenameHelper(QMetaMethodArgument a)
{ return a.name; }
inline const void *dataHelper(QMetaMethodArgument a)
{ return a.data; }
inline const QMetaTypeInterface *metaTypeHelper(QMetaMethodArgument a)
{ return a.metaType; }

inline const char *typenameHelper(const char *) = delete;
template <typename T> inline const void *dataHelper(const char *) = delete;
inline const QMetaTypeInterface *metaTypeHelper(const char *) = delete;
inline const char *typenameHelper(const char16_t *) = delete;
template <typename T> inline const void *dataHelper(const char16_t *) = delete;
inline const QMetaTypeInterface *metaTypeHelper(const char16_t *) = delete;

} // namespace QtPrivate::Invoke

template <typename... Args> inline auto invokeMethodHelper(QMetaMethodReturnArgument r, const Args &... arguments)
{
    std::array params = { const_cast<const void *>(r.data), Invoke::dataHelper(arguments)... };
    std::array names = { r.name, Invoke::typenameHelper(arguments)... };
    std::array types = { r.metaType, Invoke::metaTypeHelper(arguments)... };
    static_assert(params.size() == types.size());
    static_assert(params.size() == names.size());

    struct R {
        decltype(params) parameters;
        decltype(names) typeNames;
        decltype(types) metaTypes;
        constexpr qsizetype parameterCount() const { return qsizetype(parameters.size()); }
    };
    return R { params, names, types };
}
} // namespace QtPrivate

template <typename T> void qReturnArg(const T &&) = delete;
template <typename T> inline QTemplatedMetaMethodReturnArgument<T> qReturnArg(T &data)
{
    return QtPrivate::Invoke::returnArgument(nullptr, data);
}

struct Q_CORE_EXPORT QMetaObject
{
    class Connection;
    const char *className() const;
    const QMetaObject *superClass() const;

    bool inherits(const QMetaObject *metaObject) const noexcept;
    QObject *cast(QObject *obj) const
    { return const_cast<QObject *>(cast(const_cast<const QObject *>(obj))); }
    const QObject *cast(const QObject *obj) const;

#if !defined(QT_NO_TRANSLATION) || defined(Q_QDOC)
    QString tr(const char *s, const char *c, int n = -1) const;
#endif // QT_NO_TRANSLATION

    QMetaType metaType() const;

    int methodOffset() const;
    int enumeratorOffset() const;
    int propertyOffset() const;
    int classInfoOffset() const;

    int constructorCount() const;
    int methodCount() const;
    int enumeratorCount() const;
    int propertyCount() const;
    int classInfoCount() const;

    int indexOfConstructor(const char *constructor) const;
    int indexOfMethod(const char *method) const;
    int indexOfSignal(const char *signal) const;
    int indexOfSlot(const char *slot) const;
    int indexOfEnumerator(const char *name) const;

    int indexOfProperty(const char *name) const;
    int indexOfClassInfo(const char *name) const;

    QMetaMethod constructor(int index) const;
    QMetaMethod method(int index) const;
    QMetaEnum enumerator(int index) const;
    QMetaProperty property(int index) const;
    QMetaClassInfo classInfo(int index) const;
    QMetaProperty userProperty() const;

    static bool checkConnectArgs(const char *signal, const char *method);
    static bool checkConnectArgs(const QMetaMethod &signal,
                                 const QMetaMethod &method);
    static QByteArray normalizedSignature(const char *method);
    static QByteArray normalizedType(const char *type);

    // internal index-based connect
    static Connection connect(const QObject *sender, int signal_index,
                        const QObject *receiver, int method_index,
                        int type = 0, int *types = nullptr);
    // internal index-based disconnect
    static bool disconnect(const QObject *sender, int signal_index,
                           const QObject *receiver, int method_index);
    static bool disconnectOne(const QObject *sender, int signal_index,
                              const QObject *receiver, int method_index);
    // internal slot-name based connect
    static void connectSlotsByName(QObject *o);

#ifdef Q_QDOC
    template<typename PointerToMemberFunction>
    static QMetaObject::Connection connect(const QObject *sender, const QMetaMethod &signal, const QObject *receiver, PointerToMemberFunction method, Qt::ConnectionType type = Qt::AutoConnection);
    template<typename Functor>
    static QMetaObject::Connection connect(const QObject *sender, const QMetaMethod &signal, const QObject *context, Functor functor, Qt::ConnectionType type = Qt::AutoConnection);
#else
    template <typename Func>
    static inline Connection
        connect(const QObject *sender, const QMetaMethod &signal,
                const typename QtPrivate::ContextTypeForFunctor<Func>::ContextType *context, Func &&slot,
                Qt::ConnectionType type = Qt::AutoConnection);
#endif // Q_QDOC

    // internal index-based signal activation
    static void activate(QObject *sender, int signal_index, void **argv);
    static void activate(QObject *sender, const QMetaObject *, int local_signal_index, void **argv);
    static void activate(QObject *sender, int signal_offset, int local_signal_index, void **argv);
    template <typename Ret, typename... Args> static inline void
    activate(QObject *sender, const QMetaObject *mo, int local_signal_index, Ret *ret, const Args &... args)
    {
        void *_a[] = {
            const_cast<void *>(reinterpret_cast<const volatile void *>(ret)),
            const_cast<void *>(reinterpret_cast<const volatile void *>(std::addressof(args)))...
        };
        activate(sender, mo, local_signal_index, _a);
    }

#if QT_VERSION <= QT_VERSION_CHECK(7, 0, 0)
    static bool invokeMethod(QObject *obj, const char *member,
                             Qt::ConnectionType,
                             QGenericReturnArgument ret,
                             QGenericArgument val0 = QGenericArgument(nullptr),
                             QGenericArgument val1 = QGenericArgument(),
                             QGenericArgument val2 = QGenericArgument(),
                             QGenericArgument val3 = QGenericArgument(),
                             QGenericArgument val4 = QGenericArgument(),
                             QGenericArgument val5 = QGenericArgument(),
                             QGenericArgument val6 = QGenericArgument(),
                             QGenericArgument val7 = QGenericArgument(),
                             QGenericArgument val8 = QGenericArgument(),
                             QGenericArgument val9 = QGenericArgument());

    static inline bool invokeMethod(QObject *obj, const char *member,
                             QGenericReturnArgument ret,
                             QGenericArgument val0 = QGenericArgument(nullptr),
                             QGenericArgument val1 = QGenericArgument(),
                             QGenericArgument val2 = QGenericArgument(),
                             QGenericArgument val3 = QGenericArgument(),
                             QGenericArgument val4 = QGenericArgument(),
                             QGenericArgument val5 = QGenericArgument(),
                             QGenericArgument val6 = QGenericArgument(),
                             QGenericArgument val7 = QGenericArgument(),
                             QGenericArgument val8 = QGenericArgument(),
                             QGenericArgument val9 = QGenericArgument())
    {
        return invokeMethod(obj, member, Qt::AutoConnection, ret, val0, val1, val2, val3,
                val4, val5, val6, val7, val8, val9);
    }

    static inline bool invokeMethod(QObject *obj, const char *member,
                             Qt::ConnectionType type,
                             QGenericArgument val0,
                             QGenericArgument val1 = QGenericArgument(),
                             QGenericArgument val2 = QGenericArgument(),
                             QGenericArgument val3 = QGenericArgument(),
                             QGenericArgument val4 = QGenericArgument(),
                             QGenericArgument val5 = QGenericArgument(),
                             QGenericArgument val6 = QGenericArgument(),
                             QGenericArgument val7 = QGenericArgument(),
                             QGenericArgument val8 = QGenericArgument(),
                             QGenericArgument val9 = QGenericArgument())
    {
        return invokeMethod(obj, member, type, QGenericReturnArgument(), val0, val1, val2,
                                 val3, val4, val5, val6, val7, val8, val9);
    }

    static inline bool invokeMethod(QObject *obj, const char *member,
                             QGenericArgument val0,
                             QGenericArgument val1 = QGenericArgument(),
                             QGenericArgument val2 = QGenericArgument(),
                             QGenericArgument val3 = QGenericArgument(),
                             QGenericArgument val4 = QGenericArgument(),
                             QGenericArgument val5 = QGenericArgument(),
                             QGenericArgument val6 = QGenericArgument(),
                             QGenericArgument val7 = QGenericArgument(),
                             QGenericArgument val8 = QGenericArgument(),
                             QGenericArgument val9 = QGenericArgument())
    {
        return invokeMethod(obj, member, Qt::AutoConnection, QGenericReturnArgument(), val0,
                val1, val2, val3, val4, val5, val6, val7, val8, val9);
    }
#endif // Qt < 7.0

    template <typename ReturnArg, typename... Args> static
#ifdef Q_QDOC
    bool
#else
    QtPrivate::Invoke::IfNotOldStyleArgs<bool, Args...>
#endif
    invokeMethod(QObject *obj, const char *member, Qt::ConnectionType c,
                 QTemplatedMetaMethodReturnArgument<ReturnArg> r, Args &&... arguments)
    {
        auto h = QtPrivate::invokeMethodHelper(r, std::forward<Args>(arguments)...);
        return invokeMethodImpl(obj, member, c, h.parameterCount(), h.parameters.data(),
                                h.typeNames.data(), h.metaTypes.data());
    }

    template <typename... Args> static
#ifdef Q_QDOC
    bool
#else
    QtPrivate::Invoke::IfNotOldStyleArgs<bool, Args...>
#endif
    invokeMethod(QObject *obj, const char *member, Qt::ConnectionType c, Args &&... arguments)
    {
        QTemplatedMetaMethodReturnArgument<void> r = {};
        return invokeMethod(obj, member, c, r, std::forward<Args>(arguments)...);
    }

    template <typename ReturnArg, typename... Args> static
#ifdef Q_QDOC
    bool
#else
    QtPrivate::Invoke::IfNotOldStyleArgs<bool, Args...>
#endif
    invokeMethod(QObject *obj, const char *member, QTemplatedMetaMethodReturnArgument<ReturnArg> r,
                 Args &&... arguments)
    {
        return invokeMethod(obj, member, Qt::AutoConnection, r, std::forward<Args>(arguments)...);
    }

    template <typename... Args> static
#ifdef Q_QDOC
    bool
#else
    QtPrivate::Invoke::IfNotOldStyleArgs<bool, Args...>
#endif
    invokeMethod(QObject *obj, const char *member, Args &&... arguments)
    {
        QTemplatedMetaMethodReturnArgument<void> r = {};
        return invokeMethod(obj, member, Qt::AutoConnection, r, std::forward<Args>(arguments)...);
    }

#ifdef Q_QDOC
    template<typename Functor, typename FunctorReturnType>
    static bool invokeMethod(QObject *context, Functor &&function, Qt::ConnectionType type = Qt::AutoConnection, FunctorReturnType *ret = nullptr);
    template<typename Functor, typename FunctorReturnType>
    static bool invokeMethod(QObject *context, Functor &&function, FunctorReturnType *ret);

    template<typename Functor, typename FunctorReturnType, typename... Args>
    static bool invokeMethod(QObject *context, Functor &&function, Qt::ConnectionType type, QTemplatedMetaMethodReturnArgument<FunctorReturnType> ret, Args &&...arguments);
    template<typename Functor, typename FunctorReturnType, typename... Args>
    static bool invokeMethod(QObject *context, Functor &&function, QTemplatedMetaMethodReturnArgument<FunctorReturnType> ret, Args &&...arguments);
    template<typename Functor, typename... Args>
    static bool invokeMethod(QObject *context, Functor &&function, Qt::ConnectionType type, Args &&...arguments);
    template<typename Functor, typename... Args>
    static bool invokeMethod(QObject *context, Functor &&function, Args &&...arguments);
#else
    template <typename Func>
    static std::enable_if_t<!std::disjunction_v<std::is_convertible<Func, const char *>,
                                                QtPrivate::Invoke::AreOldStyleArgs<Func>>,
                            bool>
    invokeMethod(typename QtPrivate::ContextTypeForFunctor<Func>::ContextType *object,
                 Func &&function, Qt::ConnectionType type,
                 typename QtPrivate::Callable<Func>::ReturnType *ret)
    {
        using R = typename QtPrivate::Callable<Func>::ReturnType;
        const auto getReturnArg = [ret]() -> QTemplatedMetaMethodReturnArgument<R> {
            if constexpr (std::is_void_v<R>)
                return {};
            else
                return ret ? qReturnArg(*ret) : QTemplatedMetaMethodReturnArgument<R>{};
        };
        return invokeMethod(object, std::forward<Func>(function), type, getReturnArg());
    }
    template <typename Func>
    static std::enable_if_t<!std::disjunction_v<std::is_convertible<Func, const char *>,
                                                QtPrivate::Invoke::AreOldStyleArgs<Func>>,
                            bool>
    invokeMethod(typename QtPrivate::ContextTypeForFunctor<Func>::ContextType *object,
                 Func &&function, typename QtPrivate::Callable<Func>::ReturnType *ret)
    {
        return invokeMethod(object, std::forward<Func>(function), Qt::AutoConnection, ret);
    }

    template <typename Func, typename... Args>
    static std::enable_if_t<!std::disjunction_v<std::is_convertible<Func, const char *>,
                                                QtPrivate::Invoke::AreOldStyleArgs<Args...>>,
                            bool>
    invokeMethod(typename QtPrivate::ContextTypeForFunctor<Func>::ContextType *object,
                 Func &&function, Qt::ConnectionType type,
                 QTemplatedMetaMethodReturnArgument<
                         typename QtPrivate::Callable<Func, Args...>::ReturnType>
                         ret,
                 Args &&...args)
    {
        return invokeMethodCallableHelper(object, std::forward<Func>(function), type, ret,
                                          std::forward<Args>(args)...);
    }

    template <typename Func, typename... Args>
    static std::enable_if_t<!std::disjunction_v<std::is_convertible<Func, const char *>,
                                                QtPrivate::Invoke::AreOldStyleArgs<Args...>>,
                            bool>
    invokeMethod(typename QtPrivate::ContextTypeForFunctor<Func>::ContextType *object,
                 Func &&function, Qt::ConnectionType type, Args &&...args)
    {
        using R = typename QtPrivate::Callable<Func, Args...>::ReturnType;
        QTemplatedMetaMethodReturnArgument<R> r{ QtPrivate::qMetaTypeInterfaceForType<R>(), nullptr,
                                                 nullptr };
        return invokeMethod(object, std::forward<Func>(function), type, r,
                            std::forward<Args>(args)...);
    }

    template <typename Func, typename... Args>
    static std::enable_if_t<!std::disjunction_v<std::is_convertible<Func, const char *>,
                                                QtPrivate::Invoke::AreOldStyleArgs<Args...>>,
                            bool>
    invokeMethod(typename QtPrivate::ContextTypeForFunctor<Func>::ContextType *object,
                 Func &&function,
                 QTemplatedMetaMethodReturnArgument<
                         typename QtPrivate::Callable<Func, Args...>::ReturnType>
                         ret,
                 Args &&...args)
    {
        return invokeMethod(object, std::forward<Func>(function), Qt::AutoConnection, ret,
                            std::forward<Args>(args)...);
    }

    template <typename Func, typename... Args>
    static std::enable_if_t<!std::disjunction_v<std::is_convertible<Func, const char *>,
                                                QtPrivate::Invoke::AreOldStyleArgs<Args...>>,
                            bool>
    invokeMethod(typename QtPrivate::ContextTypeForFunctor<Func>::ContextType *object,
                 Func &&function, Args &&...args)
    {
        using R = typename QtPrivate::Callable<Func, Args...>::ReturnType;
        QTemplatedMetaMethodReturnArgument<R> r{ QtPrivate::qMetaTypeInterfaceForType<R>(), nullptr,
                                                 nullptr };
        return invokeMethod(object, std::forward<Func>(function), Qt::AutoConnection, r,
                            std::forward<Args>(args)...);
    }

#endif

#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
    QObject *newInstance(QGenericArgument val0,
                         QGenericArgument val1 = QGenericArgument(),
                         QGenericArgument val2 = QGenericArgument(),
                         QGenericArgument val3 = QGenericArgument(),
                         QGenericArgument val4 = QGenericArgument(),
                         QGenericArgument val5 = QGenericArgument(),
                         QGenericArgument val6 = QGenericArgument(),
                         QGenericArgument val7 = QGenericArgument(),
                         QGenericArgument val8 = QGenericArgument(),
                         QGenericArgument val9 = QGenericArgument()) const;
#endif

    template <typename... Args>
#ifdef Q_QDOC
    QObject *
#else
    QtPrivate::Invoke::IfNotOldStyleArgs<QObject *, Args...>
#endif
    newInstance(Args &&... arguments) const
    {
        auto h = QtPrivate::invokeMethodHelper(QMetaMethodReturnArgument{}, std::forward<Args>(arguments)...);
        return newInstanceImpl(this, h.parameterCount(), h.parameters.data(),
                               h.typeNames.data(), h.metaTypes.data());
    }

    enum Call {
        InvokeMetaMethod,
        ReadProperty,
        WriteProperty,
        ResetProperty,
        CreateInstance,
        IndexOfMethod,
        RegisterPropertyMetaType,
        RegisterMethodArgumentMetaType,
        BindableProperty,
        CustomCall,
        ConstructInPlace,
    };

    int static_metacall(Call, int, void **) const;
    static int metacall(QObject *, Call, int, void **);

    template <const QMetaObject &MO> static constexpr const QMetaObject *staticMetaObject()
    {
        return &MO;
    }

    struct SuperData {
        using Getter = const QMetaObject *(*)();
        const QMetaObject *direct;
        SuperData() = default;
        constexpr SuperData(std::nullptr_t) : direct(nullptr) {}
        constexpr SuperData(const QMetaObject *mo) : direct(mo) {}

        constexpr const QMetaObject *operator->() const { return operator const QMetaObject *(); }

#ifdef QT_NO_DATA_RELOCATION
        Getter indirect = nullptr;
        constexpr SuperData(Getter g) : direct(nullptr), indirect(g) {}
        constexpr operator const QMetaObject *() const
        { return indirect ? indirect() : direct; }
        template <const QMetaObject &MO> static constexpr SuperData link()
        { return SuperData(QMetaObject::staticMetaObject<MO>); }
#else
        constexpr SuperData(Getter g) : direct(g()) {}
        constexpr operator const QMetaObject *() const
        { return direct; }
        template <const QMetaObject &MO> static constexpr SuperData link()
        { return SuperData(QMetaObject::staticMetaObject<MO>()); }
#endif
    };

    struct Data { // private data
        SuperData superdata;
        const uint *stringdata;
        const uint *data;
        typedef void (*StaticMetacallFunction)(QObject *, QMetaObject::Call, int, void **);
        StaticMetacallFunction static_metacall;
        const SuperData *relatedMetaObjects;
        const QtPrivate::QMetaTypeInterface *const *metaTypes;
        void *extradata; //reserved for future use
    } d;

private:
    // Just need to have this here with a separate name so the other inline
    // functions can call this without any ambiguity
    template <typename Func, typename... Args>
    static bool
    invokeMethodCallableHelper(typename QtPrivate::ContextTypeForFunctor<Func>::ContextType *object,
            Func &&function, Qt::ConnectionType type, const QMetaMethodReturnArgument &ret,
            Args &&...args)
    {
        using Callable = QtPrivate::Callable<Func, Args...>;
        using ExpectedArguments = typename Callable::Arguments;
        static_assert(sizeof...(Args) <= ExpectedArguments::size, "Too many arguments");
        using ActualArguments = QtPrivate::List<Args...>;
        static_assert(QtPrivate::CheckCompatibleArguments<ActualArguments,
                              ExpectedArguments>::value,
                "Incompatible arguments");

        auto h = QtPrivate::invokeMethodHelper(ret, std::forward<Args>(args)...);

        // NOLINTBEGIN(clang-analyzer-cplusplus.NewDeleteLeaks)
        auto callable = new QtPrivate::QCallableObject<std::decay_t<Func>, ActualArguments,
                typename Callable::ReturnType>(std::forward<Func>(function));
        return invokeMethodImpl(object, callable, type, h.parameterCount(), h.parameters.data(),
                h.typeNames.data(), h.metaTypes.data());
        // NOLINTEND(clang-analyzer-cplusplus.NewDeleteLeaks)
    }

    static bool invokeMethodImpl(QObject *object, const char *member, Qt::ConnectionType type,
                                 qsizetype parameterCount, const void *const *parameters, const char *const *names,
                                 const QtPrivate::QMetaTypeInterface * const *metaTypes);
    static bool invokeMethodImpl(QObject *object, QtPrivate::QSlotObjectBase *slotObj,
                                 Qt::ConnectionType type, qsizetype parameterCount,
                                 const void *const *params, const char *const *names,
                                 const QtPrivate::QMetaTypeInterface *const *metaTypes);
#if QT_CORE_REMOVED_SINCE(6, 7)
    static bool invokeMethodImpl(QObject *object, QtPrivate::QSlotObjectBase *slot, Qt::ConnectionType type, void *ret);
#endif
    static QObject *newInstanceImpl(const QMetaObject *mobj, qsizetype parameterCount,
                                    const void **parameters, const char **typeNames,
                                    const QtPrivate::QMetaTypeInterface **metaTypes);

    static QMetaObject::Connection connectImpl(const QObject *sender, const QMetaMethod& signal,
                                            const QObject *receiver, void **slotPtr,
                                            QtPrivate::QSlotObjectBase *slot, Qt::ConnectionType type);

    friend class QTimer;
    friend class QChronoTimer;
};

class Q_CORE_EXPORT QMetaObject::Connection {
    void *d_ptr; //QObjectPrivate::Connection*
    explicit Connection(void *data) : d_ptr(data) {  }
    friend class QObject;
    friend class QObjectPrivate;
    friend struct QMetaObject;
    bool isConnected_helper() const;
public:
    ~Connection();
    Connection();
    Connection(const Connection &other);
    Connection &operator=(const Connection &other);
#ifdef Q_QDOC
    operator bool() const;
#else
    // still using the restricted bool trick here, in order to support
    // code using copy-init (e.g. `bool ok = connect(...)`)
    typedef void *Connection::*RestrictedBool;
    operator RestrictedBool() const { return d_ptr && isConnected_helper() ? &Connection::d_ptr : nullptr; }
#endif

    Connection(Connection &&other) noexcept : d_ptr(std::exchange(other.d_ptr, nullptr)) {}
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(Connection)
    void swap(Connection &other) noexcept { qt_ptr_swap(d_ptr, other.d_ptr); }
};

template <typename Func>
QMetaObject::Connection
    QMetaObject::connect(const QObject *sender, const QMetaMethod &signal,
                         const typename QtPrivate::ContextTypeForFunctor<Func>::ContextType *context, Func &&slot,
                         Qt::ConnectionType type)
{
    using Slot = std::decay_t<Func>;
    using FunctionSlotType = QtPrivate::FunctionPointer<Slot>;
    void **pSlot = nullptr;
    QtPrivate::QSlotObjectBase *slotObject;
    if constexpr (FunctionSlotType::ArgumentCount != -1) {
        slotObject = new QtPrivate::QCallableObject<Slot, typename FunctionSlotType::Arguments, typename FunctionSlotType::ReturnType>(std::forward<Func>(slot));
        if constexpr (FunctionSlotType::IsPointerToMemberFunction) {
            pSlot = const_cast<void **>(reinterpret_cast<void *const *>(&slot));
        } else {
            Q_ASSERT_X((type & Qt::UniqueConnection) == 0, "",
                "QObject::connect: Unique connection requires the slot to be a pointer to "
                "a member function of a QObject subclass.");
        }
    } else {
        using FunctorSlotType = QtPrivate::FunctionPointer<decltype(&Slot::operator())>;
        slotObject = new QtPrivate::QCallableObject<Slot, typename FunctorSlotType::Arguments, typename FunctorSlotType::ReturnType>(std::forward<Func>(slot));
    }
    return QMetaObject::connectImpl(sender, signal, context, pSlot, slotObject, type);
}

inline void swap(QMetaObject::Connection &lhs, QMetaObject::Connection &rhs) noexcept
{
    lhs.swap(rhs);
}

inline const QMetaObject *QMetaObject::superClass() const
{ return d.superdata; }

namespace QtPrivate {
    // Trait that tells if a QObject has a Q_OBJECT macro
    template <typename Object> struct HasQ_OBJECT_Macro {
        template <typename T>
        static char test(int (T::*)(QMetaObject::Call, int, void **));
        static int test(int (Object::*)(QMetaObject::Call, int, void **));
        enum { Value =  sizeof(test(&Object::qt_metacall)) == sizeof(int) };
    };

    template <class TgtType, class SrcType>
    inline TgtType qobject_cast_helper(SrcType *object)
    {
        using ObjType = std::remove_cv_t<std::remove_pointer_t<TgtType>> ;
        static_assert(std::is_pointer_v<TgtType>,
                "qobject_cast requires to cast towards a pointer type");
        static_assert(HasQ_OBJECT_Macro<ObjType>::Value,
                "qobject_cast requires the type to have a Q_OBJECT macro");

        if constexpr (std::is_final_v<ObjType>) {
            if (object && object->metaObject() == &ObjType::staticMetaObject)
                return static_cast<TgtType>(object);
            return nullptr;
        } else {
            return static_cast<TgtType>(ObjType::staticMetaObject.cast(object));
        }
    }
}

QT_END_NAMESPACE

#endif // QOBJECTDEFS_H
