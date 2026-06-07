// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKOPACITYANIMATION_P_H
#define QQUICKOPACITYANIMATION_P_H

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

#include <QtQuick/private/qquickanimation_p.h>
#include <QtQuickVectorImageHelpers/qtquickvectorimagehelpersexports.h>

QT_BEGIN_NAMESPACE

class Q_QUICKVECTORIMAGEHELPERS_EXPORT QQuickColorOpacityAnimation : public QQuickPropertyAnimation
{
    Q_OBJECT

    Q_DECLARE_PRIVATE(QQuickPropertyAnimation)
    Q_PROPERTY(qreal from READ from WRITE setFrom)
    Q_PROPERTY(qreal to READ to WRITE setTo)
    QML_NAMED_ELEMENT(ColorOpacityAnimation)

public:
    QQuickColorOpacityAnimation(QObject *parent = nullptr);

    qreal from() const;
    void setFrom(qreal from);

    qreal to() const;
    void setTo(qreal to);
};

QT_END_NAMESPACE

#endif // QQUICKOPACITYANIMATION_P_H

