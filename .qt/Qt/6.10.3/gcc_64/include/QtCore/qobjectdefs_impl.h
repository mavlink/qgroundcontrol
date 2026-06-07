// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2013 Olivier Goffart <ogoffart@woboq.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOBJECTDEFS_H
#error Do not include qobjectdefs_impl.h directly
#include <QtCore/qnamespace.h>
#endif

#if 0
#pragma qt_sync_skip_header_check
#pragma qt_sync_stop_processing
#endif

#include <QtCore/qfunctionaltools_impl.h>

#include <memory>

QT_BEGIN_NAMESPACE
class QObject;
class QObjectPrivate;
class QMetaMethod;
class QByteArray;

namespace QtPrivate {
    template <typename T> struct RemoveRef { typedef T Type; };
    template <typename T> struct RemoveRef<T&> { typedef T Type; };
    template <typename T> struct RemoveConstRef { typedef T Type; };
    template <typename T> struct RemoveConstRef<const T&> { typedef T Type; };

    /*
       The following List classes are used to help to handle the list of arguments.
       It follow the same principles as the lisp lists.
       List_Left<L,N> take a list and a number as a parameter and returns (via the Value typedef,
       the list composed of the first N element of the list
     */
    // With variadic template, lists are represented using a variadic template argument instead of the lisp way
    template <typename... Ts> struct List { static constexpr size_t size = sizeof...(Ts); };
    template<typename T> struct SizeOfList { static constexpr size_t value = 1; };
    template<> struct SizeOfList<List<>> { static constexpr size_t value = 0; };
    template<typename ...Ts> struct SizeOfList<List<Ts...>>  { static constexpr size_t value = List<Ts...>::size; };
    template <typename Head, typename... Tail> struct List<Head, Tail...> {
        static constexpr size_t size = 1 + sizeof...(Tail);
        typedef Head Car; typedef List<Tail...> Cdr;
    };
    template <typename, typename> struct List_Append;
    template <typename... L1, typename...L2> struct List_Append<List<L1...>, List<L2...>> { typedef List<L1..., L2...> Value; };
    template <typename L, int N> struct List_Left {
        typedef typename List_Append<List<typename L::Car>,typename List_Left<typename L::Cdr, N - 1>::Value>::Value Value;
    };
    template <typename L> struct List_Left<L, 0> { typedef List<> Value; };

    /*
        This is used to store the return value from a slot, whether the caller
        wants to store this value (QMetaObject::invokeMethod() with
        qReturnArg() or non-void signal ) or not.
     */
    struct FunctorCallBase
    {
        template <typename R, typename Lambda>
        static void call_internal([[maybe_unused]] void **args, Lambda &&fn)
            noexcept(std::is_nothrow_invocable_v<Lambda>)
        {
            if constexpr (std::is_void_v<R> || std::is_void_v<std::invoke_result_t<Lambda>>) {
                std::forward<Lambda>(fn)();
            } else {
                if (args[0])
                    *reinterpret_cast<R *>(args[0]) = std::forward<Lambda>(fn)();
                else
                    [[maybe_unused]] auto r = std::forward<Lambda>(fn)();
            }
        }
    };

    /*
      The FunctionPointer<Func> struct is a type trait for function pointer.
        - ArgumentCount  is the number of argument, or -1 if it is unknown
        - the Object typedef is the Object of a pointer to member function
        - the Arguments typedef is the list of argument (in a QtPrivate::List)
        - the Function typedef is an alias to the template parameter Func
        - the call<Args, R>(f,o,args) method is used to call that slot
            Args is the list of argument of the signal
            R is the return type of the signal
            f is the function pointer
            o is the receiver object
            and args is the array of pointer to arguments, as used in qt_metacall

       The Functor<Func,N> struct is the helper to call a functor of N argument.
       Its call function is the same as the FunctionPointer::call function.
     */
    template<typename Func> struct FunctionPointer { enum {ArgumentCount = -1, IsPointerToMemberFunction = false}; };

