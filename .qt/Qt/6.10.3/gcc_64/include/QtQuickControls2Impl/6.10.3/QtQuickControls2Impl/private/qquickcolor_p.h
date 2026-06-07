// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKCOLOR_P_H
#define QQUICKCOLOR_P_H

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

#include <QtCore/qobject.h>
#include <QtGui/qcolor.h>
#include <QtQml/qqml.h>
#include <QtQuickControls2Impl/private/qtquickcontrols2implglobal_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICKCONTROLS2IMPL_EXPORT QQuickColor : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Color)
    QML_SINGLETON
    QML_ADDED_IN_VERSION(2, 3)

public:
    explicit QQuickColor(QObject *parent = nullptr);

    Q_INVOKABLE QColor transparent(const QColor &color, qreal opacity) const;
    Q_INVOKABLE QColor blend(const QColor &a, const QColor &b, qreal factor) const;
};

QT_END_NAMESPACE

#endif // QQUICKCOLOR_P_H
