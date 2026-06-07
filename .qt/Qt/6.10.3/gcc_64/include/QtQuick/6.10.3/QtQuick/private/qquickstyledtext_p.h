// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSTYLEDTEXT_H
#define QQUICKSTYLEDTEXT_H

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

#include <QSize>
#include <QPointF>
#include <QList>
#include <QUrl>
#include <QScopedPointer>
#include <QtQuick/private/qquickpixmap_p.h>

QT_BEGIN_NAMESPACE

class QQuickStyledTextImgTag;
class QQuickStyledTextPrivate;
class QString;
class QQmlContext;

class Q_AUTOTEST_EXPORT QQuickStyledTextImgTag
{
public:
    QQuickStyledTextImgTag() = default;
    ~QQuickStyledTextImgTag() = default;

    enum Align {
        Bottom,
        Middle,
        Top
    };

    QUrl url;
    QPointF pos;
    QSize size;
    int position = 0;
    qreal offset = 0.0; // this offset allows us to compensate for flooring reserved space
    Align align = QQuickStyledTextImgTag::Bottom;
    QScopedPointer<QQuickPixmap> pix;
};

class Q_AUTOTEST_EXPORT QQuickStyledText
{
public:
    static void parse(const QString &string, QTextLayout &layout,
                      QList<QQuickStyledTextImgTag*> &imgTags,
                      const QUrl &baseUrl,
                      QQmlContext *context,
                      bool preloadImages,
                      bool *fontSizeModified);

private:
    QQuickStyledText(const QString &string, QTextLayout &layout,
                           QList<QQuickStyledTextImgTag*> &imgTags,
                           const QUrl &baseUrl,
                           QQmlContext *context,
                           bool preloadImages,
                           bool *fontSizeModified);
    ~QQuickStyledText();

    QQuickStyledTextPrivate *d;
};

QT_END_NAMESPACE

#endif
