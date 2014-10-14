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

// FIXME: Get rid of this
class QGCFirmwareUpgradeWorker;

class PX4FirmwareUpgrade : public QWidget
{
    Q_OBJECT

public:
    explicit PX4FirmwareUpgrade(QWidget *parent = 0);
    ~PX4FirmwareUpgrade();

signals:
    void filenameSet(QString filename);
    void connectLinks();
    void disconnectLinks();

protected:
    void changeEvent(QEvent *e);
    void loadSettings();
    void storeSettings();
    void updateBoardId(const QString &fileName);

public slots:
    void onPortAddedOrRemoved();
    void onLoadStart();
    void onLoadFinished(bool success);
    void onUserAbort();
    void onDetectFinished(bool success, int board_id, const QString &boardName, const QString &bootLoader);
    void onFlashURL(const QString &url);
    void onDownloadProgress(qint64 curr, qint64 total);

private slots:
    void _selectAdvancedFirmwareFilename(void);
    void onCancelButtonClicked();
    void onUploadButtonClicked();
    void _scanForBoard(void);
    void onDownloadFinished();
    void onDownloadRequested(const QNetworkRequest &request);
    void onLinkClicked(const QUrl&);
    void _showAdvancedMode(bool show);

private:
    bool                        _loading;
    Ui::PX4FirmwareUpgrade      _ui;
    QTimer*                     _timer;
    QGCFirmwareUpgradeWorker*   _worker;
    QWidget*                    _boardFoundWidget;

    QString                     _advancedFirmwareFilename;  ///< File to flash when using advanced mode
    static const char*          _settingsGroup;
};

#endif // PX4FirmwareUpgrade_H
