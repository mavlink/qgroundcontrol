// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLATFORMCLIPBOARD_H
#define QPLATFORMCLIPBOARD_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include <QtGui/qtguiglobal.h>

#ifndef QT_NO_CLIPBOARD

#include <QtGui/QClipboard>

QT_BEGIN_NAMESPACE


class Q_GUI_EXPORT QPlatformClipboard
{
public:
    Q_DISABLE_COPY_MOVE(QPlatformClipboard)

    QPlatformClipboard() = default;
    virtual ~QPlatformClipboard();

    virtual QMimeData *mimeData(QClipboard::Mode mode = QClipboard::Clipboard);
    virtual void setMimeData(QMimeData *data, QClipboard::Mode mode = QClipboard::Clipboard);
    virtual bool supportsMode(QClipboard::Mode mode) const;
    virtual bool ownsMode(QClipboard::Mode mode) const;
    void emitChanged(QClipboard::Mode mode);
};

QT_END_NAMESPACE

#endif // QT_NO_CLIPBOARD

#endif //QPLATFORMCLIPBOARD_H
