// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGENERICUNIXFONTDATABASE_H
#define QGENERICUNIXFONTDATABASE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>

#if QT_CONFIG(fontconfig)
#include <QtGui/private/qfontconfigdatabase_p.h>
using QGenericUnixFontDatabase = QFontconfigDatabase;
#elif QT_CONFIG(freetype)
#include <QtGui/private/qfreetypefontdatabase_p.h>
using QGenericUnixFontDatabase = QFreeTypeFontDatabase;
#else
#include <qpa/qplatformfontdatabase.h>
using QGenericUnixFontDatabase = QPlatformFontDatabase;
#endif

#endif // QGENERICUNIXFONTDATABASE_H
