// Copyright (C) 2016 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKITEMGRABRESULT_H
#define QQUICKITEMGRABRESULT_H

#include <QtCore/QObject>
#include <QtCore/QSize>
#include <QtCore/QUrl>
#include <QtGui/QImage>
#include <QtQml/QJSValue>
#include <QtQml/qqml.h>
#include <QtQuick/qtquickglobal.h>

QT_BEGIN_NAMESPACE

class QQuickItemGrabResultPrivate;

class Q_QUICK_EXPORT QQuickItemGrabResult : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickItemGrabResult)

    Q_PROPERTY(QImage image READ image CONSTANT)
    Q_PROPERTY(QUrl url READ url CONSTANT)
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(2, 0)

public:
    QImage image() const;
    QUrl url() const;

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#if QT_DEPRECATED_SINCE(5, 15)
    QT_DEPRECATED_X("This overload is deprecated. Use the const member function instead")
    Q_INVOKABLE bool saveToFile(const QString &fileName);
#endif
#endif
    Q_INVOKABLE bool saveToFile(const QString &fileName) const;
    Q_REVISION(6, 2) Q_INVOKABLE bool saveToFile(const QUrl &fileName) const;

protected:
    bool event(QEvent *) override;

Q_SIGNALS:
    void ready();

private Q_SLOTS:
    void setup();
    void render();

private:
    friend class QQuickItem;

    QQuickItemGrabResult(QObject *parent = nullptr);
};

QT_END_NAMESPACE

#endif
