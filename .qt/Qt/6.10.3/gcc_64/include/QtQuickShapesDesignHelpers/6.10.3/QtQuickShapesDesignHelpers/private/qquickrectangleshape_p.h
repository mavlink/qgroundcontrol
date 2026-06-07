// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKRECTANGLESHAPE_P_H
#define QQUICKRECTANGLESHAPE_P_H

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

#include <QtQuickShapes/private/qquickshape_p.h>
#include <QtQuickShapesDesignHelpers/qtquickshapesdesignhelpersexports.h>

QT_BEGIN_NAMESPACE

class QQuickRectangleShapePrivate;

class Q_QUICKSHAPESDESIGNHELPERS_EXPORT QQuickRectangleShape : public QQuickShape
{
    Q_OBJECT

    Q_PROPERTY(int radius READ radius WRITE setRadius NOTIFY radiusChanged)
    Q_PROPERTY(int topLeftRadius READ topLeftRadius WRITE setTopLeftRadius NOTIFY topLeftRadiusChanged RESET resetTopLeftRadius FINAL)
    Q_PROPERTY(int topRightRadius READ topRightRadius WRITE setTopRightRadius NOTIFY topRightRadiusChanged RESET resetTopRightRadius FINAL)
    Q_PROPERTY(int bottomLeftRadius READ bottomLeftRadius WRITE setBottomLeftRadius NOTIFY bottomLeftRadiusChanged RESET resetBottomLeftRadius FINAL)
    Q_PROPERTY(int bottomRightRadius READ bottomRightRadius WRITE setBottomRightRadius NOTIFY bottomRightRadiusChanged RESET resetBottomRightRadius FINAL)
    Q_PROPERTY(bool bevel READ hasBevel WRITE setBevel NOTIFY bevelChanged FINAL)
    Q_PROPERTY(bool topLeftBevel READ hasTopLeftBevel WRITE setTopLeftBevel NOTIFY topLeftBevelChanged RESET resetTopLeftBevel FINAL)
    Q_PROPERTY(bool topRightBevel READ hasTopRightBevel WRITE setTopRightBevel NOTIFY topRightBevelChanged RESET resetTopRightBevel FINAL)
    Q_PROPERTY(bool bottomLeftBevel READ hasBottomLeftBevel WRITE setBottomLeftBevel NOTIFY bottomLeftBevelChanged RESET resetBottomLeftBevel FINAL)
    Q_PROPERTY(bool bottomRightBevel READ hasBottomRightBevel WRITE setBottomRightBevel NOTIFY bottomRightBevelChanged RESET resetBottomRightBevel FINAL)
    Q_PROPERTY(QColor strokeColor READ strokeColor WRITE setStrokeColor NOTIFY strokeColorChanged FINAL)
    Q_PROPERTY(qreal strokeWidth READ strokeWidth WRITE setStrokeWidth NOTIFY strokeWidthChanged FINAL)
    Q_PROPERTY(QColor fillColor READ fillColor WRITE setFillColor NOTIFY fillColorChanged FINAL)
    Q_PROPERTY(QQuickShapePath::JoinStyle joinStyle READ joinStyle WRITE setJoinStyle NOTIFY joinStyleChanged FINAL)
    Q_PROPERTY(QQuickShapePath::CapStyle capStyle READ capStyle WRITE setCapStyle NOTIFY capStyleChanged FINAL)
    Q_PROPERTY(QQuickShapePath::StrokeStyle strokeStyle READ strokeStyle WRITE setStrokeStyle NOTIFY strokeStyleChanged FINAL)
    Q_PROPERTY(qreal dashOffset READ dashOffset WRITE setDashOffset NOTIFY dashOffsetChanged FINAL)
    Q_PROPERTY(QVector<qreal> dashPattern READ dashPattern WRITE setDashPattern NOTIFY dashPatternChanged FINAL)
    Q_PROPERTY(QQuickShapeGradient *fillGradient READ fillGradient WRITE setFillGradient RESET resetFillGradient FINAL)
    Q_PROPERTY(BorderMode borderMode READ borderMode WRITE setBorderMode RESET resetBorderMode FINAL)

