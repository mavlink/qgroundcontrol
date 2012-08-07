/**
 ******************************************************************************
 *
 * @file       fancymainwindow.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 *             Parts by Nokia Corporation (qt-info@nokia.com) Copyright (C) 2009.
 * @brief      
 * @see        The GNU Public License (GPL) Version 3
 * @defgroup   
 * @{
 * 
 *****************************************************************************/
/* 
 * This program is free software; you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation; either version 3 of the License, or 
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License 
 * for more details.
 * 
 * You should have received a copy of the GNU General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "fancymainwindow.h"

#include <QtGui/QAction>
#include <QtCore/QSettings>
#include <QtGui/QDockWidget>
#include <QtCore/QSettings>


using namespace Utils;

FancyMainWindow::FancyMainWindow(QWidget *parent)
    : QMainWindow(parent),
    m_locked(true),
    m_handleDockVisibilityChanges(true)
{
}

QDockWidget *FancyMainWindow::addDockForWidget(QWidget *widget)
{
    QDockWidget *dockWidget = new QDockWidget(widget->windowTitle(), this);
    dockWidget->setObjectName(widget->windowTitle());
    dockWidget->setWidget(widget);
    connect(dockWidget->toggleViewAction(), SIGNAL(triggered()),
        this, SLOT(onDockActionTriggered()), Qt::QueuedConnection);
    connect(dockWidget, SIGNAL(visibilityChanged(bool)),
            this, SLOT(onDockVisibilityChange(bool)));
    connect(dockWidget, SIGNAL(topLevelChanged(bool)),
            this, SLOT(onTopLevelChanged()));
    m_dockWidgets.append(dockWidget);
    m_dockWidgetActiveState.append(true);
    updateDockWidget(dockWidget);
    return dockWidget;
}

void FancyMainWindow::updateDockWidget(QDockWidget *dockWidget)
{
    const QDockWidget::DockWidgetFeatures features =
            (m_locked) ? QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable
                       : QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable;
    QWidget *titleBarWidget = dockWidget->titleBarWidget();
    if (m_locked && !titleBarWidget && !dockWidget->isFloating())
        titleBarWidget = new QWidget(dockWidget);
    else if ((!m_locked || dockWidget->isFloating()) && titleBarWidget) {
        delete titleBarWidget;
        titleBarWidget = 0;
    }
    dockWidget->setTitleBarWidget(titleBarWidget);
    dockWidget->setFeatures(features);
}

void FancyMainWindow::onDockActionTriggered()
{
    QDockWidget *dw = qobject_cast<QDockWidget *>(sender()->parent());
    if (dw) {
        if (dw->isVisible())
            dw->raise();
    }
}

void FancyMainWindow::onDockVisibilityChange(bool visible)
{
    if (!m_handleDockVisibilityChanges)
        return;
    QDockWidget *dockWidget = qobject_cast<QDockWidget *>(sender());
    int index = m_dockWidgets.indexOf(dockWidget);
    m_dockWidgetActiveState[index] = visible;
}

void FancyMainWindow::onTopLevelChanged()
{
    updateDockWidget(qobject_cast<QDockWidget*>(sender()));
}

void FancyMainWindow::setTrackingEnabled(bool enabled)
{
    if (enabled) {
        m_handleDockVisibilityChanges = true;
        for (int i = 0; i < m_dockWidgets.size(); ++i)
            m_dockWidgetActiveState[i] = m_dockWidgets[i]->isVisible();
    } else {
        m_handleDockVisibilityChanges = false;
    }
}

void FancyMainWindow::setLocked(bool locked)
{
    m_locked = locked;
    foreach (QDockWidget *dockWidget, m_dockWidgets) {
        updateDockWidget(dockWidget);
    }
}

void FancyMainWindow::hideEvent(QHideEvent *event)
{
    Q_UNUSED(event)
    handleVisibilityChanged(false);
}

void FancyMainWindow::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)
    handleVisibilityChanged(true);
}

void FancyMainWindow::handleVisibilityChanged(bool visible)
{
    m_handleDockVisibilityChanges = false;
    for (int i = 0; i < m_dockWidgets.size(); ++i) {
        QDockWidget *dockWidget = m_dockWidgets.at(i);
        if (dockWidget->isFloating()) {
            dockWidget->setVisible(visible && m_dockWidgetActiveState.at(i));
        }
    }
    if (visible)
        m_handleDockVisibilityChanges = true;
}

void FancyMainWindow::saveSettings(QSettings *settings) const
{
    QHash<QString, QVariant> hash = saveSettings();
    QHashIterator<QString, QVariant> it(hash);
    while (it.hasNext()) {
        it.next();
        settings->setValue(it.key(), it.value());
    }
}

void FancyMainWindow::restoreSettings(QSettings *settings)
{
    QHash<QString, QVariant> hash;
    foreach (const QString &key, settings->childKeys()) {
        hash.insert(key, settings->value(key));
    }
    restoreSettings(hash);
}

QHash<QString, QVariant> FancyMainWindow::saveSettings() const
{
    QHash<QString, QVariant> settings;
    settings["State"] = saveState();
    settings["Locked"] = m_locked;
    for (int i = 0; i < m_dockWidgetActiveState.count(); ++i) {
        settings[m_dockWidgets.at(i)->objectName()] =
                           m_dockWidgetActiveState.at(i);
    }
    return settings;
}

void FancyMainWindow::restoreSettings(const QHash<QString, QVariant> &settings)
{
    QByteArray ba = settings.value("State", QByteArray()).toByteArray();
    if (!ba.isEmpty())
        restoreState(ba);
    m_locked = settings.value("Locked", true).toBool();
    for (int i = 0; i < m_dockWidgetActiveState.count(); ++i) {
        m_dockWidgetActiveState[i] = settings.value(m_dockWidgets.at(i)->objectName(), false).toBool();
    }
}
