// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QIMAGEPIXMAP_CLEANUPHOOKS_P_H
#define QIMAGEPIXMAP_CLEANUPHOOKS_P_H

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
#include <QtGui/qpixmap.h>

QT_BEGIN_NAMESPACE

typedef void (*_qt_image_cleanup_hook_64)(qint64);
typedef void (*_qt_pixmap_cleanup_hook_pmd)(QPlatformPixmap*);


class QImagePixmapCleanupHooks;

class Q_GUI_EXPORT QImagePixmapCleanupHooks
{
public:
    static QImagePixmapCleanupHooks *instance();

    static void enableCleanupHooks(const QImage &image);
    static void enableCleanupHooks(const QPixmap &pixmap);
    static void enableCleanupHooks(QPlatformPixmap *handle);

    static bool isImageCached(const QImage &image);
    static bool isPixmapCached(const QPixmap &pixmap);

    // Gets called when a pixmap data is about to be modified:
    void addPlatformPixmapModificationHook(_qt_pixmap_cleanup_hook_pmd);

    // Gets called when a pixmap data is about to be destroyed:
    void addPlatformPixmapDestructionHook(_qt_pixmap_cleanup_hook_pmd);

    // Gets called when an image is about to be modified or destroyed:
    void addImageHook(_qt_image_cleanup_hook_64);

    void removePlatformPixmapModificationHook(_qt_pixmap_cleanup_hook_pmd);
    void removePlatformPixmapDestructionHook(_qt_pixmap_cleanup_hook_pmd);
    void removeImageHook(_qt_image_cleanup_hook_64);

    static void executePlatformPixmapModificationHooks(QPlatformPixmap*);
    static void executePlatformPixmapDestructionHooks(QPlatformPixmap*);
    static void executeImageHooks(qint64 key);

private:
    QList<_qt_image_cleanup_hook_64> imageHooks;
    QList<_qt_pixmap_cleanup_hook_pmd> pixmapModificationHooks;
    QList<_qt_pixmap_cleanup_hook_pmd> pixmapDestructionHooks;
};

QT_END_NAMESPACE

#endif // QIMAGEPIXMAP_CLEANUPHOOKS_P_H
