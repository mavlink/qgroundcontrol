// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSHAREDIMAGEPROVIDER_H
#define QSHAREDIMAGEPROVIDER_H

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

#include "qtlabssharedimageglobal_p.h"

#include <QQuickImageProvider>
#include <private/qquickpixmap_p.h>
#include <QScopedPointer>

#include "qsharedimageloader_p.h"

QT_BEGIN_NAMESPACE

class SharedImageProvider;

class QuickSharedImageLoader : public QSharedImageLoader
{
    Q_OBJECT
    friend class SharedImageProvider;

public:
    enum ImageParameter {
        OriginalSize = 0,
        RequestedSize,
        ProviderOptions,
        NumImageParameters
    };

    QuickSharedImageLoader(QObject *parent = nullptr);
protected:
    QImage loadFile(const QString &path, ImageParameters *params) override;
    QString key(const QString &path, ImageParameters *params) override;
};

class Q_LABSSHAREDIMAGE_EXPORT SharedImageProvider : public QQuickImageProviderWithOptions
{
public:
    SharedImageProvider();

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize, const QQuickImageProviderOptions &options) override;

protected:
    QScopedPointer<QuickSharedImageLoader> loader;
};

QT_END_NAMESPACE

#endif // QSHAREDIMAGEPROVIDER_H
