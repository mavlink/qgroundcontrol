// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2019 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGLOBAL_H
#define QGLOBAL_H

#if 0
#pragma qt_class(QtGlobal)
#endif

#ifdef __cplusplus
#  include <type_traits>
#  include <cstddef>
#  include <utility>
#  include <cstdint>
#endif
#ifndef __ASSEMBLER__
#  include <assert.h>
#  include <stdbool.h>
#  include <stddef.h>
#endif

#include <QtCore/qtcoreglobal.h>

#include <QtCore/qtpreprocessorsupport.h>

#include <QtCore/qsystemdetection.h>
#include <QtCore/qprocessordetection.h>
#include <QtCore/qcompilerdetection.h>

#ifndef __ASSEMBLER__
#  include <QtCore/qassert.h>
#  include <QtCore/qtnoop.h>
#  include <QtCore/qtypes.h>
#endif /* !__ASSEMBLER__ */
#include <QtCore/qtversion.h>

#if defined(__cplusplus)

#include <QtCore/qtclasshelpermacros.h>

// We need to keep QTypeInfo, QSysInfo, QFlags, qDebug & family in qglobal.h for compatibility with Qt 4.
// Be careful when changing the order of these files.
#include <QtCore/qtypeinfo.h>
#include <QtCore/qsysinfo.h>
#include <QtCore/qlogging.h>

#include <QtCore/qflags.h>

#include <QtCore/qatomic.h>
#include <QtCore/qconstructormacros.h>
#include <QtCore/qdarwinhelpers.h>
#include <QtCore/qexceptionhandling.h>
#include <QtCore/qforeach.h>
#include <QtCore/qfunctionpointer.h>
#include <QtCore/qglobalstatic.h>
#include <QtCore/qmalloc.h>
#include <QtCore/qminmax.h>
#include <QtCore/qnumeric.h>
#include <QtCore/qoverload.h>
#include <QtCore/qswap.h>
#include <QtCore/qtdeprecationmarkers.h>
#include <QtCore/qtenvironmentvariables.h>
#include <QtCore/qtresource.h>
#include <QtCore/qttranslation.h>
#include <QtCore/qttypetraits.h>
#if QT_CONFIG(version_tagging)
#include <QtCore/qversiontagging.h>
#endif
#endif /* __cplusplus */

#endif /* QGLOBAL_H */
