// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QXBMHANDLER_P_H
#define QXBMHANDLER_P_H

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
#include "QtGui/qimageiohandler.h"

#ifndef QT_NO_IMAGEFORMAT_XBM

QT_BEGIN_NAMESPACE

class QXbmHandler : public QImageIOHandler
{
public:
    QXbmHandler();
    bool canRead() const override;
    bool read(QImage *image) override;
    bool write(const QImage &image) override;

    static bool canRead(QIODevice *device);

    QVariant option(ImageOption option) const override;
    void setOption(ImageOption option, const QVariant &value) override;
    bool supportsOption(ImageOption option) const override;

private:
    bool readHeader();
    enum State {
        Ready,
        ReadHeader,
        Error
    };
    State state;
    int width;
    int height;
    QString fileName;
};

QT_END_NAMESPACE

#endif // QT_NO_IMAGEFORMAT_XBM

#endif // QXBMHANDLER_P_H
