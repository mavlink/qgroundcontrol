// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef PARTICLEEXTRUDER_H
#define PARTICLEEXTRUDER_H

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
#include <QRectF>
#include <QPointF>
#include <QtQml/qqml.h>
#include <QtCore/private/qglobal_p.h>
#include <QtQuickParticles/qtquickparticlesexports.h>

QT_BEGIN_NAMESPACE

class Q_QUICKPARTICLES_EXPORT QQuickParticleExtruder : public QObject
{
    Q_OBJECT

    QML_NAMED_ELEMENT(ParticleExtruder)
    QML_ADDED_IN_VERSION(2, 0)
    QML_UNCREATABLE("Abstract type. Use one of the inheriting types instead.")

public:
    explicit QQuickParticleExtruder(QObject *parent = nullptr);
    virtual QPointF extrude(const QRectF &);
    virtual bool contains(const QRectF &bounds, const QPointF &point);
};

QT_END_NAMESPACE

#endif // PARTICLEEXTRUDER_H
