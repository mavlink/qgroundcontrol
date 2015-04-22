/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include "MainWindow.h"

namespace Ui
{
    class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:

    enum {
        ShowDefault,
        ShowCommLinks,
        ShowControllers,
        ShowMavlink
    };

#ifdef __mobile__
    SettingsDialog(QWidget *parent = 0, int showTab = ShowDefault, Qt::WindowFlags flags = Qt::Sheet);
#else
    SettingsDialog(JoystickInput *joystick, QWidget *parent = 0, int showTab = ShowDefault, Qt::WindowFlags flags = Qt::Sheet);
#endif
    ~SettingsDialog();

public slots:
    void styleChanged(int index);

private slots:
    void _deleteSettingsToggled(bool checked);
    void _selectSavedFilesDirectory(void);
    void _validateBeforeClose(void);

    void on_showGPS_clicked(bool checked);
    void on_showBattery_clicked(bool checked);
    void on_showMessages_clicked(bool checked);
    void on_showMav_clicked(bool checked);

    void on_showRSSI_clicked(bool checked);

private:
    MainWindow*         _mainWindow;
    Ui::SettingsDialog* _ui;
};

#endif
