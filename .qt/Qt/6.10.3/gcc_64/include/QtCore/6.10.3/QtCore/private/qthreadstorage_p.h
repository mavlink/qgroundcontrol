// Copyright (C) 2025 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTHREADSTORAGE_P_H
#define QTHREADSTORAGE_P_H

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

#include <qlist.h>
#include <private/qglobal_p.h>

QT_BEGIN_NAMESPACE

namespace QThreadStoragePrivate {
void init();
void finish(QList<void *> *tls);
} // namespace QThreadStoragePrivate

QT_END_NAMESPACE

#endif // QTHREADSTORAGE_P_H
