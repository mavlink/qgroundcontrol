// Copyright (C) 2019 The Qt Company Ltd.
// Copyright (C) 2019 Olivier Goffart <ogoffart@woboq.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTMETAMACROS_H
#define QTMETAMACROS_H

#include <QtCore/qglobal.h>
#include <QtCore/qtclasshelpermacros.h>

QT_BEGIN_NAMESPACE

#ifndef Q_MOC_OUTPUT_REVISION
// This number should be in sync with moc's outputrevision.h
#define Q_MOC_OUTPUT_REVISION 69
#endif

// The following macros can be defined by tools that understand Qt
// to have the information from the macro.
#ifndef QT_ANNOTATE_CLASS
# define QT_ANNOTATE_CLASS(type, ...)
#endif
#ifndef QT_ANNOTATE_CLASS2
# define QT_ANNOTATE_CLASS2(type, a1, a2)
#endif
#ifndef QT_ANNOTATE_FUNCTION
# define QT_ANNOTATE_FUNCTION(x)
#endif
#ifndef QT_ANNOTATE_ACCESS_SPECIFIER
# define QT_ANNOTATE_ACCESS_SPECIFIER(x)
#endif

// The following macros are our "extensions" to C++
// They are used, strictly speaking, only by the moc.

#ifndef Q_MOC_RUN
#ifndef QT_NO_META_MACROS
# if defined(QT_NO_KEYWORDS)
#  define QT_NO_EMIT
# else
#   ifndef QT_NO_SIGNALS_SLOTS_KEYWORDS
#     define slots Q_SLOTS
#     define signals Q_SIGNALS
#   endif
# endif
# define Q_SLOTS QT_ANNOTATE_ACCESS_SPECIFIER(qt_slot)
# define Q_SIGNALS public QT_ANNOTATE_ACCESS_SPECIFIER(qt_signal)
# define Q_PRIVATE_SLOT(d, signature) QT_ANNOTATE_CLASS2(qt_private_slot, d, signature)
# define Q_EMIT
#ifndef QT_NO_EMIT
# define emit
#endif
#ifndef Q_CLASSINFO
# define Q_CLASSINFO(name, value)
#endif
#define Q_PLUGIN_METADATA(x) QT_ANNOTATE_CLASS(qt_plugin_metadata, x)
#define Q_INTERFACES(x) QT_ANNOTATE_CLASS(qt_interfaces, x)
#define Q_PROPERTY(...) QT_ANNOTATE_CLASS(qt_property, __VA_ARGS__)
#define Q_PRIVATE_PROPERTY(d, text) QT_ANNOTATE_CLASS2(qt_private_property, d, text)
#ifndef Q_REVISION
# define Q_REVISION(...)
#endif
#define Q_OVERRIDE(text) QT_ANNOTATE_CLASS(qt_override, text)
#define QDOC_PROPERTY(text) QT_ANNOTATE_CLASS(qt_qdoc_property, text)
#define Q_ENUMS(x) QT_ANNOTATE_CLASS(qt_enums, x)
#define Q_FLAGS(x) QT_ANNOTATE_CLASS(qt_enums, x)
#define Q_ENUM_IMPL(ENUM) \
    friend constexpr const QMetaObject *qt_getEnumMetaObject(ENUM) noexcept { return &staticMetaObject; } \
    friend constexpr const char *qt_getEnumName(ENUM) noexcept { return #ENUM; }
#define Q_ENUM(x) QT_ANNOTATE_CLASS(qt_enums, x) Q_ENUM_IMPL(x)
#define Q_FLAG(x) QT_ANNOTATE_CLASS(qt_enums, x) Q_ENUM_IMPL(x)
#define Q_ENUM_NS_IMPL(ENUM) \
    inline constexpr const QMetaObject *qt_getEnumMetaObject(ENUM) noexcept { return &staticMetaObject; } \
    inline constexpr const char *qt_getEnumName(ENUM) noexcept { return #ENUM; }
#define Q_ENUM_NS(x) QT_ANNOTATE_CLASS(qt_enums, x) Q_ENUM_NS_IMPL(x)
#define Q_FLAG_NS(x) QT_ANNOTATE_CLASS(qt_enums, x) Q_ENUM_NS_IMPL(x)
#define Q_SCRIPTABLE QT_ANNOTATE_FUNCTION(qt_scriptable)
#define Q_INVOKABLE  QT_ANNOTATE_FUNCTION(qt_invokable)
#define Q_SIGNAL QT_ANNOTATE_FUNCTION(qt_signal)
#define Q_SLOT QT_ANNOTATE_FUNCTION(qt_slot)
#define Q_MOC_INCLUDE(...) QT_ANNOTATE_CLASS(qt_moc_include, __VA_ARGS__)
#endif // QT_NO_META_MACROS

#ifndef QT_NO_TRANSLATION
// full set of tr functions
#  define QT_TR_FUNCTIONS \
    static inline QString tr(const char *s, const char *c = nullptr, int n = -1) \
        { return staticMetaObject.tr(s, c, n); }
#else
// inherit the ones from QObject
# define QT_TR_FUNCTIONS
#endif

#ifdef Q_QDOC
#define QT_TR_FUNCTIONS
#endif

#if defined(Q_CC_CLANG)
#  if Q_CC_CLANG >= 1100
#    define Q_OBJECT_NO_OVERRIDE_WARNING    QT_WARNING_DISABLE_CLANG("-Winconsistent-missing-override") QT_WARNING_DISABLE_CLANG("-Wsuggest-override")
#  elif Q_CC_CLANG >= 306
#    define Q_OBJECT_NO_OVERRIDE_WARNING    QT_WARNING_DISABLE_CLANG("-Winconsistent-missing-override")
#  endif
#elif defined(Q_CC_GNU) && Q_CC_GNU >= 501
#  define Q_OBJECT_NO_OVERRIDE_WARNING      QT_WARNING_DISABLE_GCC("-Wsuggest-override")
#elif defined(Q_CC_MSVC)
#  define Q_OBJECT_NO_OVERRIDE_WARNING      QT_WARNING_DISABLE_MSVC(26433)
#else
#  define Q_OBJECT_NO_OVERRIDE_WARNING
#endif

#if defined(Q_CC_GNU) && Q_CC_GNU >= 600
#  define Q_OBJECT_NO_ATTRIBUTES_WARNING    QT_WARNING_DISABLE_GCC("-Wattributes")
#else
#  define Q_OBJECT_NO_ATTRIBUTES_WARNING
#endif

#define QT_META_OBJECT_VARS \
    template <typename> static constexpr auto qt_create_metaobjectdata();       \
    template <typename MetaObjectTagType> static constexpr inline auto          \
    qt_staticMetaObjectContent = qt_create_metaobjectdata<MetaObjectTagType>(); \
    template <typename MetaObjectTagType> static constexpr inline auto          \
    qt_staticMetaObjectStaticContent = qt_staticMetaObjectContent<MetaObjectTagType>.staticData;\
    template <typename MetaObjectTagType> static constexpr inline auto          \
    qt_staticMetaObjectRelocatingContent = qt_staticMetaObjectContent<MetaObjectTagType>.relocatingData;

#define QT_OBJECT_GADGET_COMMON  \
    QT_META_OBJECT_VARS \
    Q_OBJECT_NO_ATTRIBUTES_WARNING \
    Q_DECL_HIDDEN static void qt_static_metacall(QObject *, QMetaObject::Call, int, void **);

/* qmake ignore Q_OBJECT */
#define Q_OBJECT \
public: \
    QT_WARNING_PUSH \
    Q_OBJECT_NO_OVERRIDE_WARNING \
    static const QMetaObject staticMetaObject; \
    virtual const QMetaObject *metaObject() const; \
    virtual void *qt_metacast(const char *); \
    virtual int qt_metacall(QMetaObject::Call, int, void **); \
    QT_TR_FUNCTIONS \
private: \
    QT_OBJECT_GADGET_COMMON \
    QT_DEFINE_TAG_STRUCT(QPrivateSignal); \
    QT_WARNING_POP \
    QT_ANNOTATE_CLASS(qt_qobject, "")

/* qmake ignore Q_OBJECT */
#define Q_OBJECT_FAKE Q_OBJECT QT_ANNOTATE_CLASS(qt_fake, "")

#ifndef QT_NO_META_MACROS
/* qmake ignore Q_GADGET_EXPORT */
#define Q_GADGET_EXPORT(...) \
public: \
    static __VA_ARGS__ const QMetaObject staticMetaObject; \
    void qt_check_for_QGADGET_macro(); \
    typedef void QtGadgetHelper; \
private: \
    QT_WARNING_PUSH \
    QT_OBJECT_GADGET_COMMON \
    QT_WARNING_POP \
    QT_ANNOTATE_CLASS(qt_qgadget, "") \
    /*end*/

/* qmake ignore Q_GADGET */
#define Q_GADGET Q_GADGET_EXPORT()

    /* qmake ignore Q_NAMESPACE_EXPORT */
#define Q_NAMESPACE_EXPORT(...) \
    extern __VA_ARGS__ const QMetaObject staticMetaObject; \
    template <typename> static constexpr auto qt_create_metaobjectdata(); \
    QT_ANNOTATE_CLASS(qt_qnamespace, "") \
    /*end*/

/* qmake ignore Q_NAMESPACE */
#define Q_NAMESPACE Q_NAMESPACE_EXPORT() \
    /*end*/

#endif // QT_NO_META_MACROS

#else // Q_MOC_RUN
#define slots slots
#define signals signals
#define Q_SLOTS Q_SLOTS
#define Q_SIGNALS Q_SIGNALS
#define Q_CLASSINFO(name, value) Q_CLASSINFO(name, value)
#define Q_INTERFACES(x) Q_INTERFACES(x)
#define Q_PROPERTY(text) Q_PROPERTY(text)
#define Q_PRIVATE_PROPERTY(d, text) Q_PRIVATE_PROPERTY(d, text)
#define Q_REVISION(...) Q_REVISION(__VA_ARGS__)
#define Q_OVERRIDE(text) Q_OVERRIDE(text)
#define Q_ENUMS(x) Q_ENUMS(x)
#define Q_FLAGS(x) Q_FLAGS(x)
#define Q_ENUM(x) Q_ENUM(x)
#define Q_FLAG(x) Q_FLAG(x)
#define Q_ENUM_NS(x) Q_ENUM_NS(x)
#define Q_FLAG_NS(x) Q_FLAG_NS(x)
 /* qmake ignore Q_OBJECT */
#define Q_OBJECT Q_OBJECT
 /* qmake ignore Q_OBJECT */
#define Q_OBJECT_FAKE Q_OBJECT_FAKE
 /* qmake ignore Q_GADGET */
#define Q_GADGET Q_GADGET
#define Q_SCRIPTABLE Q_SCRIPTABLE
#define Q_INVOKABLE Q_INVOKABLE
#define Q_SIGNAL Q_SIGNAL
#define Q_SLOT Q_SLOT
#endif //Q_MOC_RUN

QT_END_NAMESPACE

#endif // QTMETAMACROS_H
