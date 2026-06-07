// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QDYNAMICTOOLBAREXTENSION_P_H
#define QDYNAMICTOOLBAREXTENSION_P_H

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
#include "QtWidgets/qtoolbutton.h"

QT_REQUIRE_CONFIG(toolbutton);

QT_BEGIN_NAMESPACE

class Q_AUTOTEST_EXPORT QToolBarExtension : public QToolButton
{
    Q_OBJECT

public:
    explicit QToolBarExtension(QWidget *parent);
    void paintEvent(QPaintEvent *) override;
    QSize sizeHint() const override;

public Q_SLOTS:
    void setOrientation(Qt::Orientation o);

protected:
    bool event(QEvent *e) override;

private:
    Qt::Orientation m_orientation;
};

QT_END_NAMESPACE

#endif // QDYNAMICTOOLBAREXTENSION_P_H
