// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKABSTRACTCOLORPICKER_P_P_H
#define QQUICKABSTRACTCOLORPICKER_P_P_H

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

#include <QtQuickTemplates2/private/qquickcontrol_p_p.h>
#include <QtQuickTemplates2/private/qquickdeferredexecute_p_p.h>

#include "qquickabstractcolorpicker_p.h"
#include "qquickcolordialogutils_p.h"

QT_BEGIN_NAMESPACE

class QQuickAbstractColorPickerPrivate : public QQuickControlPrivate
{
    Q_DECLARE_PUBLIC(QQuickAbstractColorPicker);

public:
    explicit QQuickAbstractColorPickerPrivate();

    static QQuickAbstractColorPickerPrivate *get(QQuickAbstractColorPicker *colorPicker)
    {
        return colorPicker->d_func();
    }

    bool handlePress(const QPointF &point, ulong timestamp) override;
    bool handleMove(const QPointF &point, ulong timestamp) override;
    bool handleRelease(const QPointF &point, ulong timestamp) override;
    void handleUngrab() override;

    void cancelHandle();
    void executeHandle(bool complete = false);

    void itemImplicitWidthChanged(QQuickItem *item) override;
    void itemImplicitHeightChanged(QQuickItem *item) override;

    HSVA m_hsva;
    QPointF m_pressPoint;
    QQuickDeferredPointer<QQuickItem> m_handle;
    bool m_pressed = false;

protected:
    bool m_hsl = false; // Use hsv by default.
};

QT_END_NAMESPACE

#endif // QQUICKABSTRACTCOLORPICKER_P_P_H
