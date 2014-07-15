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

#include "uas/QGCUASFileManager.h"
#include "ui_QGCUASFileView.h"

class QGCUASFileView : public QWidget
{
    Q_OBJECT

public:
    explicit QGCUASFileView(QWidget *parent, QGCUASFileManager *manager);

protected:
    QGCUASFileManager* _manager;
    
private slots:
    void _refreshTree(void);
    void _downloadFiles(void);
    void _treeStatusMessage(const QString& msg);
    void _treeErrorMessage(const QString& msg);
    void _listComplete(void);
    void _downloadStatusMessage(const QString& msg);
    void _currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);
    void _listCompleteTimeout(void);

private:
    void _setupListCompleteTimeout(void);
    void _clearListCompleteTimeout(void);
    void _requestDirectoryList(const QString& dir);

    static const int        _typeFile = QTreeWidgetItem::UserType + 1;
    static const int        _typeDir = QTreeWidgetItem::UserType + 2;
    static const int        _typeError = QTreeWidgetItem::UserType + 3;
    
    QTimer                  _listCompleteTimer;                     ///> Used to signal a timeout waiting for a listComplete signal
    static const int        _listCompleteTimerTimeoutMsecs = 5000;  ///> Timeout in msecs for listComplete timer
    
    QList<int>              _walkIndexStack;
    QList<QTreeWidgetItem*> _walkItemStack;
    Ui::QGCUASFileView      _ui;
};

#endif // QGCUASFILEVIEW_H
