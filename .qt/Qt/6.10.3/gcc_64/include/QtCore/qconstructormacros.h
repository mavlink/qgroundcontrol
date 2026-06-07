// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCONSTRUCTORMACROS_H
#define QCONSTRUCTORMACROS_H

#if 0
#pragma qt_class(QtConstructorMacros)
#pragma qt_sync_stop_processing
#endif

#if defined(__cplusplus)

#ifndef Q_CONSTRUCTOR_FUNCTION
# define Q_CONSTRUCTOR_FUNCTION0(AFUNC) \
    namespace { \
    static const struct AFUNC ## _ctor_class_ { \
        inline AFUNC ## _ctor_class_() { AFUNC(); } \
    } AFUNC ## _ctor_instance_; \
    }

# define Q_CONSTRUCTOR_FUNCTION(AFUNC) Q_CONSTRUCTOR_FUNCTION0(AFUNC)
#endif

#ifndef Q_DESTRUCTOR_FUNCTION
# define Q_DESTRUCTOR_FUNCTION0(AFUNC) \
    namespace { \
    static const struct AFUNC ## _dtor_class_ { \
        inline AFUNC ## _dtor_class_() { } \
        inline ~ AFUNC ## _dtor_class_() { AFUNC(); } \
    } AFUNC ## _dtor_instance_; \
    }
# define Q_DESTRUCTOR_FUNCTION(AFUNC) Q_DESTRUCTOR_FUNCTION0(AFUNC)
#endif

#endif // __cplusplus

#endif // QCONSTRUCTORMACROS_H
