// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef VARYINGVECTOR_H
#define VARYINGVECTOR_H

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

#include <QObject>
#include <QPointF>
#include <QtQml/qqml.h>
#include <QtCore/private/qglobal_p.h>
#include <QtQuickParticles/qtquickparticlesexports.h>

QT_BEGIN_NAMESPACE

class Q_QUICKPARTICLES_EXPORT QQuickDirection : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(NullVector)
    QML_ADDED_IN_VERSION(2, 0)
    QML_UNCREATABLE("Abstract type. Use one of the inheriting types instead.")

public:
    explicit QQuickDirection(QObject *parent = nullptr);

    virtual QPointF sample(const QPointF &from);
Q_SIGNALS:

public Q_SLOTS:

protected:
};

QT_END_NAMESPACE
#endif // VARYINGVECTOR_H
