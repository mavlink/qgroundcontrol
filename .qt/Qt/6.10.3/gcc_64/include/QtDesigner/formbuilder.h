// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef FORMBUILDER_H
#define FORMBUILDER_H

#if 0
#  pragma qt_class(QFormBuilder)
#  pragma qt_sync_skip_header_check
#endif

#include "uilib_global.h"
#include "abstractformbuilder.h"

QT_BEGIN_NAMESPACE

class QDesignerCustomWidgetInterface;

#ifdef QFORMINTERNAL_NAMESPACE
namespace QFormInternal
{
#endif

class QDESIGNER_UILIB_EXPORT QFormBuilder: public QAbstractFormBuilder
{
public:
    QFormBuilder();
    ~QFormBuilder() override;

    QStringList pluginPaths() const;

    void clearPluginPaths();
    void addPluginPath(const QString &pluginPath);
    void setPluginPath(const QStringList &pluginPaths);

    QList<QDesignerCustomWidgetInterface*> customWidgets() const;

protected:
    QWidget *create(DomUI *ui, QWidget *parentWidget) override;
    QWidget *create(DomWidget *ui_widget, QWidget *parentWidget) override;
    QLayout *create(DomLayout *ui_layout, QLayout *layout, QWidget *parentWidget) override;
    QLayoutItem *create(DomLayoutItem *ui_layoutItem, QLayout *layout, QWidget *parentWidget) override;
    QAction *create(DomAction *ui_action, QObject *parent) override;
    QActionGroup *create(DomActionGroup *ui_action_group, QObject *parent) override;

    QWidget *createWidget(const QString &widgetName, QWidget *parentWidget, const QString &name) override;
    QLayout *createLayout(const QString &layoutName, QObject *parent, const QString &name) override;

    void createConnections(DomConnections *connections, QWidget *widget) override;

    bool addItem(DomLayoutItem *ui_item, QLayoutItem *item, QLayout *layout) override;
    bool addItem(DomWidget *ui_widget, QWidget *widget, QWidget *parentWidget) override;

    virtual void updateCustomWidgets();
    void applyProperties(QObject *o, const QList<DomProperty*> &properties) override;

    static QWidget *widgetByName(QWidget *topLevel, const QString &name);

private:
};

#ifdef QFORMINTERNAL_NAMESPACE
}
#endif

QT_END_NAMESPACE

#endif // FORMBUILDER_H
