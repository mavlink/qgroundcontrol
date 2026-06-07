// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef PLUGINDIALOG_H
#define PLUGINDIALOG_H

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

#include "ui_plugindialog.h"

QT_BEGIN_NAMESPACE

class QFileInfo;

class QDesignerFormEditorInterface;

namespace qdesigner_internal {

class PluginDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PluginDialog(QDesignerFormEditorInterface *core, QWidget *parent = nullptr);

private slots:
    void updateCustomWidgetPlugins();
    void treeWidgetContextMenu(const QPoint &pos);

private:
    void populateTreeWidget();
    QTreeWidgetItem* setTopLevelItem(const QString &itemName);
    QTreeWidgetItem* setPluginItem(QTreeWidgetItem *topLevelItem,
                                   const QFileInfo &file, const QFont &font);
    QTreeWidgetItem *setItem(QTreeWidgetItem *pluginItem, const QString &name,
                             const QString &toolTip, const QString &whatsThis,
                             const QIcon &icon);

    QDesignerFormEditorInterface *m_core;
    QT_PREPEND_NAMESPACE(Ui)::PluginDialog ui;
    QIcon interfaceIcon;
    QIcon featureIcon;
};

}

QT_END_NAMESPACE

#endif
