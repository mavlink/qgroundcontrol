// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QWIDGET_ANIMATOR_P_H
#define QWIDGET_ANIMATOR_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include <qobject.h>
#include <qhash.h>
#include <qpointer.h>

QT_BEGIN_NAMESPACE

class QWidget;
class QMainWindowLayout;
class QPropertyAnimation;
class QRect;

class QWidgetAnimator : public QObject
{
    Q_OBJECT
public:
    QWidgetAnimator(QMainWindowLayout *layout);
    void animate(QWidget *widget, const QRect &final_geometry, bool animate);
    bool animating() const;

    void abort(QWidget *widget);

private:
    typedef QHash<QWidget*, QPointer<QPropertyAnimation> > AnimationMap;
    AnimationMap m_animation_map;
#if QT_CONFIG(mainwindow)
    QMainWindowLayout *m_mainWindowLayout;
#endif
};

QT_END_NAMESPACE

#endif // QWIDGET_ANIMATOR_P_H
