// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef Q_VXWORKS_PLATFORMDEFS_H
#define Q_VXWORKS_PLATFORMDEFS_H

#include "qglobal.h"

#include <unistd.h>
#include <fcntl.h>

#include <sys/time.h>
#include <dirent.h>
#include <pthread.h>

// from VxWorks 7 <system.h>
#ifndef S_ISSOCK
# ifdef S_IFSOCK
#   define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)
# else
#   define S_ISSOCK(m) 0
# endif
#endif

#include "../common/posix/qplatformdefs.h"

#undef QT_OPEN_LARGEFILE

#define O_LARGEFILE         0
#define QT_OPEN_LARGEFILE   O_LARGEFILE

#define QT_MMAP                 ::mmap

#define QT_SNPRINTF             ::snprintf
#define QT_VSNPRINTF            ::vsnprintf

#define S_ISVTX 01000

/*
 * vxworks introduces some defines that replace READ with 0 and WRITE with 1
 * this causes issues with moc and Q_PROPERTY
 */
#undef READ
#undef WRITE

/* vxworks exposes these definitions only when _POSIX_C_SOURCE >=200809L but we don't want to set this, as it hides other API */
#ifndef UTIME_NOW
#  define UTIME_NOW       ((1l << 30) - 1l)
#  define UTIME_OMIT      ((1l << 30) - 2l)
#endif

#endif /* Q_VXWORKS_PLATFORMDEFS_H */
