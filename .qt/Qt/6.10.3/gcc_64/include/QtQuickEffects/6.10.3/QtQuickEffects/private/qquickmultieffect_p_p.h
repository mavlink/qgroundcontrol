// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKMULTIEFFECT_P_P_H
#define QQUICKMULTIEFFECT_P_P_H

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

#include <private/qquickmultieffect_p.h>
#include <private/qquickitem_p.h>
#include <private/qgfxsourceproxy_p.h>
#include <QtCore/qvector.h>

QT_BEGIN_NAMESPACE

class QQuickShaderEffect;
class QQuickShaderEffectSource;

class QQuickMultiEffectPrivate : public QQuickItemPrivate
{
    Q_DECLARE_PUBLIC(QQuickMultiEffect)

public:
    QQuickMultiEffectPrivate();
    ~QQuickMultiEffectPrivate();

    QQuickItem *source() const;
    void setSource(QQuickItem *item);

    bool autoPaddingEnabled() const;
    void setAutoPaddingEnabled(bool enabled);

    QRectF paddingRect() const;
    void setPaddingRect(const QRectF &rect);

    qreal brightness() const;
    void setBrightness(qreal brightness);

    qreal contrast() const;
    void setContrast(qreal contrast);

    qreal saturation() const;
    void setSaturation(qreal saturation);

    qreal colorization() const;
    void setColorization(qreal colorization);

    QColor colorizationColor() const;
    void setColorizationColor(const QColor &color);

    bool blurEnabled() const;
    void setBlurEnabled(bool enabled);

    qreal blur() const;
    void setBlur(qreal blur);

    int blurMax() const;
    void setBlurMax(int blurMax);

    qreal blurMultiplier() const;
    void setBlurMultiplier(qreal blurMultiplier);

    bool shadowEnabled() const;
    void setShadowEnabled(bool enabled);

    qreal shadowOpacity() const;
    void setShadowOpacity(qreal shadowOpacity);

    qreal shadowBlur() const;
    void setShadowBlur(qreal shadowBlur);

    qreal shadowHorizontalOffset() const;
    void setShadowHorizontalOffset(qreal offset);

    qreal shadowVerticalOffset() const;
    void setShadowVerticalOffset(qreal offset);

    QColor shadowColor() const;
    void setShadowColor(const QColor &color);

    qreal shadowScale() const;
    void setShadowScale(qreal shadowScale);

    bool maskEnabled() const;
    void setMaskEnabled(bool enabled);

    QQuickItem *maskSource() const;
    void setMaskSource(QQuickItem *item);

    qreal maskThresholdMin() const;
    void setMaskThresholdMin(qreal threshold);

    qreal maskSpreadAtMin() const;
    void setMaskSpreadAtMin(qreal spread);

    qreal maskThresholdMax() const;
    void setMaskThresholdMax(qreal threshold);

    qreal maskSpreadAtMax() const;
    void setMaskSpreadAtMax(qreal spread);

    bool maskInverted() const;
    void setMaskInverted(bool inverted);

    QRectF itemRect() const;
    QString fragmentShader() const;
    QString vertexShader() const;
    bool hasProxySource() const;

    void handleGeometryChange(const QRectF &newGeometry, const QRectF &oldGeometry);
    void handleItemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &value);
    void initialize();
    void updateMaskThresholdSpread();
    void updateCenterOffset();
    void updateShadowOffset();
    void updateColorizationColor();
    void updateShadowColor();
    float calculateLod(float blurAmount);
    float blurWeight(float v);
    void getBlurWeights(float blurLod, QVector4D &blurWeight1, QVector2D &blurWeight2);
    void updateBlurWeights();
    void updateShadowBlurWeights();
    void updateBlurItemSizes(bool forceUpdate = false);
    void updateEffectShaders();
    void updateBlurLevel(bool forceUpdate = false);
    void updateBlurItemsAmount(int blurLevel);
    void updateSourcePadding();
    void updateProxyActiveCheck();
    void proxyOutputChanged();

private:
    bool m_initialized = false;
    QQuickItem *m_sourceItem = nullptr;
    QGfxSourceProxyME *m_shaderSource = nullptr;
    QQuickShaderEffect *m_shaderEffect = nullptr;
    QQuickShaderEffectSource *m_dummyShaderSource = nullptr;
    QVector<QQuickShaderEffect *> m_blurEffects;
    bool m_autoPaddingEnabled = true;
    QRectF m_paddingRect;
    qreal m_brightness = 0.0;
    qreal m_contrast = 0.0;
    qreal m_saturation = 0.0;
    qreal m_colorization = 0.0;
    QColor m_colorizationColor = { 255, 0, 0, 255 };
    bool m_blurEnabled = false;
    qreal m_blur = 0.0;
    int m_blurMax = 32;
    qreal m_blurMultiplier = 0.0;
    bool m_shadowEnabled = false;
    qreal m_shadowOpacity = 1.0;
    qreal m_shadowBlur = 1.0;
    qreal m_shadowHorizontalOffset = 0.0;
    qreal m_shadowVerticalOffset = 0.0;
    QColor m_shadowColor = { 0, 0, 0, 255 };
    qreal m_shadowScale = 1.0;
    bool m_maskEnabled = false;
    QQuickItem *m_maskSourceItem = nullptr;
    qreal m_maskThresholdMin = 0.0;
    qreal m_maskSpreadAtMin = 0.0;
    qreal m_maskThresholdMax = 1.0;
    qreal m_maskSpreadAtMax = 0.0;
    bool m_maskInverted = false;

    int m_blurLevel = 0;
    QString m_vertShader;
    QString m_fragShader;
    QSizeF m_firstBlurItemSize;

    QVector4D m_blurWeight1;
    QVector2D m_blurWeight2;
    QVector4D m_shadowBlurWeight1;
    QVector2D m_shadowBlurWeight2;
};



QT_END_NAMESPACE

#endif // QQUICKMULTIEFFECT_P_P_H
