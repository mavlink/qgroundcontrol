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

#ifndef QDESIGNER_WIDGET_H
#define QDESIGNER_WIDGET_H

#include "shared_global_p.h"
#include <QtWidgets/qdialog.h>
#include <QtWidgets/qlabel.h>

QT_BEGIN_NAMESPACE

class QDesignerFormWindowInterface;

namespace qdesigner_internal {
    class FormWindowBase;
}

class QDESIGNER_SHARED_EXPORT QDesignerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit QDesignerWidget(QDesignerFormWindowInterface* formWindow, QWidget *parent = nullptr);
    ~QDesignerWidget() override;

    QDesignerFormWindowInterface* formWindow() const;

    void updatePixmap();

    QSize minimumSizeHint() const override
    { return QWidget::minimumSizeHint().expandedTo(QSize(16, 16)); }

protected:
    void paintEvent(QPaintEvent *e) override;

private:
    qdesigner_internal::FormWindowBase* m_formWindow;
};

class QDESIGNER_SHARED_EXPORT QDesignerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit QDesignerDialog(QDesignerFormWindowInterface *fw, QWidget *parent);

    QSize minimumSizeHint() const override
    { return QDialog::minimumSizeHint().expandedTo(QSize(16, 16)); }

protected:
    void paintEvent(QPaintEvent *e) override;

private:
    qdesigner_internal::FormWindowBase* m_formWindow;
};

class QDESIGNER_SHARED_EXPORT Line : public QFrame
{
    Q_OBJECT
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation)
public:
    explicit Line(QWidget *parent) : QFrame(parent)
    { setAttribute(Qt::WA_MouseNoMask); setFrameStyle(HLine | Sunken); }

    inline void setOrientation(Qt::Orientation orient)
    { setFrameShape(orient == Qt::Horizontal ? HLine : VLine); }

    inline Qt::Orientation orientation() const
    { return frameShape() == HLine ? Qt::Horizontal : Qt::Vertical; }
};

QT_END_NAMESPACE

#endif // QDESIGNER_WIDGET_H