    template<typename ObjPrivate> inline void assertObjectType(QObjectPrivate *d);
    template<typename Obj> inline void assertObjectType(QObject *o)
    {
        // ensure all three compile
        [[maybe_unused]] auto staticcast = [](QObject *obj) { return static_cast<Obj *>(obj); };
        [[maybe_unused]] auto qobjcast = [](QObject *obj) { return Obj::staticMetaObject.cast(obj); };
#ifdef __cpp_rtti
        [[maybe_unused]] auto dyncast = [](QObject *obj) { return dynamic_cast<Obj *>(obj); };
        auto cast = dyncast;
#else
        auto cast = qobjcast;
#endif
        Q_ASSERT_X(cast(o), Obj::staticMetaObject.className(),
                   "Called object is not of the correct type (class destructor may have already run)");
    }

    template <typename, typename, typename, typename> struct FunctorCall;
    template <size_t... II, typename... SignalArgs, typename R, typename Function>
    struct FunctorCall<std::index_sequence<II...>, List<SignalArgs...>, R, Function> : FunctorCallBase
    {
        static void call(Function &f, void **arg)
        {
            call_internal<R>(arg, [&] {
                return f((*reinterpret_cast<typename RemoveRef<SignalArgs>::Type *>(arg[II+1]))...);
            });
        }
    };
    template <size_t... II, typename... SignalArgs, typename R, typename... SlotArgs, typename SlotRet, class Obj>
    struct FunctorCall<std::index_sequence<II...>, List<SignalArgs...>, R, SlotRet (Obj::*)(SlotArgs...)> : FunctorCallBase
    {
        static void call(SlotRet (Obj::*f)(SlotArgs...), Obj *o, void **arg)
        {
            assertObjectType<Obj>(o);
            call_internal<R>(arg, [&] {
                return (o->*f)((*reinterpret_cast<typename RemoveRef<SignalArgs>::Type *>(arg[II+1]))...);
            });
        }
    };
    template <size_t... II, typename... SignalArgs, typename R, typename... SlotArgs, typename SlotRet, class Obj>
    struct FunctorCall<std::index_sequence<II...>, List<SignalArgs...>, R, SlotRet (Obj::*)(SlotArgs...) const> : FunctorCallBase
    {
        static void call(SlotRet (Obj::*f)(SlotArgs...) const, Obj *o, void **arg)
        {
            assertObjectType<Obj>(o);
            call_internal<R>(arg, [&] {
                return (o->*f)((*reinterpret_cast<typename RemoveRef<SignalArgs>::Type *>(arg[II+1]))...);
            });
        }
    };
    template <size_t... II, typename... SignalArgs, typename R, typename... SlotArgs, typename SlotRet, class Obj>
    struct FunctorCall<std::index_sequence<II...>, List<SignalArgs...>, R, SlotRet (Obj::*)(SlotArgs...) noexcept> : FunctorCallBase
    {
        static void call(SlotRet (Obj::*f)(SlotArgs...) noexcept, Obj *o, void **arg)
        {
            assertObjectType<Obj>(o);
            call_internal<R>(arg, [&]() noexcept {
                return (o->*f)((*reinterpret_cast<typename RemoveRef<SignalArgs>::Type *>(arg[II+1]))...);
            });
        }
    };
    template <size_t... II, typename... SignalArgs, typename R, typename... SlotArgs, typename SlotRet, class Obj>
    struct FunctorCall<std::index_sequence<II...>, List<SignalArgs...>, R, SlotRet (Obj::*)(SlotArgs...) const noexcept> : FunctorCallBase
    {
        static void call(SlotRet (Obj::*f)(SlotArgs...) const noexcept, Obj *o, void **arg)
        {
            assertObjectType<Obj>(o);
            call_internal<R>(arg, [&]() noexcept {
                return (o->*f)((*reinterpret_cast<typename RemoveRef<SignalArgs>::Type *>(arg[II+1]))...);
            });
        }
    };

