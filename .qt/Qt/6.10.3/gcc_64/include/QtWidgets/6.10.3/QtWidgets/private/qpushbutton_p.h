// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QPUSHBUTTON_P_H
#define QPUSHBUTTON_P_H

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "private/qabstractbutton_p.h"

#include <QtCore/qpointer.h>

QT_REQUIRE_CONFIG(pushbutton);

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

QT_BEGIN_NAMESPACE

class QDialog;
class QPushButton;

class QPushButtonPrivate : public QAbstractButtonPrivate
{
    Q_DECLARE_PUBLIC(QPushButton)
public:
    enum AutoDefaultValue { Off = 0, On = 1, Auto = 2 };

    QPushButtonPrivate()
        : QAbstractButtonPrivate(QSizePolicy::PushButton), autoDefault(Auto),
          defaultButton(false), flat(false), menuOpen(false), hovering(false),
          lastAutoDefault(false)
    {}

    void init();
    static QPushButtonPrivate* get(QPushButton *b) { return b->d_func(); }
#if QT_CONFIG(menu)
    QPoint adjustedMenuPosition();
#endif
    void resetLayoutItemMargins();
    void popupPressed();
#if QT_CONFIG(dialog)
    QDialog *dialogParent() const;
#else
    QDialog *dialogParent() const { return nullptr; }
#endif

    QPointer<QMenu> menu;
    uint autoDefault : 2;
    uint defaultButton : 1;
    uint flat : 1;
    uint menuOpen : 1;
    uint hovering : 1;
    mutable uint lastAutoDefault : 1;
};

QT_END_NAMESPACE

#endif // QPUSHBUTTON_P_H
