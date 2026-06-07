// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLPARSERSTATUS_H
#define QQMLPARSERSTATUS_H

#include <QtQml/qtqmlglobal.h>
#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE


class Q_QML_EXPORT QQmlParserStatus
{
public:
    QQmlParserStatus();
    virtual ~QQmlParserStatus();

    virtual void classBegin()=0;
    virtual void componentComplete()=0;

private:
    quintptr d;
};

#define QQmlParserStatus_iid "org.qt-project.Qt.QQmlParserStatus"
Q_DECLARE_INTERFACE(QQmlParserStatus, QQmlParserStatus_iid)

QT_END_NAMESPACE

#endif // QQMLPARSERSTATUS_H