    template<class Obj, typename Ret, typename... Args> struct FunctionPointer<Ret (Obj::*) (Args...)>
    {
        typedef Obj Object;
        typedef List<Args...>  Arguments;
        typedef Ret ReturnType;
        typedef Ret (Obj::*Function) (Args...);
        enum {ArgumentCount = sizeof...(Args), IsPointerToMemberFunction = true};
        template <typename SignalArgs, typename R>
        static void call(Function f, Obj *o, void **arg) {
            FunctorCall<std::index_sequence_for<Args...>, SignalArgs, R, Function>::call(f, o, arg);
        }
    };
    template<class Obj, typename Ret, typename... Args> struct FunctionPointer<Ret (Obj::*) (Args...) const>
    {
        typedef Obj Object;
        typedef List<Args...>  Arguments;
        typedef Ret ReturnType;
        typedef Ret (Obj::*Function) (Args...) const;
        enum {ArgumentCount = sizeof...(Args), IsPointerToMemberFunction = true};
        template <typename SignalArgs, typename R>
        static void call(Function f, Obj *o, void **arg) {
            FunctorCall<std::index_sequence_for<Args...>, SignalArgs, R, Function>::call(f, o, arg);
        }
    };

    template<typename Ret, typename... Args> struct FunctionPointer<Ret (*) (Args...)>
    {
        typedef List<Args...> Arguments;
        typedef Ret ReturnType;
        typedef Ret (*Function) (Args...);
        enum {ArgumentCount = sizeof...(Args), IsPointerToMemberFunction = false};
        template <typename SignalArgs, typename R>
        static void call(Function f, void *, void **arg) {
            FunctorCall<std::index_sequence_for<Args...>, SignalArgs, R, Function>::call(f, arg);
        }
    };

    template<class Obj, typename Ret, typename... Args> struct FunctionPointer<Ret (Obj::*) (Args...) noexcept>
    {
        typedef Obj Object;
        typedef List<Args...>  Arguments;
        typedef Ret ReturnType;
        typedef Ret (Obj::*Function) (Args...) noexcept;
        enum {ArgumentCount = sizeof...(Args), IsPointerToMemberFunction = true};
        template <typename SignalArgs, typename R>
        static void call(Function f, Obj *o, void **arg) {
            FunctorCall<std::index_sequence_for<Args...>, SignalArgs, R, Function>::call(f, o, arg);
        }
    };
    template<class Obj, typename Ret, typename... Args> struct FunctionPointer<Ret (Obj::*) (Args...) const noexcept>
    {
        typedef Obj Object;
        typedef List<Args...>  Arguments;
        typedef Ret ReturnType;
        typedef Ret (Obj::*Function) (Args...) const noexcept;
        enum {ArgumentCount = sizeof...(Args), IsPointerToMemberFunction = true};
        template <typename SignalArgs, typename R>
        static void call(Function f, Obj *o, void **arg) {
            FunctorCall<std::index_sequence_for<Args...>, SignalArgs, R, Function>::call(f, o, arg);
        }
    };

    template<typename Ret, typename... Args> struct FunctionPointer<Ret (*) (Args...) noexcept>
    {
        typedef List<Args...> Arguments;
        typedef Ret ReturnType;
        typedef Ret (*Function) (Args...) noexcept;
        enum {ArgumentCount = sizeof...(Args), IsPointerToMemberFunction = false};
        template <typename SignalArgs, typename R>
        static void call(Function f, void *, void **arg) {
            FunctorCall<std::index_sequence_for<Args...>, SignalArgs, R, Function>::call(f, arg);
        }
    };

    // Traits to detect if there is a conversion between two types,
    // and that conversion does not include a narrowing conversion.
    template <typename T>
    struct NarrowingDetector { T t[1]; }; // from P0608

    template <typename From, typename To, typename Enable = void>
    struct IsConvertibleWithoutNarrowing : std::false_type {};

    template <typename From, typename To>
    struct IsConvertibleWithoutNarrowing<From, To,
            std::void_t< decltype( NarrowingDetector<To>{ {std::declval<From>()} } ) >
        > : std::true_type {};

    // Check for the actual arguments. If they are exactly the same,
    // then don't bother checking for narrowing; as a by-product,
    // this solves the problem of incomplete types (which must be supported,
    // or they would error out in the trait above).
    template <typename From, typename To, typename Enable = void>
    struct AreArgumentsConvertibleWithoutNarrowingBase : std::false_type {};

    template <typename From, typename To>
    struct AreArgumentsConvertibleWithoutNarrowingBase<From, To,
        std::enable_if_t<
            std::disjunction_v<std::is_same<From, To>, IsConvertibleWithoutNarrowing<From, To>>
        >
    > : std::true_type {};

