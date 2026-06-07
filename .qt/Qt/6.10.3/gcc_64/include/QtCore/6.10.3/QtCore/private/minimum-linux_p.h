// Copyright (C) 2017 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef MINIMUMLINUX_P_H
#define MINIMUMLINUX_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

// EXTRA WARNING
// -------------
//
// This file must also be valid assembler source.
//

#include "private/qglobal_p.h"
#include <sys/stat.h>

QT_BEGIN_NAMESPACE

/* Minimum Linux kernel version:
 * We require the following features in Qt (unconditional, no fallback):
 *   Feature                    Added in version        Macro
 * - inotify_init1              before 2.6.12-rc12
 * - futex(2)                   before 2.6.12-rc12
 * - FUTEX_WAKE_OP              2.6.14                  FUTEX_OP
 * - linkat(2)                  2.6.17                  O_TMPFILE && QT_CONFIG(linkat)
 * - FUTEX_PRIVATE_FLAG         2.6.22
 * - O_CLOEXEC                  2.6.23
 * - eventfd                    2.6.23
 * - FUTEX_WAIT_BITSET          2.6.25
 * - pipe2 & dup3               2.6.27
 * - accept4                    2.6.28
 * - renameat2                  3.16                    QT_CONFIG(renameat2)
 * - getrandom                  3.17                    QT_CONFIG(getentropy)
 * - statx                      4.11                    STATX_BASIC_STATS
 */

#if defined(__GLIBC__) && defined(STATX_BASIC_STATS)
// if using glibc, the statx() function in sysdeps/unix/sysv/linux/statx.c
// falls back to stat() for us.
#  define QT_ELF_NOTE_OS_MAJOR      4
#  define QT_ELF_NOTE_OS_MINOR      11
#  define QT_ELF_NOTE_OS_PATCH      0
#elif QT_CONFIG(getentropy)
#  define QT_ELF_NOTE_OS_MAJOR      3
#  define QT_ELF_NOTE_OS_MINOR      17
#  define QT_ELF_NOTE_OS_PATCH      0
#elif QT_CONFIG(renameat2)
#  define QT_ELF_NOTE_OS_MAJOR      3
#  define QT_ELF_NOTE_OS_MINOR      16
#  define QT_ELF_NOTE_OS_PATCH      0
#else
#  define QT_ELF_NOTE_OS_MAJOR      2
#  define QT_ELF_NOTE_OS_MINOR      6
#  define QT_ELF_NOTE_OS_PATCH      28
#endif

/* you must include <elf.h> */
#define QT_ELF_NOTE_OS_TYPE         ELF_NOTE_OS_LINUX

QT_END_NAMESPACE

#endif // MINIMUMLINUX_P_H
