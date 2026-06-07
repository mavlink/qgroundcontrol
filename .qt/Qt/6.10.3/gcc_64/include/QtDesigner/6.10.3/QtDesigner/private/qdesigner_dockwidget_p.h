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

#ifndef QDESIGNER_DOCKWIDGET_H
#define QDESIGNER_DOCKWIDGET_H

#include "shared_global_p.h"

#include <qdesigner_propertysheet_p.h>

#include <QtWidgets/qdockwidget.h>

QT_BEGIN_NAMESPACE

class QDesignerFormWindowInterface;

class QDESIGNER_SHARED_EXPORT QDesignerDockWidget: public QDockWidget
{
    Q_OBJECT
    Q_PROPERTY(Qt::DockWidgetArea dockWidgetArea READ dockWidgetArea WRITE setDockWidgetArea DESIGNABLE true STORED false)
    Q_PROPERTY(bool docked READ docked WRITE setDocked DESIGNABLE true STORED false)
public:
    QDesignerDockWidget(QWidget *parent = nullptr);
    ~QDesignerDockWidget() override;

    bool docked() const;
    void setDocked(bool b);

    Qt::DockWidgetArea dockWidgetArea() const;
    void setDockWidgetArea(Qt::DockWidgetArea dockWidgetArea);

    bool inMainWindow() const;

private:
    QDesignerFormWindowInterface *formWindow() const;
    QMainWindow *findMainWindow() const;
};

class QDESIGNER_SHARED_EXPORT QDockWidgetPropertySheet : public QDesignerPropertySheet
{
     Q_OBJECT
public:
    using QDesignerPropertySheet::QDesignerPropertySheet;

    bool isEnabled(int index) const override;
};

using QDockWidgetPropertySheetFactory =
    QDesignerPropertySheetFactory<QDockWidget, QDockWidgetPropertySheet>;

QT_END_NAMESPACE

#endif // QDESIGNER_DOCKWIDGET_H
