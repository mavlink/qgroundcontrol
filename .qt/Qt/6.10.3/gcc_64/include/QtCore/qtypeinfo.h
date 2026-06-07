// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTYPEINFO_H
#define QTYPEINFO_H

#include <QtCore/qcompilerdetection.h>
#include <QtCore/qcontainerfwd.h>

#include <type_traits>

QT_BEGIN_NAMESPACE

class QDebug;

/*
   QTypeInfo     - type trait functionality
*/

namespace QtPrivate {

// Helper for QTypeInfo<T>::isComplex, which used to be simply
// !std::is_trivial_v but P3247 deprecated it for C++26. It used to be defined
// (since C++11) by [class]/7 as: "A trivial class is a class that is trivially
// copyable and has one or more default constructors, all of which are either
// trivial or deleted and at least one of which is not deleted. [ Note: In
// particular, a trivially copyable or trivial class does not have virtual
// functions or virtual base classes. — end note ]".

template <typename T>
inline constexpr bool qIsComplex =
        !std::is_trivially_default_constructible_v<T> || !std::is_trivially_copyable_v<T>;

// A trivially copyable class must also have a trivial, non-deleted
// destructor [class.prop/1.3], CWG1734. Some implementations don't
// check for a trivial destructor, because of backwards compatibility
// with C++98's definition of trivial copyability.
// Since trivial copiability has implications for the ABI, implementations
// can't "just fix" their traits. So, although formally redundant, we
// explicitly check for trivial destruction here.
template <typename T>
inline constexpr bool qIsRelocatable =  std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>;

// Denotes types that are trivially default constructible, and for which
// value-initialization can be achieved by filling their storage with 0 bits.
// There is no type trait we can use for this, so we hardcode a list of
// possibilities that we know are OK on the architectures that we support.
// The most notable exception are pointers to data members, which for instance
// on the Itanium ABI are initialized to -1.
template <typename T>
inline constexpr bool qIsValueInitializationBitwiseZero =
        std::is_scalar_v<T> && !std::is_member_object_pointer_v<T>;

}

/*
  The catch-all template.
*/

template <typename T>
class QTypeInfo
{
public:
    enum {
        isPointer [[deprecated("Use std::is_pointer instead")]] = std::is_pointer_v<T>,
        isIntegral [[deprecated("Use std::is_integral instead")]] = std::is_integral_v<T>,
        isComplex = QtPrivate::qIsComplex<T>,
        isRelocatable = QtPrivate::qIsRelocatable<T>,
        isValueInitializationBitwiseZero = QtPrivate::qIsValueInitializationBitwiseZero<T>,
    };
};

template<>
class QTypeInfo<void>
{
public:
    enum {
        isPointer [[deprecated("Use std::is_pointer instead")]] = false,
        isIntegral [[deprecated("Use std::is_integral instead")]] = false,
        isComplex = false,
        isRelocatable = false,
        isValueInitializationBitwiseZero = false,
    };
};

/*!
    \class QTypeInfoMerger
    \inmodule QtCore
    \internal

    \brief QTypeInfoMerger merges the QTypeInfo flags of T1, T2... and presents them
    as a QTypeInfo<T> would do.

    Let's assume that we have a simple set of structs:

    \snippet code/src_corelib_global_qglobal.cpp 50

    To create a proper QTypeInfo specialization for A struct, we have to check
    all sub-components; B, C and D, then take the lowest common denominator and call
    Q_DECLARE_TYPEINFO with the resulting flags. An easier and less fragile approach is to
    use QTypeInfoMerger, which does that automatically. So struct A would have
    the following QTypeInfo definition:

    \snippet code/src_corelib_global_qglobal.cpp 51
*/
template <class T, class...Ts>
class QTypeInfoMerger
{
    static_assert(sizeof...(Ts) > 0);
public:
    static constexpr bool isComplex = ((QTypeInfo<Ts>::isComplex) || ...);
    static constexpr bool isRelocatable = ((QTypeInfo<Ts>::isRelocatable) && ...);
    [[deprecated("Use std::is_pointer instead")]] static constexpr bool isPointer = false;
    [[deprecated("Use std::is_integral instead")]] static constexpr bool isIntegral = false;
    static constexpr bool isValueInitializationBitwiseZero = false;
    static_assert(!isRelocatable ||
                  std::is_copy_constructible_v<T> ||
                  std::is_move_constructible_v<T>,
                  "All Ts... are Q_RELOCATABLE_TYPE, but T is neither copy- nor move-constructible, "
                  "so cannot be Q_RELOCATABLE_TYPE. Please mark T as Q_COMPLEX_TYPE manually.");
};

