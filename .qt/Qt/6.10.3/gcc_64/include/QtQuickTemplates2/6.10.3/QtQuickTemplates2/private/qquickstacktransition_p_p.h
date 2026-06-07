// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSTACKTRANSITION_P_P_H
#define QQUICKSTACKTRANSITION_P_P_H

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

#include <QtQuick/private/qtquickglobal_p.h>

QT_REQUIRE_CONFIG(quick_viewtransitions);

#include <QtQuickTemplates2/private/qquickstackview_p.h>
#include <QtQuick/private/qquickitemviewtransition_p.h>

QT_BEGIN_NAMESPACE

class QQuickStackElement;

struct QQuickStackTransition
{
    static QQuickStackTransition popExit(QQuickStackView::Operation operation, QQuickStackElement *element, QQuickStackView *view);
    static QQuickStackTransition popEnter(QQuickStackView::Operation operation, QQuickStackElement *element, QQuickStackView *view);

    static QQuickStackTransition pushExit(QQuickStackView::Operation operation, QQuickStackElement *element, QQuickStackView *view);
    static QQuickStackTransition pushEnter(QQuickStackView::Operation operation, QQuickStackElement *element, QQuickStackView *view);

    static QQuickStackTransition replaceExit(QQuickStackView::Operation operation, QQuickStackElement *element, QQuickStackView *view);
    static QQuickStackTransition replaceEnter(QQuickStackView::Operation operation, QQuickStackElement *element, QQuickStackView *view);

    bool target = false;
    QQuickStackView::Status status = QQuickStackView::Inactive;
    QQuickItemViewTransitioner::TransitionType type = QQuickItemViewTransitioner::NoTransition;
    QRectF viewBounds;
    QQuickStackElement *element = nullptr;
    QQuickTransition *transition = nullptr;
};

QT_END_NAMESPACE

#endif // QQUICKSTACKTRANSITION_P_P_H
