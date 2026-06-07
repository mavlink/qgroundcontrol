// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSHAREDIMAGELOADER_H
#define QSHAREDIMAGELOADER_H

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

#include <QImage>
#include <QVariant>
#include <QLoggingCategory>
#include <qqml.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcSharedImage);

class QSharedImageLoaderPrivate;

class Q_LABSSHAREDIMAGE_EXPORT QSharedImageLoader : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QSharedImageLoader)

    // We need to provide some type, in order to mention the 1.0 version.
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(1, 0)

public:
    typedef QVector<QVariant> ImageParameters;

    QSharedImageLoader(QObject *parent = nullptr);
    ~QSharedImageLoader();

    QImage load(const QString &path, ImageParameters *params = nullptr);

protected:
    virtual QImage loadFile(const QString &path, ImageParameters *params);
    virtual QString key(const QString &path, ImageParameters *params);

private:
    Q_DISABLE_COPY(QSharedImageLoader)
};

QT_END_NAMESPACE

#endif // QSHAREDIMAGELOADER_H
