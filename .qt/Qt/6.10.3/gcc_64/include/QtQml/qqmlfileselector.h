// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLFILESELECTOR_H
#define QQMLFILESELECTOR_H

#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtQml/QQmlEngine>
#include <QtQml/qtqmlglobal.h>

QT_BEGIN_NAMESPACE

class QFileSelector;
class QQmlFileSelectorPrivate;
class Q_QML_EXPORT QQmlFileSelector : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQmlFileSelector)
public:
    explicit QQmlFileSelector(QQmlEngine *engine, QObject *parent = nullptr);
    ~QQmlFileSelector() override;
    QFileSelector *selector() const noexcept;
    void setSelector(QFileSelector *selector);
    void setExtraSelectors(const QStringList &strings);

#if QT_DEPRECATED_SINCE(6, 0)
    QT_DEPRECATED static QQmlFileSelector *get(QQmlEngine*);
#endif

private:
    Q_DISABLE_COPY(QQmlFileSelector)
};

QT_END_NAMESPACE

#endif
