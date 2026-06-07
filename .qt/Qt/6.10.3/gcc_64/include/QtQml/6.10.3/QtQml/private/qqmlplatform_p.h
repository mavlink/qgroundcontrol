// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLPLATFORM_P_H
#define QQMLPLATFORM_P_H

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

#include <QtCore/QObject>
#include <qqml.h>
#include <private/qtqmlglobal_p.h>

QT_BEGIN_NAMESPACE

class Q_QML_EXPORT QQmlPlatform : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString os READ os CONSTANT)
    Q_PROPERTY(QString pluginName READ pluginName CONSTANT)
    QML_ANONYMOUS

public:
    explicit QQmlPlatform(QObject *parent = nullptr);
    virtual ~QQmlPlatform();

    static QString os();
    QString pluginName() const;

private:
    Q_DISABLE_COPY(QQmlPlatform)
};

QT_END_NAMESPACE

#endif // QQMLPLATFORM_P_H
