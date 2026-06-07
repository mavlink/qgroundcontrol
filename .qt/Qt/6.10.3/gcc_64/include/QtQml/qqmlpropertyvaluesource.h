// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLPROPERTYVALUESOURCE_H
#define QQMLPROPERTYVALUESOURCE_H

#include <QtQml/qtqmlglobal.h>
#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE


class QQmlProperty;
class Q_QML_EXPORT QQmlPropertyValueSource
{
public:
    QQmlPropertyValueSource();
    virtual ~QQmlPropertyValueSource();
    virtual void setTarget(const QQmlProperty &) = 0;
};

#define QQmlPropertyValueSource_iid "org.qt-project.Qt.QQmlPropertyValueSource"

Q_DECLARE_INTERFACE(QQmlPropertyValueSource, QQmlPropertyValueSource_iid)

QT_END_NAMESPACE

#endif // QQMLPROPERTYVALUESOURCE_H
