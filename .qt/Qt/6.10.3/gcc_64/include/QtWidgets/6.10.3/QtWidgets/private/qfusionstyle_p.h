// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QFUSIONSTYLE_P_H
#define QFUSIONSTYLE_P_H

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

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include <QtWidgets/qcommonstyle.h>

QT_BEGIN_NAMESPACE


#if QT_CONFIG(style_fusion)

class QFusionStylePrivate;
class Q_WIDGETS_EXPORT QFusionStyle : public QCommonStyle
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QFusionStyle)

public:
    QFusionStyle();
    ~QFusionStyle();

    QPalette standardPalette () const override;
    void drawPrimitive(PrimitiveElement elem,
                       const QStyleOption *option,
                       QPainter *painter, const QWidget *widget = nullptr) const override;
    void drawControl(ControlElement ce, const QStyleOption *option, QPainter *painter,
                     const QWidget *widget) const override;
    int pixelMetric(PixelMetric metric, const QStyleOption *option = nullptr, const QWidget *widget = nullptr) const override;
    void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                            QPainter *painter, const QWidget *widget) const override;
    QRect subElementRect(SubElement r, const QStyleOption *opt, const QWidget *widget = nullptr) const override;
    QSize sizeFromContents(ContentsType type, const QStyleOption *option,
                           const QSize &size, const QWidget *widget) const override;
    SubControl hitTestComplexControl(ComplexControl cc, const QStyleOptionComplex *opt,
                                     const QPoint &pt, const QWidget *w = nullptr) const override;
    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex *opt,
                         SubControl sc, const QWidget *widget) const override;
    QPixmap generatedIconPixmap(QIcon::Mode iconMode, const QPixmap &pixmap,
                                const QStyleOption *opt) const override;
    int styleHint(StyleHint hint, const QStyleOption *option = nullptr, const QWidget *widget = nullptr,
                  QStyleHintReturn *returnData = nullptr) const override;
    QRect itemPixmapRect(const QRect &r, int flags, const QPixmap &pixmap) const override;
    QIcon iconFromTheme(StandardPixmap standardIcon) const;
    QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption *option = nullptr,
                       const QWidget *widget = nullptr) const override;
    QPixmap standardPixmap(StandardPixmap standardPixmap, const QStyleOption *opt,
                           const QWidget *widget = nullptr) const override;
    void drawItemPixmap(QPainter *painter, const QRect &rect,
                        int alignment, const QPixmap &pixmap) const override;
    void drawItemText(QPainter *painter, const QRect &rect,
                      int flags, const QPalette &pal, bool enabled,
                      const QString &text, QPalette::ColorRole textRole = QPalette::NoRole) const override;
    void polish(QWidget *widget) override;
    void polish(QApplication *app) override;
    void polish(QPalette &pal) override;
    void unpolish(QWidget *widget) override;
    void unpolish(QApplication *app) override;

protected:
    QFusionStyle(QFusionStylePrivate &dd);

private:
    bool isHighContrast() const;
    Qt::ColorScheme colorScheme() const;
};

#endif // style_fusion

QT_END_NAMESPACE

#endif //QFUSIONSTYLE_P_H
