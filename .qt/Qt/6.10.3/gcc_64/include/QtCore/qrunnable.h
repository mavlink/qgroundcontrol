// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QRUNNABLE_H
#define QRUNNABLE_H

#include <QtCore/qcompilerdetection.h>
#include <QtCore/qfunctionaltools_impl.h>
#include <QtCore/qtclasshelpermacros.h>
#include <QtCore/qtcoreexports.h>

#include <functional>
#include <type_traits>

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QRunnable
{
    bool m_autoDelete = true;

    Q_DISABLE_COPY(QRunnable)
public:
    virtual void run() = 0;

    constexpr QRunnable() noexcept = default;
    virtual ~QRunnable();
#if QT_CORE_REMOVED_SINCE(6, 6)
    static QRunnable *create(std::function<void()> functionToRun);
#endif
    template <typename Callable>
    using if_callable = std::enable_if_t<std::is_invocable_r_v<void, Callable>, bool>;

    template <typename Callable, if_callable<Callable> = true>
    static QRunnable *create(Callable &&functionToRun);
    static QRunnable *create(std::nullptr_t) = delete;

    bool autoDelete() const { return m_autoDelete; }
    void setAutoDelete(bool autoDelete) { m_autoDelete = autoDelete; }

private:
    static Q_DECL_COLD_FUNCTION QRunnable *warnNullCallable();
    class QGenericRunnable;
};

class Q_CORE_EXPORT QRunnable::QGenericRunnable : public QRunnable
{
    // Type erasure, to only instantiate a non-virtual class per Callable:
    class HelperBase
    {
    protected:
        enum class Op {
            Run,
            Destroy,
        };
        using OpFn = void* (*)(Op, HelperBase *, void*);
        OpFn fn;
    protected:
        constexpr explicit HelperBase(OpFn f) noexcept : fn(f) {}
        ~HelperBase() = default;
    public:
        void run() { fn(Op::Run, this, nullptr); }
        void destroy() { fn(Op::Destroy, this, nullptr); }
    };

    template <typename Callable>
    class Helper : public HelperBase, private QtPrivate::CompactStorage<Callable>
    {
        using Storage = QtPrivate::CompactStorage<Callable>;
        static void *impl(Op op, HelperBase *that, [[maybe_unused]] void *arg)
        {
            const auto _this = static_cast<Helper*>(that);
            switch (op) {
            case Op::Run:     _this->object()(); break;
            case Op::Destroy: delete _this; break;
            }
            return nullptr;
        }
    public:
        template <typename UniCallable>
        explicit Helper(UniCallable &&functionToRun) noexcept
            : HelperBase(&impl),
              Storage{std::forward<UniCallable>(functionToRun)}
        {
        }
    };

    HelperBase *runHelper;
public:
    template <typename Callable, if_callable<Callable> = true>
    explicit QGenericRunnable(Callable &&c)
        : runHelper(new Helper<std::decay_t<Callable>>(std::forward<Callable>(c)))
    {
    }
    ~QGenericRunnable() override;

    void run() override;
};

namespace QtPrivate {

template <typename T>
constexpr inline bool is_function_pointer_v = std::conjunction_v<
        std::is_pointer<T>,
        std::is_function<std::remove_pointer_t<T>>
    >;
template <typename T>
constexpr inline bool is_std_function_v = false;
template <typename T>
constexpr inline bool is_std_function_v<std::function<T>> = true;

} // namespace QtPrivate

template <typename Callable, QRunnable::if_callable<Callable>>
QRunnable *QRunnable::create(Callable &&functionToRun)
{
    using F = std::decay_t<Callable>;
    constexpr bool is_std_function = QtPrivate::is_std_function_v<F>;
    constexpr bool is_function_pointer = QtPrivate::is_function_pointer_v<F>;
    if constexpr (is_std_function || is_function_pointer) {
        bool is_null;
        if constexpr (is_std_function) {
            is_null = !functionToRun;
        } else if constexpr (is_function_pointer) {
            // shut up warnings about functions always having a non-null address:
            const void *functionPtr = reinterpret_cast<void *>(functionToRun);
            is_null = !functionPtr;
        }
        if (is_null)
            return warnNullCallable();
    }

    return new QGenericRunnable(std::forward<Callable>(functionToRun));
}

QT_END_NAMESPACE

#endif