    /*
       Logic that check if the arguments of the slot matches the argument of the signal.
       To be used like this:
       static_assert(CheckCompatibleArguments<FunctionPointer<Signal>::Arguments, FunctionPointer<Slot>::Arguments>::value)
    */
    template<typename A1, typename A2> struct AreArgumentsCompatible {
        static int test(const std::remove_reference_t<A2>&);
        static char test(...);
        enum { value = sizeof(test(std::declval<std::remove_reference_t<A1>>())) == sizeof(int) };
#ifdef QT_NO_NARROWING_CONVERSIONS_IN_CONNECT
        using AreArgumentsConvertibleWithoutNarrowing = AreArgumentsConvertibleWithoutNarrowingBase<std::decay_t<A1>, std::decay_t<A2>>;
        static_assert(AreArgumentsConvertibleWithoutNarrowing::value, "Signal and slot arguments are not compatible (narrowing)");
#endif
    };
    template<typename A1, typename A2> struct AreArgumentsCompatible<A1, A2&> { enum { value = false }; };
    template<typename A> struct AreArgumentsCompatible<A&, A&> { enum { value = true }; };
    // void as a return value
    template<typename A> struct AreArgumentsCompatible<void, A> { enum { value = true }; };
    template<typename A> struct AreArgumentsCompatible<A, void> { enum { value = true }; };
    template<> struct AreArgumentsCompatible<void, void> { enum { value = true }; };

    template <typename List1, typename List2> struct CheckCompatibleArguments { enum { value = false }; };
    template <> struct CheckCompatibleArguments<List<>, List<>> { enum { value = true }; };
    template <typename List1> struct CheckCompatibleArguments<List1, List<>> { enum { value = true }; };
    template <typename Arg1, typename Arg2, typename... Tail1, typename... Tail2>
    struct CheckCompatibleArguments<List<Arg1, Tail1...>, List<Arg2, Tail2...>>
    {
        enum { value = AreArgumentsCompatible<typename RemoveConstRef<Arg1>::Type, typename RemoveConstRef<Arg2>::Type>::value
                    && CheckCompatibleArguments<List<Tail1...>, List<Tail2...>>::value };
    };

    /*
       Find the maximum number of arguments a functor object can take and be still compatible with
       the arguments from the signal.
       Value is the number of arguments, or -1 if nothing matches.
     */
    template <typename Functor, typename ArgList> struct ComputeFunctorArgumentCount;

    template <typename Functor, typename ArgList, bool Done> struct ComputeFunctorArgumentCountHelper
    { enum { Value = -1 }; };
    template <typename Functor, typename First, typename... ArgList>
    struct ComputeFunctorArgumentCountHelper<Functor, List<First, ArgList...>, false>
        : ComputeFunctorArgumentCount<Functor,
            typename List_Left<List<First, ArgList...>, sizeof...(ArgList)>::Value> {};

    template <typename Functor, typename... ArgList> struct ComputeFunctorArgumentCount<Functor, List<ArgList...>>
    {
        template <typename F> static auto test(F f) -> decltype(((f.operator()((std::declval<ArgList>())...)), int()));
        static char test(...);
        enum {
            Ok = sizeof(test(std::declval<Functor>())) == sizeof(int),
            Value = Ok ? int(sizeof...(ArgList)) : int(ComputeFunctorArgumentCountHelper<Functor, List<ArgList...>, Ok>::Value)
        };
    };

    /* get the return type of a functor, given the signal argument list  */
    template <typename Functor, typename ArgList> struct FunctorReturnType;
    template <typename Functor, typename... ArgList> struct FunctorReturnType<Functor, List<ArgList...>>
        : std::invoke_result<Functor, ArgList...>
    { };

    template<typename Func, typename... Args>
    struct FunctorCallable
    {
        using ReturnType = std::invoke_result_t<Func, Args...>;
        using Function = ReturnType(*)(Args...);
        enum {ArgumentCount = sizeof...(Args)};
        using Arguments = QtPrivate::List<Args...>;

        template <typename SignalArgs, typename R>
        static void call(Func &f, void *, void **arg) {
            FunctorCall<std::index_sequence_for<Args...>, SignalArgs, R, Func>::call(f, arg);
        }
    };

