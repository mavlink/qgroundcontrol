// Copyright (C) 2024 Loongson Technology Corporation Limited.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDRAWHELPER_LOONGARCH64_P_H
#define QDRAWHELPER_LOONGARCH64_P_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include <private/qdrawhelper_p.h>

QT_BEGIN_NAMESPACE

#ifdef QT_COMPILER_SUPPORTS_LSX
void qt_memfill64_lsx(quint64 *dest, quint64 value, qsizetype count);
void qt_memfill32_lsx(quint32 *dest, quint32 value, qsizetype count);
void qt_bitmapblit32_lsx(QRasterBuffer *rasterBuffer, int x, int y,
                         const QRgba64 &color,
                         const uchar *src, int width, int height, int stride);
void qt_bitmapblit8888_lsx(QRasterBuffer *rasterBuffer, int x, int y,
                           const QRgba64 &color,
                           const uchar *src, int width, int height, int stride);
void qt_bitmapblit16_lsx(QRasterBuffer *rasterBuffer, int x, int y,
                         const QRgba64 &color,
                         const uchar *src, int width, int height, int stride);
void qt_blend_argb32_on_argb32_lsx(uchar *destPixels, int dbpl,
                                   const uchar *srcPixels, int sbpl,
                                   int w, int h,
                                   int const_alpha);
void qt_blend_rgb32_on_rgb32_lsx(uchar *destPixels, int dbpl,
                                 const uchar *srcPixels, int sbpl,
                                 int w, int h,
                                 int const_alpha);

#endif // QT_COMPILER_SUPPORTS_LSX

#ifdef QT_COMPILER_SUPPORTS_LASX
void qt_memfill64_lasx(quint64 *dest, quint64 value, qsizetype count);
void qt_memfill32_lasx(quint32 *dest, quint32 value, qsizetype count);
#endif // QT_COMPILER_SUPPORTS_LASX

QT_END_NAMESPACE

#endif // QDRAWHELPER_LOONGARCH64_P_H
