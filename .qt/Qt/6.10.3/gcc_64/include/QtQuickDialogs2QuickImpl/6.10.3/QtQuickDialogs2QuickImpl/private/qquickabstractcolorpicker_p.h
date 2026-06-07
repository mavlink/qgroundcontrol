// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKABSTRACTCOLORPICKER_P_H
#define QQUICKABSTRACTCOLORPICKER_P_H

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

#include <QtGui/qcolor.h>
#include <QtQuickTemplates2/private/qquickcontrol_p.h>

#include "qtquickdialogs2quickimplglobal_p.h"

QT_BEGIN_NAMESPACE

class QQuickAbstractColorPickerPrivate;

class Q_QUICKDIALOGS2QUICKIMPL_EXPORT QQuickAbstractColorPicker : public QQuickControl
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(qreal hue READ hue WRITE setHue NOTIFY colorChanged)
    Q_PROPERTY(qreal saturation READ saturation WRITE setSaturation NOTIFY colorChanged)
    Q_PROPERTY(qreal value READ value WRITE setValue NOTIFY colorChanged)
    Q_PROPERTY(qreal lightness READ lightness WRITE setLightness NOTIFY colorChanged)
    Q_PROPERTY(qreal alpha READ alpha WRITE setAlpha NOTIFY colorChanged FINAL)
    Q_PROPERTY(bool pressed READ isPressed WRITE setPressed NOTIFY pressedChanged FINAL)
    Q_PROPERTY(QQuickItem *handle READ handle WRITE setHandle NOTIFY handleChanged FINAL)
    Q_PROPERTY(qreal implicitHandleWidth READ implicitHandleWidth NOTIFY implicitHandleWidthChanged FINAL)
    Q_PROPERTY(qreal implicitHandleHeight READ implicitHandleHeight NOTIFY implicitHandleHeightChanged FINAL)
    Q_CLASSINFO("DeferredPropertyNames", "background,contentItem,handle")
    QML_NAMED_ELEMENT(AbstractColorPicker)
    QML_ADDED_IN_VERSION(6, 4)
    QML_UNCREATABLE("AbstractColorPicker is abstract.")

public:
    ~QQuickAbstractColorPicker() override;

    QColor color() const;
    void setColor(const QColor &c);

    qreal hue() const;
    void setHue(qreal hue);

    qreal saturation() const;
    void setSaturation(qreal saturation);

    qreal value() const;
    void setValue(qreal value);

    qreal lightness() const;
    void setLightness(qreal lightness);

    qreal alpha() const;
    void setAlpha(qreal alpha);

    bool isPressed() const;
    void setPressed(bool pressed);

    QQuickItem *handle() const;
    void setHandle(QQuickItem *handle);

    qreal implicitHandleWidth() const;
    qreal implicitHandleHeight() const;

Q_SIGNALS:
    void colorChanged(const QColor &color);
    void pressedChanged();
    void handleChanged();
    void implicitHandleWidthChanged();
    void implicitHandleHeightChanged();

    void colorPicked(const QColor &color);

protected:
    QQuickAbstractColorPicker(QQuickAbstractColorPickerPrivate &dd, QQuickItem *parent);

    virtual QColor colorAt(const QPointF &pos) = 0;
    void componentComplete() override;

private:
    void updateColor(const QPointF &pos);
    Q_DISABLE_COPY(QQuickAbstractColorPicker)
    Q_DECLARE_PRIVATE(QQuickAbstractColorPicker)
};

QT_END_NAMESPACE

#endif // QQUICKABSTRACTCOLORPICKER_P_H
