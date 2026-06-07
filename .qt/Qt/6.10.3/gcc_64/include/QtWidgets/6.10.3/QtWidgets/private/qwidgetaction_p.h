// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QWIDGETACTION_P_H
#define QWIDGETACTION_P_H

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
#include "private/qaction_widgets_p.h"

#include <QtCore/qpointer.h>

QT_REQUIRE_CONFIG(action);

QT_BEGIN_NAMESPACE

class QWidgetActionPrivate : public QtWidgetsActionPrivate
{
    Q_DECLARE_PUBLIC(QWidgetAction)
public:
    inline QWidgetActionPrivate() : defaultWidgetInUse(false), autoCreated(false) {}
    QPointer<QWidget> defaultWidget;
    QList<QWidget *> createdWidgets;
    uint defaultWidgetInUse : 1;
    uint autoCreated : 1; // created by QToolBar::addWidget and the like

    inline void widgetDestroyed(QObject *o) {
        createdWidgets.removeAll(static_cast<QWidget *>(o));
    }
};

QT_END_NAMESPACE

#endif
