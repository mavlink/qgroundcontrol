// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QABSTRACTFILEICONPROVIDER_P_H
#define QABSTRACTFILEICONPROVIDER_P_H

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
#if QT_CONFIG(mimetype)
#include <QtCore/QMimeDatabase>
#endif
#include "qabstractfileiconprovider.h"

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QAbstractFileIconProviderPrivate
{
    Q_DECLARE_PUBLIC(QAbstractFileIconProvider)

public:
    QAbstractFileIconProviderPrivate(QAbstractFileIconProvider *q);
    virtual ~QAbstractFileIconProviderPrivate();

    QIcon getPlatformThemeIcon(QAbstractFileIconProvider::IconType type) const;
    QIcon getIconThemeIcon(QAbstractFileIconProvider::IconType type) const;
    QIcon getPlatformThemeIcon(const QFileInfo &info) const;
    QIcon getIconThemeIcon(const QFileInfo &info) const;

    static void clearIconTypeCache();
    static QString getFileType(const QFileInfo &info);

    QAbstractFileIconProvider *q_ptr = nullptr;
    QAbstractFileIconProvider::Options options = {};

#if QT_CONFIG(mimetype)
    QMimeDatabase mimeDatabase;
#endif
};

QT_END_NAMESPACE

#endif // QABSTRACTFILEICONPROVIDER_P_H