// QTypeInfo for std::pair:
//   std::pair is spec'ed to be struct { T1 first; T2 second; }, so, unlike tuple<>,
//   we _can_ specialize QTypeInfo for pair<>:
template <class T1, class T2>
class QTypeInfo<std::pair<T1, T2>> : public QTypeInfoMerger<std::pair<T1, T2>, T1, T2> {};

#define Q_DECLARE_MOVABLE_CONTAINER(CONTAINER) \
template <typename ...T> \
class QTypeInfo<CONTAINER<T...>> \
{ \
public: \
    enum { \
        isPointer [[deprecated("Use std::is_pointer instead")]] = false, \
        isIntegral [[deprecated("Use std::is_integral instead")]] = false, \
        isComplex = true, \
        isRelocatable = true, \
        isValueInitializationBitwiseZero = false, \
    }; \
}

Q_DECLARE_MOVABLE_CONTAINER(QList);
Q_DECLARE_MOVABLE_CONTAINER(QQueue);
Q_DECLARE_MOVABLE_CONTAINER(QStack);
Q_DECLARE_MOVABLE_CONTAINER(QSet);
Q_DECLARE_MOVABLE_CONTAINER(QMap);
Q_DECLARE_MOVABLE_CONTAINER(QMultiMap);
Q_DECLARE_MOVABLE_CONTAINER(QHash);
Q_DECLARE_MOVABLE_CONTAINER(QMultiHash);
Q_DECLARE_MOVABLE_CONTAINER(QCache);

#undef Q_DECLARE_MOVABLE_CONTAINER

/*
   Specialize a specific type with:

     Q_DECLARE_TYPEINFO(type, flags);

   where 'type' is the name of the type to specialize and 'flags' is
   logically-OR'ed combination of the flags below.
*/
enum { /* TYPEINFO flags */
    Q_COMPLEX_TYPE = 0,
    Q_PRIMITIVE_TYPE = 0x1,
    Q_RELOCATABLE_TYPE = 0x2,
    Q_MOVABLE_TYPE = 0x2,
    Q_DUMMY_TYPE = 0x4,
};

#define Q_DECLARE_TYPEINFO_BODY(TYPE, FLAGS) \
class QTypeInfo<TYPE > \
{ \
public: \
    enum { \
        isComplex = (((FLAGS) & Q_PRIMITIVE_TYPE) == 0) && QtPrivate::qIsComplex<TYPE>, \
        isRelocatable = !isComplex || ((FLAGS) & Q_RELOCATABLE_TYPE) || QtPrivate::qIsRelocatable<TYPE>, \
        isPointer [[deprecated("Use std::is_pointer instead")]] = std::is_pointer_v< TYPE >, \
        isIntegral [[deprecated("Use std::is_integral instead")]] = std::is_integral< TYPE >::value, \
        isValueInitializationBitwiseZero = QtPrivate::qIsValueInitializationBitwiseZero<TYPE>, \
    }; \
    static_assert(!QTypeInfo<TYPE>::isRelocatable || \
                  std::is_copy_constructible_v<TYPE > || \
                  std::is_move_constructible_v<TYPE >, \
                  #TYPE " is neither copy- nor move-constructible, so cannot be Q_RELOCATABLE_TYPE"); \
}

#define Q_DECLARE_TYPEINFO(TYPE, FLAGS) \
template<> \
Q_DECLARE_TYPEINFO_BODY(TYPE, FLAGS)

/* Specialize QTypeInfo for QFlags<T> */
template<typename T> class QFlags;
template<typename T>
Q_DECLARE_TYPEINFO_BODY(QFlags<T>, Q_PRIMITIVE_TYPE);

QT_END_NAMESPACE
#endif // QTYPEINFO_H
