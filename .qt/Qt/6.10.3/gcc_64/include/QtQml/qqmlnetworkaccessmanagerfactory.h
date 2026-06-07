// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLNETWORKACCESSMANAGERFACTORY_H
#define QQMLNETWORKACCESSMANAGERFACTORY_H

#include <QtQml/qtqmlglobal.h>
#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(qml_network)

class QNetworkAccessManager;
class Q_QML_EXPORT QQmlNetworkAccessManagerFactory
{
public:
    virtual ~QQmlNetworkAccessManagerFactory();
    virtual QNetworkAccessManager *create(QObject *parent) = 0;

};

#endif // qml_network

QT_END_NAMESPACE

#endif // QQMLNETWORKACCESSMANAGERFACTORY_H
