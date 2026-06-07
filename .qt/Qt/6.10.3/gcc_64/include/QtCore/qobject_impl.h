// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef Q_QDOC

#ifndef QOBJECT_H
#error Do not include qobject_impl.h directly
#endif

#if 0
#pragma qt_sync_skip_header_check
#pragma qt_sync_stop_processing
#endif

QT_BEGIN_NAMESPACE


namespace QtPrivate {
    /*
        Logic to statically generate the array of qMetaTypeId
        ConnectionTypes<FunctionPointer<Signal>::Arguments>::types() returns an array
        of int that is suitable for the types arguments of the connection functions.

        The array only exist of all the types are declared as a metatype
        (detected using the TypesAreDeclaredMetaType helper struct)
        If one of the type is not declared, the function return 0 and the signal
        cannot be used in queued connection.
    */
    template <typename ArgList> struct TypesAreDeclaredMetaType { enum { Value = false }; };
    template <> struct TypesAreDeclaredMetaType<List<>> { enum { Value = true }; };
    template <typename Arg, typename... Tail> struct TypesAreDeclaredMetaType<List<Arg, Tail...> >
    { enum { Value = QMetaTypeId2<Arg>::Defined && TypesAreDeclaredMetaType<List<Tail...>>::Value }; };

    template <typename ArgList, bool Declared = TypesAreDeclaredMetaType<ArgList>::Value > struct ConnectionTypes
    { static const int *types() { return nullptr; } };
    template <> struct ConnectionTypes<List<>, true>
    { static const int *types() { return nullptr; } };
    template <typename... Args> struct ConnectionTypes<List<Args...>, true>
    { static const int *types() { static const int t[sizeof...(Args) + 1] = { (QtPrivate::QMetaTypeIdHelper<Args>::qt_metatype_id())..., 0 }; return t; } };
}


QT_END_NAMESPACE

#endif
