// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKICON_P_H
#define QQUICKICON_P_H

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

#include <QtCore/qurl.h>
#include <QtCore/qstring.h>
#include <QtCore/qobjectdefs.h>
#include <QtCore/qshareddata.h>
#include <QtQuickTemplates2/private/qtquicktemplates2global_p.h>
#include <QtGui/qcolor.h>
#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

class QQuickIconPrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickIcon
{
    Q_GADGET
    Q_PROPERTY(QString name READ name WRITE setName RESET resetName FINAL)
    Q_PROPERTY(QUrl source READ source WRITE setSource RESET resetSource FINAL)
    Q_PROPERTY(int width READ width WRITE setWidth RESET resetWidth FINAL)
    Q_PROPERTY(int height READ height WRITE setHeight RESET resetHeight FINAL)
    Q_PROPERTY(QColor color READ color WRITE setColor RESET resetColor FINAL)
    Q_PROPERTY(bool cache READ cache WRITE setCache RESET resetCache FINAL)
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(2, 3)

public:
    QQuickIcon();
    QQuickIcon(const QQuickIcon &other);
    ~QQuickIcon();

    QQuickIcon& operator=(const QQuickIcon &other);
    bool operator==(const QQuickIcon &other) const;
    bool operator!=(const QQuickIcon &other) const;

    bool isEmpty() const;

    QString name() const;
    void setName(const QString &name);
    void resetName();

    QUrl source() const;
    void setSource(const QUrl &source);
    void resetSource();
    QUrl resolvedSource() const;
    void ensureRelativeSourceResolved(const QObject *owner);

    int width() const;
    void setWidth(int width);
    void resetWidth();

    int height() const;
    void setHeight(int height);
    void resetHeight();

    QColor color() const;
    void setColor(const QColor &color);
    void resetColor();

    bool cache() const;
    void setCache(bool cache);
    void resetCache();

    QQuickIcon resolve(const QQuickIcon &other) const;

private:
    QExplicitlySharedDataPointer<QQuickIconPrivate> d;
};

QT_END_NAMESPACE

#endif // QQUICKICON_P_H
