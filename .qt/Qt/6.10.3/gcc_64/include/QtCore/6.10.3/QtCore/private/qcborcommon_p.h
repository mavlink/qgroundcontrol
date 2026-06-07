// Copyright (C) 2018 Intel Corporation.
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#ifndef QCBORCOMMON_P_H
#define QCBORCOMMON_P_H

#include "qcborcommon.h"
#include "private/qglobal_p.h"

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

QT_BEGIN_NAMESPACE

#ifdef QT_NO_DEBUG
#  define NDEBUG 1
#endif
#undef assert
#define assert Q_ASSERT

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wunused-function")
QT_WARNING_DISABLE_CLANG("-Wunused-function")
QT_WARNING_DISABLE_CLANG("-Wundefined-internal")

#define CBOR_NO_HALF_FLOAT_TYPE 1
#define CBOR_NO_VALIDATION_API  1
#define CBOR_NO_PRETTY_API      1

#include <cbor.h>

QT_WARNING_POP

Q_DECLARE_TYPEINFO(CborValue, Q_PRIMITIVE_TYPE);

QT_END_NAMESPACE

#endif // QCBORCOMMON_P_H
