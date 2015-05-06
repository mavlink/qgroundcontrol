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

#include "uas/FileManager.h"
#include "ui_QGCUASFileView.h"

class QGCUASFileView : public QWidget
{
    Q_OBJECT

public:
    explicit QGCUASFileView(QWidget *parent, FileManager *manager);

protected:
    FileManager* _manager;
    
private slots:
    void _refreshTree(void);
    void _listEntryReceived(const QString& entry);
    void _listErrorMessage(const QString& msg);
    void _listComplete(void);
    
    void _downloadFile(void);
    void _uploadFile(void);
    void _downloadLength(unsigned int length);
    void _downloadProgress(unsigned int length);
    void _downloadErrorMessage(const QString& msg);
    void _downloadComplete(void);

    void _currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);

private:
    void _connectDownloadSignals(void);
    void _disconnectDownloadSignals(void);
    void _connectListSignals(void);
    void _disconnectListSignals(void);
    void _requestDirectoryList(const QString& dir);

    static const int        _typeFile = QTreeWidgetItem::UserType + 1;
    static const int        _typeDir = QTreeWidgetItem::UserType + 2;
    static const int        _typeError = QTreeWidgetItem::UserType + 3;
    
    QList<int>              _walkIndexStack;
    QList<QTreeWidgetItem*> _walkItemStack;
    Ui::QGCUASFileView      _ui;
    
    QString _downloadFilename;  ///< File currently being downloaded, not including path
    QTime   _downloadStartTime; ///< Time at which download started
    
    bool _listInProgress;       ///< Indicates that a listDirectory command is in progress
    bool _downloadInProgress;   ///< Indicates that a downloadPath command is in progress
    bool _uploadInProgress;     ///< Indicates that a upload command is in progress
};

#endif // QGCUASFILEVIEW_H
