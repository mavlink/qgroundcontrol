/**
 ******************************************************************************
 *
 * @file       fancymainwindow.h
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

#ifndef FANCYMAINWINDOW_H
#define FANCYMAINWINDOW_H

#include "utils_global.h"

#include <QtCore/QList>
#include <QtCore/QHash>

#include <QtGui/QMainWindow>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Utils {

class QTCREATOR_UTILS_EXPORT FancyMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    FancyMainWindow(QWidget *parent = 0);

    QDockWidget *addDockForWidget(QWidget *widget);
    QList<QDockWidget *> dockWidgets() const { return m_dockWidgets; }

    void setTrackingEnabled(bool enabled);
    bool isLocked() const { return m_locked; }

    void saveSettings(QSettings *settings) const;
    void restoreSettings(QSettings *settings);
    QHash<QString, QVariant> saveSettings() const;
    void restoreSettings(const QHash<QString, QVariant> &settings);

public slots:
    void setLocked(bool locked);

protected:
    void hideEvent(QHideEvent *event);
    void showEvent(QShowEvent *event);

private slots:
    void onDockActionTriggered();
    void onDockVisibilityChange(bool);
    void onTopLevelChanged();

private:
    void updateDockWidget(QDockWidget *dockWidget);
    void handleVisibilityChanged(bool visible);

    QList<QDockWidget *> m_dockWidgets;
    QList<bool> m_dockWidgetActiveState;
    bool m_locked;
    bool m_handleDockVisibilityChanges; //todo
};

} // namespace Utils

#endif // FANCYMAINWINDOW_H
