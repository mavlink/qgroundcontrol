// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKRECTANGULARSHADOW_P_P_H
#define QQUICKRECTANGULARSHADOW_P_P_H

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

#include <private/qtquickglobal_p.h>

QT_REQUIRE_CONFIG(quick_shadereffect);

#include <private/qquickrectangularshadow_p.h>
#include <private/qquickitem_p.h>

QT_BEGIN_NAMESPACE

class QQuickShaderEffect;

class QQuickRectangularShadowPrivate : public QQuickItemPrivate
{
    Q_DECLARE_PUBLIC(QQuickRectangularShadow)

public:
    QQuickRectangularShadowPrivate();

private:
    void initialize();
    void handleGeometryChange(const QRectF &newGeometry, const QRectF &oldGeometry);
    void handleItemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &value);
    qreal getPadding() const;
    void updateColor();
    void updateShaderSource();
    void updateSizeProperties();
    void updateCached();
    qreal clampedRadius() const;
    QQuickItem *currentMaterial() const;

    QQuickShaderEffect *m_defaultMaterial = nullptr;
    QQuickItem *m_material = nullptr;
    QColor m_color = { 0, 0, 0, 255 };
    QVector2D m_offset;
    qreal m_blur = 10.0;
    qreal m_radius = 0.0;
    qreal m_spread = 0.0;
    bool m_cached = false;
    bool m_initialized = false;
};

QT_END_NAMESPACE

#endif // QQUICKRECTANGULARSHADOW_P_P_H
