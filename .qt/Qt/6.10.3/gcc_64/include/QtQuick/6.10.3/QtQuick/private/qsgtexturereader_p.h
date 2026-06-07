// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGTEXTUREREADER_H
#define QSGTEXTUREREADER_H

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

#include <QString>
#include <QFileInfo>
#include <private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QIODevice;
class QQuickTextureFactory;
class QTextureFileReader;

class QSGTextureReader
{
public:
    QSGTextureReader(QIODevice *device, const QString &fileName = QString());
    ~QSGTextureReader();

    QQuickTextureFactory *read();
    bool isTexture();

    // TBD access function to params
    // TBD ask for identified fmt

    static QList<QByteArray> supportedFileFormats();

private:
    QTextureFileReader *m_reader = nullptr;
};

QT_END_NAMESPACE

#endif // QSGTEXTUREREADER_H
