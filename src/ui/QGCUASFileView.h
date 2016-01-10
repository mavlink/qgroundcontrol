/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

#ifndef QGCUASFILEVIEW_H
#define QGCUASFILEVIEW_H

#include <QWidget>
#include <QTreeWidgetItem>

#include "Vehicle.h"
#include "uas/FileManager.h"
#include "ui_QGCUASFileView.h"

class QGCUASFileView : public QWidget
{
    Q_OBJECT

public:
    explicit QGCUASFileView(QWidget *parent, Vehicle* vehicle);

protected:
    FileManager* _manager;
    
private slots:
    void _listEntryReceived(const QString& entry);
    
    void _refreshTree(void);
    void _downloadFile(void);
    void _uploadFile(void);
    
    void _commandProgress(int value);
    void _commandError(const QString& msg);
    void _commandComplete(void);

    void _currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);

private:
    void _listComplete(void);
    void _requestDirectoryList(const QString& dir);
    void _setAllButtonsEnabled(bool enabled);

    static const int        _typeFile = QTreeWidgetItem::UserType + 1;
    static const int        _typeDir = QTreeWidgetItem::UserType + 2;
    static const int        _typeError = QTreeWidgetItem::UserType + 3;
    
    QList<int>              _walkIndexStack;
    QList<QTreeWidgetItem*> _walkItemStack;
    Ui::QGCUASFileView      _ui;
    
    enum CommandState {
        commandNone,        ///< No command active
        commandList,        ///< List command active
        commandDownload,    ///< Download command active
        commandUpload       ///< Upload command active
    };
    
    CommandState _currentCommand;   ///< Current active command
};

#endif // QGCUASFILEVIEW_H
