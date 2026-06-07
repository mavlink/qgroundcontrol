// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKCOLORDIALOGIMPL_P_P_H
#define QQUICKCOLORDIALOGIMPL_P_P_H

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

#include "qquickcolordialogimpl_p.h"
#include "qquickabstractcolorpicker_p.h"
#include "qquickcolorinputs_p.h"

#include <QtQuickTemplates2/private/qquickdialog_p_p.h>
#include <QtQuickTemplates2/private/qquickdialogbuttonbox_p.h>
#include <QtQuickTemplates2/private/qquickabstractbutton_p.h>
#include <QtQuickTemplates2/private/qquickslider_p.h>

#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

class QQuickEyeDropperEventFilter : public QObject
{
public:
    enum class LeaveReason { Default, Cancel };
    explicit QQuickEyeDropperEventFilter(std::function<void(QPoint, LeaveReason)> callOnLeave,
                                         std::function<void(QPoint)> callOnUpdate)
        : m_leave(callOnLeave), m_update(callOnUpdate)
    {
    }

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    std::function<void(QPoint, LeaveReason)> m_leave;
    std::function<void(QPoint)> m_update;
    QPoint m_lastPosition;
};

class QQuickColorDialogImplPrivate : public QQuickDialogPrivate
{
public:
    Q_DECLARE_PUBLIC(QQuickColorDialogImpl);

public:
    explicit QQuickColorDialogImplPrivate();
    ~QQuickColorDialogImplPrivate();

    static QQuickColorDialogImplPrivate *get(QQuickColorDialogImpl *dialog)
    {
        return dialog->d_func();
    }

    QQuickColorDialogImplAttached *attachedOrWarn();

    void handleClick(QQuickAbstractButton *button) override;

    void eyeDropperEnter();
    void eyeDropperLeave(const QPoint &pos, QQuickEyeDropperEventFilter::LeaveReason actionOnLeave);
    void eyeDropperPointerMoved(const QPoint &pos);

    void alphaSliderMoved();

    QSharedPointer<QColorDialogOptions> options;
    HSVA m_hsva;
    std::unique_ptr<QQuickEyeDropperEventFilter> eyeDropperEventFilter;
    QPointer<QQuickWindow> m_eyeDropperWindow;
    QColor m_eyeDropperPreviousColor;
    bool m_eyeDropperMode = false;
    bool m_showAlpha = false;
    bool m_hsl = false;
};

class QQuickColorDialogImplAttachedPrivate : public QObjectPrivate
{
public:
    QPointer<QQuickDialogButtonBox> buttonBox;
    QPointer<QQuickAbstractButton> eyeDropperButton;
    QPointer<QQuickColorInputs> colorInputs;
    QPointer<QQuickAbstractColorPicker> colorPicker;
    QPointer<QQuickSlider> alphaSlider;

    Q_DECLARE_PUBLIC(QQuickColorDialogImplAttached)
};

QT_END_NAMESPACE

#endif // QQUICKCOLORDIALOGIMPL_P_P_H
