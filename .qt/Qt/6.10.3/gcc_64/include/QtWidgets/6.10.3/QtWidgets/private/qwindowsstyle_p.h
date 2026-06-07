// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QWINDOWSSTYLE_P_H
#define QWINDOWSSTYLE_P_H

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


#if QT_CONFIG(style_windows)

class QWindowsStylePrivate;

class Q_WIDGETS_EXPORT QWindowsStyle : public QCommonStyle
{
    Q_OBJECT
public:
    QWindowsStyle();
    ~QWindowsStyle();

    void polish(QApplication*) override;
    void unpolish(QApplication*) override;

    void polish(QWidget*) override;
    void unpolish(QWidget*) override;

    void polish(QPalette &) override;

    void drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p,
                       const QWidget *w = nullptr) const override;
    void drawControl(ControlElement element, const QStyleOption *opt, QPainter *p,
                     const QWidget *w = nullptr) const override;
    QRect subElementRect(SubElement r, const QStyleOption *opt, const QWidget *widget = nullptr) const override;
    void drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt, QPainter *p,
                            const QWidget *w = nullptr) const override;
    QSize sizeFromContents(ContentsType ct, const QStyleOption *opt,
                           const QSize &contentsSize, const QWidget *widget = nullptr) const override;

    int pixelMetric(PixelMetric pm, const QStyleOption *option = nullptr, const QWidget *widget = nullptr) const override;

    int styleHint(StyleHint hint, const QStyleOption *opt = nullptr, const QWidget *widget = nullptr,
                  QStyleHintReturn *returnData = nullptr) const override;

    QPixmap standardPixmap(StandardPixmap standardPixmap, const QStyleOption *opt,
                           const QWidget *widget = nullptr) const override;

    QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption *option = nullptr,
                       const QWidget *widget = nullptr) const override;

protected:
    bool eventFilter(QObject *o, QEvent *e) override;
    QWindowsStyle(QWindowsStylePrivate &dd);

private:
    Q_DISABLE_COPY_MOVE(QWindowsStyle)
    Q_DECLARE_PRIVATE(QWindowsStyle)
};

#endif // style_windows

QT_END_NAMESPACE

#endif // QWINDOWSSTYLE_P_H
