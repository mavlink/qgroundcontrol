// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTTRANSLATION_H
#define QTTRANSLATION_H

#include <QtCore/qtconfigmacros.h> // QT_NO_TRANSLATION should be defined here as well
#include <QtCore/qtcoreexports.h>

#if 0
#pragma qt_class(QtTranslation)
#pragma qt_sync_stop_processing
#endif

QT_BEGIN_NAMESPACE

class QString;

#define QT_TR_NOOP(x) x
#define QT_TR_NOOP_UTF8(x) x
#define QT_TRANSLATE_NOOP(scope, x) x
#define QT_TRANSLATE_NOOP_UTF8(scope, x) x
#define QT_TRANSLATE_NOOP3(scope, x, comment) {x, comment}
#define QT_TRANSLATE_NOOP3_UTF8(scope, x, comment) {x, comment}

#ifndef QT_NO_TRANSLATION

#define QT_TR_N_NOOP(x) x
#define QT_TRANSLATE_N_NOOP(scope, x) x
#define QT_TRANSLATE_N_NOOP3(scope, x, comment) {x, comment}

// Defined in qcoreapplication.cpp
// The better name qTrId() is reserved for an upcoming function which would
// return a much more powerful QStringFormatter instead of a QString.
Q_CORE_EXPORT QString qtTrId(const char *id, int n = -1);

#define QT_TRID_NOOP(id) id
#define QT_TRID_N_NOOP(id) id

#endif // QT_NO_TRANSLATION

QT_END_NAMESPACE

#endif /* QTTRANSLATION_H */
