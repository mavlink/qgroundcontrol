// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of Qt Designer.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//


#ifndef SPACER_WIDGET_H
#define SPACER_WIDGET_H

#include "shared_global_p.h"

#include <QtWidgets/qwidget.h>
#include <QtWidgets/qsizepolicy.h>

QT_BEGIN_NAMESPACE

class QDesignerFormWindowInterface;

class QDESIGNER_SHARED_EXPORT Spacer: public QWidget
{
    Q_OBJECT

    // Special hack: Make name appear as "spacer name"
    Q_PROPERTY(QString spacerName  READ objectName WRITE setObjectName)
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation)
    Q_PROPERTY(QSizePolicy::Policy sizeType READ sizeType WRITE setSizeType)
    Q_PROPERTY(QSize sizeHint READ sizeHintProperty WRITE setSizeHintProperty DESIGNABLE true STORED true)

public:
    Spacer(QWidget *parent = nullptr);

    QSize sizeHint() const override;

    QSize sizeHintProperty() const;
    void setSizeHintProperty(const QSize &s);

    QSizePolicy::Policy sizeType() const;
    void setSizeType(QSizePolicy::Policy t);

    Qt::Alignment alignment() const;
    Qt::Orientation orientation() const;

    void setOrientation(Qt::Orientation o);
    void setInteractiveMode(bool b) { m_interactive = b; };

    bool event(QEvent *e) override;

protected:
    void paintEvent(QPaintEvent *e) override;
    void resizeEvent(QResizeEvent* e) override;
    void updateMask();

private:
    bool isInLayout() const;
    void updateToolTip();

    const QSize m_SizeOffset = QSize(3, 3); // A small offset to ensure the spacer is still visible when reset to size 0,0
    QDesignerFormWindowInterface *m_formWindow;
    Qt::Orientation m_orientation = Qt::Vertical;
    bool m_interactive = true;
    // Cache information about 'being in layout' which is expensive to calculate.
    enum LayoutState { InLayout, OutsideLayout, UnknownLayoutState };
    mutable LayoutState m_layoutState = UnknownLayoutState;
    QSize m_sizeHint = QSize(0, 0);
};

QT_END_NAMESPACE

#endif // SPACER_WIDGET_H