    template <typename Functor, typename... Args>
    struct HasCallOperatorAcceptingArgs
    {
    private:
        template <typename F, typename = void>
        struct Test : std::false_type
        {
        };
        // We explicitly use .operator() to not return true for pointers to free/static function
        template <typename F>
        struct Test<F, std::void_t<decltype(std::declval<F>().operator()(std::declval<Args>()...))>>
            : std::true_type
        {
        };

    public:
        using Type = Test<Functor>;
        static constexpr bool value = Type::value;
    };

    template <typename Functor, typename... Args>
    constexpr bool
            HasCallOperatorAcceptingArgs_v = HasCallOperatorAcceptingArgs<Functor, Args...>::value;

    template <typename Func, typename... Args>
    struct CallableHelper
    {
    private:
        // Could've been std::conditional_t, but that requires all branches to
        // be valid
        static auto Resolve(std::true_type CallOperator) -> FunctorCallable<Func, Args...>;
        static auto Resolve(std::false_type CallOperator) -> FunctionPointer<std::decay_t<Func>>;

    public:
        using Type = decltype(Resolve(typename HasCallOperatorAcceptingArgs<std::decay_t<Func>,
                Args...>::Type{}));
    };

    template<typename Func, typename... Args>
    struct Callable : CallableHelper<Func, Args...>::Type
    {};
    template<typename Func, typename... Args>
    struct Callable<Func, List<Args...>> : CallableHelper<Func, Args...>::Type
    {};

    /*
        Wrapper around ComputeFunctorArgumentCount and CheckCompatibleArgument,
        depending on whether \a Functor is a PMF or not. Returns -1 if \a Func is
        not compatible with the \a ExpectedArguments, otherwise returns >= 0.
    */
    template<typename Prototype, typename Functor>
    inline constexpr std::enable_if_t<!std::disjunction_v<std::is_convertible<Prototype, const char *>,
                                                          std::is_same<std::decay_t<Prototype>, QMetaMethod>,
                                                          std::is_convertible<Functor, const char *>,
                                                          std::is_same<std::decay_t<Functor>, QMetaMethod>
                                                         >,
                                      int>
    countMatchingArguments()
    {
        using ExpectedArguments = typename QtPrivate::FunctionPointer<Prototype>::Arguments;
        using Actual = std::decay_t<Functor>;

        if constexpr (QtPrivate::FunctionPointer<Actual>::IsPointerToMemberFunction
                   || QtPrivate::FunctionPointer<Actual>::ArgumentCount >= 0) {
            // PMF or free function
            using ActualArguments = typename QtPrivate::FunctionPointer<Actual>::Arguments;
            if constexpr (QtPrivate::CheckCompatibleArguments<ExpectedArguments, ActualArguments>::value)
                return QtPrivate::FunctionPointer<Actual>::ArgumentCount;
            else
                return -1;
        } else {
            // lambda or functor
            return QtPrivate::ComputeFunctorArgumentCount<Actual, ExpectedArguments>::Value;
        }
    }

    // internal base class (interface) containing functions required to call a slot managed by a pointer to function.
    class QSlotObjectBase
    {
        // Don't use virtual functions here; we don't want the
        // compiler to create tons of per-polymorphic-class stuff that
        // we'll never need. We just use one function pointer, and the
        // Operations enum below to distinguish requests
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
        QAtomicInt m_ref = 1;
        typedef void (*ImplFn)(int which, QSlotObjectBase* this_, QObject *receiver, void **args, bool *ret);
        const ImplFn m_impl;
#else
        using ImplFn = void (*)(QSlotObjectBase* this_, QObject *receiver, void **args, int which, bool *ret);
        const ImplFn m_impl;
        QAtomicInt m_ref = 1;
#endif
    protected:
        // The operations that can be requested by calls to m_impl,
        // see the member functions that call m_impl below for details
        enum Operation {
            Destroy,
            Call,
            Compare,

            NumOperations
        };
    public:
        explicit QSlotObjectBase(ImplFn fn) : m_impl(fn) {}

