/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009, 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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


/// @file
///     @brief PX4 Firmware Upgrade UI
///     @author Don Gagne <don@thegagnes.com>

#ifndef PX4FirmwareUpgrade_H
#define PX4FirmwareUpgrade_H

#include <QWidget>
#include <QUrl>
#include <QSerialPort>
#include <QTimer>

#include "ui_PX4FirmwareUpgrade.h"

// FIXME: Get rid of this
class QGCFirmwareUpgradeWorker;

class PX4FirmwareUpgrade : public QWidget
{
    Q_OBJECT

public:
    explicit PX4FirmwareUpgrade(QWidget *parent = 0);

signals:
    void connectLinks(void);
    void disconnectLinks(void);

private slots:
    void _nextStep(void);
    void _cancelUpgrade(void);

private:
    /// @brief The various states that the upgrade process progresses through.
    enum upgradeStates {
        upgradeStateBegin,
        upgradeStateBoardFound,
        upgradeStateBoardNotFound,
        upgradeStateBootloaderFound,
        upgradeStateFirmwareSelected,
        upgradeStateFirmwareDownloaded,
        upgradeStateBoardUpgraded,
    };
    
    void _updateStateUI(enum upgradeStates newUpgradeState);
    void _setBoardIcon(int boardID);
    void _setFirmwareCombo(int boardID);
    bool _findBoard(void);
    
    enum upgradeStates _upgradeState;
    
    QString _port;
    QString _portDescription;
    int     _bootloaderVersion;
    int     _boardID;

    QPixmap _boardIcon;
    
    Ui::PX4FirmwareUpgrade      _ui;
};

#endif // PX4FirmwareUpgrade_H
