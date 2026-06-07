// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTEXTIMAGEHANDLER_P_H
#define QTEXTIMAGEHANDLER_P_H

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
#include "QtCore/qobject.h"
#include "QtGui/qabstracttextdocumentlayout.h"

QT_BEGIN_NAMESPACE

class QTextImageFormat;

class Q_GUI_EXPORT QTextImageHandler : public QObject,
                                       public QTextObjectInterface
{
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)
public:
    explicit QTextImageHandler(QObject *parent = nullptr);

    virtual QSizeF intrinsicSize(QTextDocument *doc, int posInDocument, const QTextFormat &format) override;
    virtual void drawObject(QPainter *p, const QRectF &rect, QTextDocument *doc, int posInDocument, const QTextFormat &format) override;
    QImage image(QTextDocument *doc, const QTextImageFormat &imageFormat);
};

QT_END_NAMESPACE

#endif // QTEXTIMAGEHANDLER_P_H
