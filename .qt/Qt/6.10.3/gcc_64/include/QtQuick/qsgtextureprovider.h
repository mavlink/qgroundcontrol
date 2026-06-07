// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGTEXTUREPROVIDER_H
#define QSGTEXTUREPROVIDER_H

#include <QtQuick/qsgtexture.h>
#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QSGTextureProvider : public QObject
{
    Q_OBJECT
public:
    virtual QSGTexture *texture() const = 0;

Q_SIGNALS:
    void textureChanged();
};

QT_END_NAMESPACE

#endif
