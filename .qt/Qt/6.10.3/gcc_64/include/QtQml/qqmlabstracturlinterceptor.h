// Copyright (C) 2016 Research In Motion.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLABSTRACTURLINTERCEPTOR_H
#define QQMLABSTRACTURLINTERCEPTOR_H

#include <QtQml/qtqmlglobal.h>

QT_BEGIN_NAMESPACE

class QUrl;

class Q_QML_EXPORT QQmlAbstractUrlInterceptor
{
public:
    enum DataType { //Matches QQmlDataBlob::Type
        QmlFile = 0,
        JavaScriptFile = 1,
        QmldirFile = 2,
        UrlString = 0x1000
    };

    QQmlAbstractUrlInterceptor() = default;
    virtual ~QQmlAbstractUrlInterceptor() = default;
    virtual QUrl intercept(const QUrl &path, DataType type) = 0;
};

QT_END_NAMESPACE
#endif
