// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKFONTLOADER_H
#define QQUICKFONTLOADER_H

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

#include <private/qtquickglobal_p.h>

#include <QtQml/qqml.h>

#include <QtCore/qobject.h>
#include <QtCore/qurl.h>

QT_BEGIN_NAMESPACE

class QQuickFontLoaderPrivate;
class Q_QUICK_EXPORT QQuickFontLoader : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickFontLoader)

    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(QFont font READ font NOTIFY fontChanged)
    QML_NAMED_ELEMENT(FontLoader)
    QML_ADDED_IN_VERSION(2, 0)

public:
    enum Status { Null = 0, Ready, Loading, Error };
    Q_ENUM(Status)

    QQuickFontLoader(QObject *parent = nullptr);

    QUrl source() const;
    void setSource(const QUrl &url);

    QString name() const;

    QFont font() const;

    Status status() const;

private Q_SLOTS:
    void updateFontInfo(int);

Q_SIGNALS:
    void sourceChanged();
    void nameChanged();
    void fontChanged();
    void statusChanged();
};

QT_END_NAMESPACE

#endif // QQUICKFONTLOADER_H

