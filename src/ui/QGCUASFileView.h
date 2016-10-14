/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