        // A custom deleter compatible with std protocols (op()()) we well as
        // the legacy QScopedPointer protocol (cleanup()).
        struct Deleter {
            void operator()(QSlotObjectBase *p) const noexcept
            { if (p) p->destroyIfLastRef(); }
            // for the non-standard QScopedPointer protocol:
            static void cleanup(QSlotObjectBase *p) noexcept { Deleter{}(p); }
        };

        bool ref() noexcept { return m_ref.ref(); }
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
        inline void destroyIfLastRef() noexcept
        { if (!m_ref.deref()) m_impl(Destroy, this, nullptr, nullptr, nullptr); }

        inline bool compare(void **a) { bool ret = false; m_impl(Compare, this, nullptr, a, &ret); return ret; }
        inline void call(QObject *r, void **a)  { m_impl(Call, this, r, a, nullptr); }
#else
        inline void destroyIfLastRef() noexcept
        { if (!m_ref.deref()) m_impl(this, nullptr, nullptr, Destroy, nullptr); }

        inline bool compare(void **a)
        {
            bool ret = false;
            m_impl(this, nullptr, a, Compare, &ret);
            return ret;
        }
        inline void call(QObject *r, void **a)  { m_impl(this, r, a, Call, nullptr); }
#endif
        bool isImpl(ImplFn f) const { return m_impl == f; }
    protected:
        ~QSlotObjectBase() {}
    private:
        Q_DISABLE_COPY_MOVE(QSlotObjectBase)
    };

    using SlotObjUniquePtr = std::unique_ptr<QSlotObjectBase,
                                             QSlotObjectBase::Deleter>;
    inline SlotObjUniquePtr copy(const SlotObjUniquePtr &other) noexcept
    {
        if (other)
            other->ref();
        return SlotObjUniquePtr{other.get()};
    }

    class SlotObjSharedPtr {
        SlotObjUniquePtr obj;
    public:
        Q_NODISCARD_CTOR Q_IMPLICIT SlotObjSharedPtr() noexcept = default;
        Q_NODISCARD_CTOR Q_IMPLICIT SlotObjSharedPtr(std::nullptr_t) noexcept : SlotObjSharedPtr() {}
        Q_NODISCARD_CTOR explicit SlotObjSharedPtr(SlotObjUniquePtr o)
            : obj(std::move(o))
        {
            // does NOT ref() (takes unique_ptr by value)
            // (that's why (QSlotObjectBase*) ctor doesn't exisit: don't know whether that one _should_)
        }
        Q_NODISCARD_CTOR SlotObjSharedPtr(const SlotObjSharedPtr &other) noexcept
            : obj{copy(other.obj)} {}
        SlotObjSharedPtr &operator=(const SlotObjSharedPtr &other) noexcept
        { auto copy = other; swap(copy); return *this; }

        Q_NODISCARD_CTOR SlotObjSharedPtr(SlotObjSharedPtr &&other) noexcept = default;
        SlotObjSharedPtr &operator=(SlotObjSharedPtr &&other) noexcept = default;
        ~SlotObjSharedPtr() = default;

        void swap(SlotObjSharedPtr &other) noexcept { obj.swap(other.obj); }

        auto get() const noexcept { return obj.get(); }
        auto operator->() const noexcept { return get(); }

        explicit operator bool() const noexcept { return bool(obj); }
    };


