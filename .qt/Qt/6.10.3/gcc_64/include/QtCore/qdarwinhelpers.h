// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTDARWINHELPERS_H
#define QTDARWINHELPERS_H

#if 0
#pragma qt_class(QtDarwinHelpers)
#pragma qt_sync_stop_processing
#endif

/*
   Utility macros and inline functions
*/

#ifndef Q_FORWARD_DECLARE_OBJC_CLASS
#  ifdef __OBJC__
#    define Q_FORWARD_DECLARE_OBJC_CLASS(classname) @class classname
#  else
#    define Q_FORWARD_DECLARE_OBJC_CLASS(classname) class classname
#  endif
#endif
#ifndef Q_FORWARD_DECLARE_CF_TYPE
#  define Q_FORWARD_DECLARE_CF_TYPE(type) typedef const struct __ ## type * type ## Ref
#endif
#ifndef Q_FORWARD_DECLARE_MUTABLE_CF_TYPE
#  define Q_FORWARD_DECLARE_MUTABLE_CF_TYPE(type) typedef struct __ ## type * type ## Ref
#endif
#ifndef Q_FORWARD_DECLARE_CG_TYPE
#    define Q_FORWARD_DECLARE_CG_TYPE(type) typedef const struct type *type##Ref
#endif
#ifndef Q_FORWARD_DECLARE_MUTABLE_CG_TYPE
#    define Q_FORWARD_DECLARE_MUTABLE_CG_TYPE(type) typedef struct type *type##Ref
#endif


#endif // QTDARWINHELPERS_H
