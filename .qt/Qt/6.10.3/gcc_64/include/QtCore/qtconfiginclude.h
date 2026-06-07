// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTCONFIGINCLUDE_H
#define QTCONFIGINCLUDE_H

#if 0
#  pragma qt_sync_stop_processing
#endif

#ifdef __cplusplus
# if __has_include(<version>) /* remove this check once Integrity, QNX have caught up */
#  include <version>
# endif
#endif

#include <QtCore/qconfig.h>

#ifdef QT_BOOTSTRAPPED
// qconfig-bootstrapped.h is not supposed to be a part of the synced header files. So we find it by
// the include path specified for Bootstrap library in the source tree instead of the build tree as
// it's done for regular header files.
#include "qconfig-bootstrapped.h"
#else
#include <QtCore/qtcore-config.h>
#endif

#endif // QTCONFIGINCLUDE_H