    // Implementation of QSlotObjectBase for which the slot is a callable (function, PMF, functor, or lambda).
    // Args and R are the List of arguments and the return type of the signal to which the slot is connected.
    template <typename Func, typename Args, typename R>
    class QCallableObject : public QSlotObjectBase,
                            private QtPrivate::CompactStorage<std::decay_t<Func>>
    {
        using FunctorValue = std::decay_t<Func>;
        using Storage = QtPrivate::CompactStorage<FunctorValue>;
        using FuncType = Callable<Func, Args>;

#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
        Q_DECL_HIDDEN static void impl(int which, QSlotObjectBase *this_, QObject *r, void **a, bool *ret)
#else
        // Design note: the first three arguments match those for typical Call
        // and Destroy uses. We return void to enable tail call optimization
        // for those too.
        Q_DECL_HIDDEN static void impl(QSlotObjectBase *this_, QObject *r, void **a, int which, bool *ret)
#endif
        {
            const auto that = static_cast<QCallableObject*>(this_);
            switch (which) {
            case Destroy:
                delete that;
                break;
            case Call:
                if constexpr (std::is_member_function_pointer_v<FunctorValue>)
                    FuncType::template call<Args, R>(that->object(), static_cast<typename FuncType::Object *>(r), a);
                else
                    FuncType::template call<Args, R>(that->object(), r, a);
                break;
            case Compare:
                if constexpr (std::is_member_function_pointer_v<FunctorValue>) {
                    *ret = *reinterpret_cast<FunctorValue *>(a) == that->object();
                    break;
                }
                // not implemented otherwise
                Q_FALLTHROUGH();
            case NumOperations:
                Q_UNUSED(ret);
            }
        }
    public:
        explicit QCallableObject(Func &&f) : QSlotObjectBase(&impl), Storage{std::move(f)} {}
        explicit QCallableObject(const Func &f) : QSlotObjectBase(&impl), Storage{f} {}
    };

    // Helper to detect the context object type based on the functor type:
    // QObject for free functions and lambdas; the callee for member function
    // pointers. The default declaration doesn't have the ContextType typedef,
    // and so non-functor APIs (like old-style string-based slots) are removed
    // from the overload set.
    template <typename Func, typename = void>
    struct ContextTypeForFunctor {};

    template <typename Func>
    struct ContextTypeForFunctor<Func,
        std::enable_if_t<!std::disjunction_v<std::is_convertible<Func, const char *>,
                                             std::is_member_function_pointer<Func>
                                            >
                        >
    >
    {
        using ContextType = QObject;
    };
    template <typename Func>
    struct ContextTypeForFunctor<Func,
        std::enable_if_t<std::conjunction_v<std::negation<std::is_convertible<Func, const char *>>,
                                            std::is_member_function_pointer<Func>,
                                            std::is_convertible<typename QtPrivate::FunctionPointer<Func>::Object *, QObject *>
                                           >
                        >
    >
    {
        using ContextType = typename QtPrivate::FunctionPointer<Func>::Object;
    };

    /*
        Returns a suitable QSlotObjectBase object that holds \a func, if possible.

        Not available (and thus produces compile-time errors) if the Functor provided is
        not compatible with the expected Prototype.
    */
    template <typename Prototype, typename Functor>
    static constexpr std::enable_if_t<QtPrivate::countMatchingArguments<Prototype, Functor>() >= 0,
        QtPrivate::QSlotObjectBase *>
    makeCallableObject(Functor &&func)
    {
        using ExpectedSignature = QtPrivate::FunctionPointer<Prototype>;
        using ExpectedReturnType = typename ExpectedSignature::ReturnType;
        using ExpectedArguments = typename ExpectedSignature::Arguments;

        using ActualSignature = QtPrivate::FunctionPointer<Functor>;
        constexpr int MatchingArgumentCount = QtPrivate::countMatchingArguments<Prototype, Functor>();
        using ActualArguments  = typename QtPrivate::List_Left<ExpectedArguments, MatchingArgumentCount>::Value;

        static_assert(int(ActualSignature::ArgumentCount) <= int(ExpectedSignature::ArgumentCount),
            "Functor requires more arguments than what can be provided.");

        // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
        return new QtPrivate::QCallableObject<std::decay_t<Functor>, ActualArguments, ExpectedReturnType>(std::forward<Functor>(func));
    }

    template<typename Prototype, typename Functor, typename = void>
    struct AreFunctionsCompatible : std::false_type {};
    template<typename Prototype, typename Functor>
    struct AreFunctionsCompatible<Prototype, Functor, std::enable_if_t<
        std::is_same_v<decltype(QtPrivate::makeCallableObject<Prototype>(std::forward<Functor>(std::declval<Functor>()))),
        QtPrivate::QSlotObjectBase *>>
    > : std::true_type {};

    template<typename Prototype, typename Functor>
    inline constexpr bool AssertCompatibleFunctions() {
        static_assert(AreFunctionsCompatible<Prototype, Functor>::value,
                      "Functor is not compatible with expected prototype!");
        return true;
    }
} // namespace QtPrivate

QT_END_NAMESPACE

