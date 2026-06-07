// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QLOGGINGCATEGORY_H
#define QLOGGINGCATEGORY_H

#include <QtCore/qglobal.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QLoggingCategory
{
    Q_DISABLE_COPY(QLoggingCategory)
public:
    explicit QLoggingCategory(const char *category, QtMsgType severityLevel = QtDebugMsg);
    ~QLoggingCategory();

    bool isEnabled(QtMsgType type) const;
    void setEnabled(QtMsgType type, bool enable);

    bool isDebugEnabled() const { return bools.enabledDebug.loadRelaxed(); }
    bool isInfoEnabled() const { return bools.enabledInfo.loadRelaxed(); }
    bool isWarningEnabled() const { return bools.enabledWarning.loadRelaxed(); }
    bool isCriticalEnabled() const { return bools.enabledCritical.loadRelaxed(); }

    const char *categoryName() const { return name; }

    // allows usage of both factory method and variable in qCX macros
    QLoggingCategory &operator()() { return *this; }
    const QLoggingCategory &operator()() const { return *this; }

    static QLoggingCategory *defaultCategory();

    typedef void (*CategoryFilter)(QLoggingCategory*);
    static CategoryFilter installFilter(CategoryFilter);

    static void setFilterRules(const QString &rules);

private:
    Q_DECL_UNUSED_MEMBER void *d = nullptr; // reserved for future use
    const char *name = nullptr;

    struct AtomicBools {
        QBasicAtomicInteger<bool> enabledDebug;
        QBasicAtomicInteger<bool> enabledWarning;
        QBasicAtomicInteger<bool> enabledCritical;
        QBasicAtomicInteger<bool> enabledInfo;
    };
    union {
        AtomicBools bools;
        QBasicAtomicInt enabled;
    };
    Q_DECL_UNUSED_MEMBER bool placeholder[4]; // reserved for future use

    QT_DEFINE_TAG_STRUCT(UnregisteredInitialization);
    explicit constexpr QLoggingCategory(UnregisteredInitialization, const char *category) noexcept;
    friend class QLoggingRegistry;
};

namespace { // allow different TUs to have different QT_NO_xxx_OUTPUT
template <QtMsgType Which> struct QLoggingCategoryMacroHolder
{
    static const bool IsOutputEnabled;
    const QLoggingCategory *category = nullptr;
    bool control = false;
    explicit QLoggingCategoryMacroHolder(const QLoggingCategory &cat)
    {
        if (IsOutputEnabled)
            init(cat);
    }
    void init(const QLoggingCategory &cat) noexcept
    {
        category = &cat;
        // same as:
        //  control = cat.isEnabled(Which);
        // but without an out-of-line call
        if constexpr (Which == QtDebugMsg) {
            control = cat.isDebugEnabled();
        } else if constexpr (Which == QtInfoMsg) {
            control = cat.isInfoEnabled();
        } else if constexpr (Which == QtWarningMsg) {
            control = cat.isWarningEnabled();
        } else if constexpr (Which == QtCriticalMsg) {
            control = cat.isCriticalEnabled();
        } else if constexpr (Which == QtFatalMsg) {
            control = true;
        } else {
            static_assert(QtPrivate::value_dependent_false<Which>(), "Unknown Qt message type");
        }
    }
    const char *name() const { return category->categoryName(); }
    explicit operator bool() const { return Q_UNLIKELY(control); }
};

template <QtMsgType Which> const bool QLoggingCategoryMacroHolder<Which>::IsOutputEnabled = true;
#if defined(QT_NO_DEBUG_OUTPUT)
template <> const bool QLoggingCategoryMacroHolder<QtDebugMsg>::IsOutputEnabled = false;
#endif
#if defined(QT_NO_INFO_OUTPUT)
template <> const bool QLoggingCategoryMacroHolder<QtInfoMsg>::IsOutputEnabled = false;
#endif
#if defined(QT_NO_WARNING_OUTPUT)
template <> const bool QLoggingCategoryMacroHolder<QtWarningMsg>::IsOutputEnabled = false;
#endif
} // unnamed namespace

#define QT_DECLARE_EXPORTED_QT_LOGGING_CATEGORY(name, export_macro) \
    inline namespace QtPrivateLogging { export_macro const QLoggingCategory &name(); }

#ifdef QT_BUILDING_QT
#define Q_DECLARE_LOGGING_CATEGORY(name) \
    inline namespace QtPrivateLogging { const QLoggingCategory &name(); }

#define Q_DECLARE_EXPORTED_LOGGING_CATEGORY(name, export_macro) \
    inline namespace QtPrivateLogging { \
    Q_DECL_DEPRECATED_X("Use QT_DECLARE_EXPORTED_QT_LOGGING_CATEGORY in Qt") \
    export_macro const QLoggingCategory &name(); \
    }

#define Q_LOGGING_CATEGORY_IMPL(name, ...) \
    const QLoggingCategory &name() \
    { \
        static const QLoggingCategory category(__VA_ARGS__); \
        return category; \
    }

#define Q_LOGGING_CATEGORY(name, ...) \
    inline namespace QtPrivateLogging { Q_LOGGING_CATEGORY_IMPL(name, __VA_ARGS__) } \
    Q_WEAK_OVERLOAD \
    Q_DECL_DEPRECATED_X("Use Q_STATIC_LOGGING_CATEGORY or add " \
                        "either Q_DECLARE_LOGGING_CATEGORY or " \
                        "QT_DECLARE_EXPORTED_QT_LOGGING_CATEGORY in a header") \
    const QLoggingCategory &name() { return QtPrivateLogging::name(); }

#define Q_STATIC_LOGGING_CATEGORY(name, ...) \
    static Q_LOGGING_CATEGORY_IMPL(name, __VA_ARGS__)

#else
#define Q_DECLARE_LOGGING_CATEGORY(name) \
    const QLoggingCategory &name();

#define Q_DECLARE_EXPORTED_LOGGING_CATEGORY(name, export_macro) \
    export_macro Q_DECLARE_LOGGING_CATEGORY(name)

#define Q_LOGGING_CATEGORY(name, ...) \
    const QLoggingCategory &name() \
    { \
        static const QLoggingCategory category(__VA_ARGS__); \
        return category; \
    }

#define Q_STATIC_LOGGING_CATEGORY(name, ...) \
    static Q_LOGGING_CATEGORY(name, __VA_ARGS__)
#endif

#define QT_MESSAGE_LOGGER_COMMON(category, level) \
    for (QLoggingCategoryMacroHolder<level> qt_category((category)()); qt_category; qt_category.control = false) \
        QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC, qt_category.name())

#define qCDebug(category, ...) QT_MESSAGE_LOGGER_COMMON(category, QtDebugMsg).debug(__VA_ARGS__)
#define qCInfo(category, ...) QT_MESSAGE_LOGGER_COMMON(category, QtInfoMsg).info(__VA_ARGS__)
#define qCWarning(category, ...) QT_MESSAGE_LOGGER_COMMON(category, QtWarningMsg).warning(__VA_ARGS__)
#define qCCritical(category, ...) QT_MESSAGE_LOGGER_COMMON(category, QtCriticalMsg).critical(__VA_ARGS__)
#define qCFatal(category, ...) QT_MESSAGE_LOGGER_COMMON(category, QtFatalMsg).fatal(__VA_ARGS__)

QT_END_NAMESPACE

#endif // QLOGGINGCATEGORY_H
