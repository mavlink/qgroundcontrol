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

#include "QGCUASFileView.h"
#include "uas/QGCUASFileManager.h"

#include <QFileDialog>
#include <QDir>
#include <QMessageBox>

QGCUASFileView::QGCUASFileView(QWidget *parent, QGCUASFileManager *manager) :
    QWidget(parent),
    _manager(manager)
{
    _ui.setupUi(this);

    bool success = connect(_ui.listFilesButton, SIGNAL(clicked()), this, SLOT(_refreshTree()));
    Q_ASSERT(success);
    success = connect(_ui.downloadButton, SIGNAL(clicked()), this, SLOT(_downloadFiles()));
    Q_ASSERT(success);
    success = connect(_ui.treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(_currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
    Q_ASSERT(success);
    Q_UNUSED(success);
}

void QGCUASFileView::_downloadFiles(void)
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Download Directory"),
                                                     QDir::homePath(),
                                                     QFileDialog::ShowDirsOnly
                                                     | QFileDialog::DontResolveSymlinks);
    // And now download to this location
    QString path;
    QTreeWidgetItem* item = _ui.treeWidget->currentItem();
    if (item && item->type() == _typeFile) {
        do {
            path.prepend("/" + item->text(0));
            item = item->parent();
        } while (item);
        qDebug() << "Download: " << path;
        
        bool success = connect(_manager, SIGNAL(statusMessage(QString)), this, SLOT(_downloadStatusMessage(QString)));
        Q_ASSERT(success);
        success = connect(_manager, SIGNAL(errorMessage(QString)), this, SLOT(_downloadStatusMessage(QString)));
        Q_ASSERT(success);
        Q_UNUSED(success);
        _manager->downloadPath(path, QDir(dir));
    }
}

void QGCUASFileView::_refreshTree(void)
{
    _ui.treeWidget->clear();

    _walkIndexStack.clear();
    _walkItemStack.clear();
    _walkIndexStack.append(0);
    _walkItemStack.append(_ui.treeWidget->invisibleRootItem());
    
    bool success = connect(_manager, SIGNAL(statusMessage(QString)), this, SLOT(_treeStatusMessage(QString)));
    Q_ASSERT(success);
    success = connect(_manager, SIGNAL(errorMessage(QString)), this, SLOT(_treeErrorMessage(QString)));
    Q_ASSERT(success);
    success = connect(_manager, SIGNAL(listComplete(void)), this, SLOT(_listComplete(void)));
    Q_ASSERT(success);
    Q_UNUSED(success);
    
    // Don't queue up more than once
    _ui.listFilesButton->setEnabled(false);

    qDebug() << "List: /";
    _manager->listDirectory("/");
}

void QGCUASFileView::_treeStatusMessage(const QString& msg)
{
    int type;
    if (msg.startsWith("F")) {
        type = _typeFile;
    } else if (msg.startsWith("D")) {
        type = _typeDir;
        if (msg == "D." || msg == "D..") {
            return;
        }
    } else {
        Q_ASSERT(false);
        return; // Silence maybe-unitialized on type below
    }

    QTreeWidgetItem* item;
    item = new QTreeWidgetItem(_walkItemStack.last(), type);
    Q_CHECK_PTR(item);
    
    item->setText(0, msg.right(msg.size() - 1));
}

void QGCUASFileView::_treeErrorMessage(const QString& msg)
{
    QTreeWidgetItem* item;
    item = new QTreeWidgetItem(_walkItemStack.last(), _typeError);
    Q_CHECK_PTR(item);
    
    item->setText(0, tr("Error: ") + msg);
    
    // Fake listComplete signal after an error
    _listComplete();
}

void QGCUASFileView::_listComplete(void)
{
    // Walk the current items, traversing down into directories
    
Again: 
    int walkIndex = _walkIndexStack.last();
    QTreeWidgetItem* parentItem = _walkItemStack.last();
    QTreeWidgetItem* childItem = parentItem->child(walkIndex);

    // Loop until we hit a directory
    while (childItem && childItem->type() != _typeDir) {
        // Move to next index at current level
        _walkIndexStack.last() = ++walkIndex;
        childItem = parentItem->child(walkIndex);
    }
    
    if (childItem) {
        // Process this item
        QString text = childItem->text(0);
        
        // Move to the next item for processing at this level
        _walkIndexStack.last() = ++walkIndex;
        
        // Push this new directory on the stack
        _walkItemStack.append(childItem);
        _walkIndexStack.append(0);
        
        // Ask for the directory list
        QString dir;
        for (int i=1; i<_walkItemStack.count(); i++) {
            QTreeWidgetItem* item = _walkItemStack[i];
            dir.append("/" + item->text(0));
        }
        qDebug() << "List:" << dir;
        _manager->listDirectory(dir);
    } else {
        // We have run out of items at the this level, pop the stack and keep going at that level
        _walkIndexStack.removeLast();
        _walkItemStack.removeLast();
        if (_walkIndexStack.count() != 0) {
            goto Again;
        } else {
            disconnect(_manager, SIGNAL(statusMessage(QString)), this, SLOT(_treeStatusMessage(QString)));
            disconnect(_manager, SIGNAL(errorMessage(QString)), this, SLOT(_treeErrorMessage(QString)));
            disconnect(_manager, SIGNAL(listComplete(void)), this, SLOT(_listComplete(void)));
            _ui.listFilesButton->setEnabled(true);
        }
    }
}

void QGCUASFileView::_downloadStatusMessage(const QString& msg)
{
    disconnect(_manager, SIGNAL(statusMessage(QString)), this, SLOT(_downloadStatusMessage(QString)));
    disconnect(_manager, SIGNAL(errorMessage(QString)), this, SLOT(_downloadStatusMessage(QString)));
    
    QMessageBox msgBox;
    msgBox.setWindowModality(Qt::ApplicationModal);
    msgBox.setText(msg);
    msgBox.exec();
}

void QGCUASFileView::_currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous)
{
    Q_UNUSED(previous);
    _ui.downloadButton->setEnabled(current ? (current->type() == _typeFile) : false);
}
