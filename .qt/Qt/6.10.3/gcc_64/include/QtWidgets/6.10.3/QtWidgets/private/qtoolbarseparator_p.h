// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QDYNAMICTOOLBARSEPARATOR_P_H
#define QDYNAMICTOOLBARSEPARATOR_P_H

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
#include "QtWidgets/qwidget.h"

QT_REQUIRE_CONFIG(toolbar);

QT_BEGIN_NAMESPACE

class QStyleOption;
class QToolBar;

class QToolBarSeparator : public QWidget
{
    Q_OBJECT
    Qt::Orientation orient;

public:
    explicit QToolBarSeparator(QToolBar *parent);

    Qt::Orientation orientation() const;

    QSize sizeHint() const override;

    void paintEvent(QPaintEvent *) override;
    void initStyleOption(QStyleOption *option) const;

public Q_SLOTS:
    void setOrientation(Qt::Orientation orientation);
};

QT_END_NAMESPACE

#endif // QDYNAMICTOOLBARSEPARATOR_P_H