    QML_NAMED_ELEMENT(RectangleShape)
    QML_ADDED_IN_VERSION(6, 10)

public:
    QQuickRectangleShape(QQuickItem *parent = nullptr);
    ~QQuickRectangleShape();

    int radius() const;
    void setRadius(int radius);

    int topLeftRadius() const;
    void setTopLeftRadius(int topLeftRadius);
    void resetTopLeftRadius();

    int topRightRadius() const;
    void setTopRightRadius(int topRightRadius);
    void resetTopRightRadius();

    int bottomLeftRadius() const;
    void setBottomLeftRadius(int bottomLeftRadius);
    void resetBottomLeftRadius();

    int bottomRightRadius() const;
    void setBottomRightRadius(int bottomRightRadius);
    void resetBottomRightRadius();

    bool hasBevel() const;
    void setBevel(bool bevel);

    bool hasTopLeftBevel() const;
    void setTopLeftBevel(bool topLeftBevel);
    void resetTopLeftBevel();

    bool hasTopRightBevel() const;
    void setTopRightBevel(bool topRightBevel);
    void resetTopRightBevel();

    bool hasBottomLeftBevel() const;
    void setBottomLeftBevel(bool bottomLeftBevel);
    void resetBottomLeftBevel();

    bool hasBottomRightBevel() const;
    void setBottomRightBevel(bool bottomRightBevel);
    void resetBottomRightBevel();

    QColor strokeColor() const;
    void setStrokeColor(const QColor &color);

    qreal strokeWidth() const;
    void setStrokeWidth(qreal width);

    QColor fillColor() const;
    void setFillColor(const QColor &color);

    QQuickShapePath::FillRule fillRule() const;
    void setFillRule(QQuickShapePath::FillRule fillRule);

    QQuickShapePath::JoinStyle joinStyle() const;
    void setJoinStyle(QQuickShapePath::JoinStyle style);

    QQuickShapePath::CapStyle capStyle() const;
    void setCapStyle(QQuickShapePath::CapStyle style);

    QQuickShapePath::StrokeStyle strokeStyle() const;
    void setStrokeStyle(QQuickShapePath::StrokeStyle style);

    qreal dashOffset() const;
    void setDashOffset(qreal offset);

    QVector<qreal> dashPattern() const;
    void setDashPattern(const QVector<qreal> &array);

    QQuickShapeGradient *fillGradient() const;
    void setFillGradient(QQuickShapeGradient *fillGradient);
    void resetFillGradient();

    enum BorderMode {
        Inside,
        Middle,
        Outside
    };
    Q_ENUM(BorderMode)
    BorderMode borderMode() const;
    void setBorderMode(BorderMode borderMode);
    void resetBorderMode();

Q_SIGNALS:
    void radiusChanged();
    void topLeftRadiusChanged();
    void topRightRadiusChanged();
    void bottomLeftRadiusChanged();
    void bottomRightRadiusChanged();
    void bevelChanged();
    void topLeftBevelChanged();
    void topRightBevelChanged();
    void bottomLeftBevelChanged();
    void bottomRightBevelChanged();
    void shapePathChanged();
    void strokeColorChanged();
    void strokeWidthChanged();
    void fillColorChanged();
    void fillRuleChanged();
    void joinStyleChanged();
    void capStyleChanged();
    void strokeStyleChanged();
    void dashOffsetChanged();
    void dashPatternChanged();
    void gradientChanged();
    void borderModeChanged();

protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private:
    Q_DISABLE_COPY(QQuickRectangleShape)
    Q_DECLARE_PRIVATE(QQuickRectangleShape)
};

QT_END_NAMESPACE

#endif // QQUICKRECTANGLESHAPE_P_H
