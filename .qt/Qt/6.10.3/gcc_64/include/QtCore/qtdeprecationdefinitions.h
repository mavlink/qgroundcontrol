// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTDEPRECATIONDEFINITIONS_H
#define QTDEPRECATIONDEFINITIONS_H

#include <QtCore/qcompilerdetection.h>

#ifndef QT_DISABLE_DEPRECATED_UP_TO
#  ifdef QT_DISABLE_DEPRECATED_BEFORE // If the deprecated macro is defined, use its value
#    define QT_DISABLE_DEPRECATED_UP_TO QT_DISABLE_DEPRECATED_BEFORE
#  else
#    define QT_DISABLE_DEPRECATED_UP_TO 0x050000
#  endif
#endif

#if QT_DISABLE_DEPRECATED_UP_TO < 0x050000 && defined(Q_CC_GNU)
#  warning QT_DISABLE_DEPRECATED_UP_TO is set to the version that is lower than the version that \
           Qt was built with. This may lead to linking issues.
#endif

#ifndef QT_WARN_DEPRECATED_UP_TO
#  ifdef QT_DEPRECATED_WARNINGS_SINCE // If the deprecated macro is defined, use its value
#    define QT_WARN_DEPRECATED_UP_TO QT_DEPRECATED_WARNINGS_SINCE
#  else
#    define QT_WARN_DEPRECATED_UP_TO 0x070000
#  endif
#endif

#endif // QTDEPRECATIONDEFINITIONS_H
