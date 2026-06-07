// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKMULTIEFFECT_H
#define QQUICKMULTIEFFECT_H

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

#include "qtquickeffectsglobal_p.h"
#include <QtQuick/qquickitem.h>
#include <QtCore/qrect.h>
#include <QtGui/qvector4d.h>
#include <QtGui/qcolor.h>

QT_BEGIN_NAMESPACE

class QQuickMultiEffectPrivate;

class Q_QUICKEFFECTS_EXPORT QQuickMultiEffect : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QQuickItem *source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(bool autoPaddingEnabled READ autoPaddingEnabled WRITE setAutoPaddingEnabled NOTIFY autoPaddingEnabledChanged)
    Q_PROPERTY(QRectF paddingRect READ paddingRect WRITE setPaddingRect NOTIFY paddingRectChanged)
    Q_PROPERTY(qreal brightness READ brightness WRITE setBrightness NOTIFY brightnessChanged)
    Q_PROPERTY(qreal contrast READ contrast WRITE setContrast NOTIFY contrastChanged)
    Q_PROPERTY(qreal saturation READ saturation WRITE setSaturation NOTIFY saturationChanged)
    Q_PROPERTY(qreal colorization READ colorization WRITE setColorization NOTIFY colorizationChanged)
    Q_PROPERTY(QColor colorizationColor READ colorizationColor WRITE setColorizationColor NOTIFY colorizationColorChanged)
    Q_PROPERTY(bool blurEnabled READ blurEnabled WRITE setBlurEnabled NOTIFY blurEnabledChanged)
    Q_PROPERTY(qreal blur READ blur WRITE setBlur NOTIFY blurChanged)
    Q_PROPERTY(int blurMax READ blurMax WRITE setBlurMax NOTIFY blurMaxChanged)
    Q_PROPERTY(qreal blurMultiplier READ blurMultiplier WRITE setBlurMultiplier NOTIFY blurMultiplierChanged)
    Q_PROPERTY(bool shadowEnabled READ shadowEnabled WRITE setShadowEnabled NOTIFY shadowEnabledChanged)
    Q_PROPERTY(qreal shadowOpacity READ shadowOpacity WRITE setShadowOpacity NOTIFY shadowOpacityChanged)
    Q_PROPERTY(qreal shadowBlur READ shadowBlur WRITE setShadowBlur NOTIFY shadowBlurChanged)
    Q_PROPERTY(qreal shadowHorizontalOffset READ shadowHorizontalOffset WRITE setShadowHorizontalOffset NOTIFY shadowHorizontalOffsetChanged)
    Q_PROPERTY(qreal shadowVerticalOffset READ shadowVerticalOffset WRITE setShadowVerticalOffset NOTIFY shadowVerticalOffsetChanged)
    Q_PROPERTY(QColor shadowColor READ shadowColor WRITE setShadowColor NOTIFY shadowColorChanged)
    Q_PROPERTY(qreal shadowScale READ shadowScale WRITE setShadowScale NOTIFY shadowScaleChanged)
    Q_PROPERTY(bool maskEnabled READ maskEnabled WRITE setMaskEnabled NOTIFY maskEnabledChanged)
    Q_PROPERTY(QQuickItem *maskSource READ maskSource WRITE setMaskSource NOTIFY maskSourceChanged)
    Q_PROPERTY(qreal maskThresholdMin READ maskThresholdMin WRITE setMaskThresholdMin NOTIFY maskThresholdMinChanged)
    Q_PROPERTY(qreal maskSpreadAtMin READ maskSpreadAtMin WRITE setMaskSpreadAtMin NOTIFY maskSpreadAtMinChanged)
    Q_PROPERTY(qreal maskThresholdMax READ maskThresholdMax WRITE setMaskThresholdMax NOTIFY maskThresholdMaxChanged)
    Q_PROPERTY(qreal maskSpreadAtMax READ maskSpreadAtMax WRITE setMaskSpreadAtMax NOTIFY maskSpreadAtMaxChanged)
    Q_PROPERTY(bool maskInverted READ maskInverted WRITE setMaskInverted NOTIFY maskInvertedChanged)
    Q_PROPERTY(QRectF itemRect READ itemRect NOTIFY itemRectChanged)
    Q_PROPERTY(QString fragmentShader READ fragmentShader NOTIFY fragmentShaderChanged)
    Q_PROPERTY(QString vertexShader READ vertexShader NOTIFY vertexShaderChanged)
    Q_PROPERTY(bool hasProxySource READ hasProxySource NOTIFY hasProxySourceChanged)
    QML_NAMED_ELEMENT(MultiEffect)
    QML_ADDED_IN_VERSION(6, 5)

public:
    QQuickMultiEffect(QQuickItem *parent = nullptr);
    ~QQuickMultiEffect() override;

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

Q_SIGNALS:
    void shaderChanged();
    void itemSizeChanged();
    void sourceChanged();
    void autoPaddingEnabledChanged();
    void paddingRectChanged();
    void brightnessChanged();
    void contrastChanged();
    void saturationChanged();
    void colorizationChanged();
    void colorizationColorChanged();
    void blurEnabledChanged();
    void blurChanged();
    void blurMaxChanged();
    void blurMultiplierChanged();
    void shadowEnabledChanged();
    void shadowOpacityChanged();
    void shadowBlurChanged();
    void shadowHorizontalOffsetChanged();
    void shadowVerticalOffsetChanged();
    void shadowColorChanged();
    void shadowScaleChanged();
    void maskEnabledChanged();
    void maskSourceChanged();
    void maskThresholdMinChanged();
    void maskSpreadAtMinChanged();
    void maskThresholdMaxChanged();
    void maskSpreadAtMaxChanged();
    void maskInvertedChanged();
    void itemRectChanged();
    void fragmentShaderChanged();
    void vertexShaderChanged();
    void hasProxySourceChanged();

protected:
    void componentComplete() override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void itemChange(ItemChange change, const ItemChangeData &value) override;

private:
    Q_DECLARE_PRIVATE(QQuickMultiEffect)
};

QT_END_NAMESPACE

#endif // QQUICKMULTIEFFECT_H
