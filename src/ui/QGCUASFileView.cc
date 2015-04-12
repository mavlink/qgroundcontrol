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
#include "uas/FileManager.h"
#include "QGCFileDialog.h"

#include <QFileDialog>
#include <QDir>
#include <QDebug>

QGCUASFileView::QGCUASFileView(QWidget *parent, FileManager *manager) :
    QWidget(parent),
    _manager(manager),
    _listInProgress(false),
    _downloadInProgress(false)
{
    _ui.setupUi(this);
    
    // Progress bar is only visible while a download is in progress
    _ui.progressBar->setVisible(false);

    bool success;
    Q_UNUSED(success);    // Silence retail unused variable error
    
    // Connect UI signals
    success = connect(_ui.listFilesButton, SIGNAL(clicked()), this, SLOT(_refreshTree()));
    Q_ASSERT(success);
    success = connect(_ui.downloadButton, SIGNAL(clicked()), this, SLOT(_downloadFile()));
    Q_ASSERT(success);
    success = connect(_ui.treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(_currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
    Q_ASSERT(success);
}

/// @brief Downloads the file currently selected in the tree view
void QGCUASFileView::_downloadFile(void)
{
    Q_ASSERT(!_downloadInProgress);
    
    _ui.statusText->clear();
    
    QString downloadToHere = QGCFileDialog::getExistingDirectory(this, tr("Download Directory"),
                                                               QDir::homePath(),
                                                               QGCFileDialog::ShowDirsOnly
                                                               | QGCFileDialog::DontResolveSymlinks);
    
    // And now download to this location
    QString path;
    QTreeWidgetItem* item = _ui.treeWidget->currentItem();
    if (item && item->type() == _typeFile) {
        _downloadFilename.clear();
        do {
            QString name = item->text(0).split("\t")[0];    // Strip off file sizes
            
            // If this is the file name and not a directory keep track of the download file name
            if (_downloadFilename.isEmpty()) {
                _downloadFilename = name;
            }
            
            path.prepend("/" + name);
            item = item->parent();
        } while (item);
        
        _ui.downloadButton->setEnabled(false);
        _downloadInProgress = true;
        _connectDownloadSignals();
        
        _manager->streamPath(path, QDir(downloadToHere));
    }
}

/// @brief Called when length of file being downloaded is known.
void QGCUASFileView::_downloadLength(unsigned int length)
{
    Q_ASSERT(_downloadInProgress);
    
    // Setup the progress bar
    QProgressBar* bar = _ui.progressBar;
    bar->setMinimum(0);
    bar->setMaximum(length);
    bar->setValue(0);
    bar->setVisible(true);
    
    _ui.downloadButton->setEnabled(true);
    _downloadStartTime.start();
    
    _ui.statusText->setText(tr("Downloading: %1").arg(_downloadFilename));
}

/// @brief Called to update the progress of the download.
///     @param bytesReceived Current count of bytes received for current download
void QGCUASFileView::_downloadProgress(unsigned int bytesReceived)
{
    static uint lastSecsReported = 0;
    
    Q_ASSERT(_downloadInProgress);
    
    _ui.progressBar->setValue(bytesReceived);
    
    // Calculate and display download rate. Only update once per second.
    uint kbReceived = bytesReceived / 1024;
    uint secs = _downloadStartTime.elapsed() / 1000;
    if (secs != 0) {
        uint kbPerSec = kbReceived / secs;
        if (kbPerSec != 0 && secs != lastSecsReported) {
            lastSecsReported = secs;
            _ui.statusText->setText(tr("Downloading: %1 %2 KB/sec").arg(_downloadFilename).arg(kbPerSec));
        }
    }
}

/// @brief Called when the download associated with the FileManager::downloadPath command completes.
void QGCUASFileView::_downloadComplete(void)
{
    Q_ASSERT(_downloadInProgress);
    _ui.downloadButton->setEnabled(true);
    _ui.progressBar->setVisible(false);
    _downloadInProgress = false;
    _disconnectDownloadSignals();
    _ui.statusText->setText(tr("Download complete: %1").arg(_downloadFilename));
}

/// @brief Called when an error occurs during a download.
///     @param msg Error message
void QGCUASFileView::_downloadErrorMessage(const QString& msg)
{
    if (_downloadInProgress) {
        _ui.downloadButton->setEnabled(true);
        _ui.progressBar->setVisible(false);
        _downloadInProgress = false;
        _disconnectDownloadSignals();
        _ui.statusText->setText(tr("Error: ") + msg);
    }
}

/// @brief Refreshes the directory list tree.
void QGCUASFileView::_refreshTree(void)
{
    Q_ASSERT(!_listInProgress);
    
    _ui.treeWidget->clear();
    _ui.statusText->clear();

    _walkIndexStack.clear();
    _walkItemStack.clear();
    _walkIndexStack.append(0);
    _walkItemStack.append(_ui.treeWidget->invisibleRootItem());
    
    // Don't queue up more than once
    _ui.listFilesButton->setEnabled(false);
    _listInProgress = true;
    _connectListSignals();

    _requestDirectoryList("/");
}

/// @brief Adds the specified directory entry to the tree view.
void QGCUASFileView::_listEntryReceived(const QString& entry)
{
    Q_ASSERT(_listInProgress);
    
    int type;
    if (entry.startsWith("F")) {
        type = _typeFile;
    } else if (entry.startsWith("D")) {
        type = _typeDir;
        if (entry == "D." || entry == "D..") {
            return;
        }
    } else {
        Q_ASSERT(false);
        return; // Silence maybe-unitialized on type
    }

    QTreeWidgetItem* item;
    item = new QTreeWidgetItem(_walkItemStack.last(), type);
    Q_CHECK_PTR(item);
    
    item->setText(0, entry.right(entry.size() - 1));
}

/// @brief Called when an error occurs during a directory listing.
///     @param msg Error message
void QGCUASFileView::_listErrorMessage(const QString& msg)
{
    if (_listInProgress) {
        _ui.listFilesButton->setEnabled(true);
        _listInProgress = false;
        _disconnectListSignals();
        _ui.statusText->setText(tr("Error: ") + msg);
    }
}

void QGCUASFileView::_listComplete(void)
{
    Q_ASSERT(_listInProgress);

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
        _requestDirectoryList(dir);
    } else {
        // We have run out of items at the this level, pop the stack and keep going at that level
        _walkIndexStack.removeLast();
        _walkItemStack.removeLast();
        if (_walkIndexStack.count() != 0) {
            goto Again;
        } else {
            _ui.listFilesButton->setEnabled(true);
            _listInProgress = false;
            _disconnectListSignals();
        }
    }
}

void QGCUASFileView::_currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous)
{
    Q_UNUSED(previous);
    // FIXME: Should not enable when downloading
    _ui.downloadButton->setEnabled(current ? (current->type() == _typeFile) : false);
}

void QGCUASFileView::_requestDirectoryList(const QString& dir)
{
    qDebug() << "List:" << dir;
    _manager->listDirectory(dir);
}

/// @brief Connects to the signals associated with the FileManager::downloadPath method. We only leave these signals connected
/// while a download because there may be multiple UAS, which in turn means multiple QGCUASFileView instances. We only want the signals
/// connected to the active FileView which is doing the current download.
void QGCUASFileView::_connectDownloadSignals(void)
{
    bool success;
    Q_UNUSED(success);    // Silence retail unused variable error

    success = connect(_manager, SIGNAL(downloadFileLength(unsigned int)), this, SLOT(_downloadLength(unsigned int)));
    Q_ASSERT(success);
    success = connect(_manager, SIGNAL(downloadFileProgress(unsigned int)), this, SLOT(_downloadProgress(unsigned int)));
    Q_ASSERT(success);
    success = connect(_manager, SIGNAL(downloadFileComplete(void)), this, SLOT(_downloadComplete(void)));
    Q_ASSERT(success);
    success = connect(_manager, SIGNAL(errorMessage(const QString&)), this, SLOT(_downloadErrorMessage(const QString&)));
    Q_ASSERT(success);
}

void QGCUASFileView::_disconnectDownloadSignals(void)
{
    disconnect(_manager, SIGNAL(downloadFileLength(unsigned int)), this, SLOT(_downloadLength(unsigned int)));
    disconnect(_manager, SIGNAL(downloadFileProgress(unsigned int)), this, SLOT(_downloadProgress(unsigned int)));
    disconnect(_manager, SIGNAL(downloadFileComplete(void)), this, SLOT(_downloadComplete(void)));
    disconnect(_manager, SIGNAL(errorMessage(const QString&)), this, SLOT(_downloadErrorMessage(const QString&)));
}

/// @brief Connects to the signals associated with the FileManager::listDirectory method. We only leave these signals connected
/// while a download because there may be multiple UAS, which in turn means multiple QGCUASFileView instances. We only want the signals
/// connected to the active FileView which is doing the current download.
void QGCUASFileView::_connectListSignals(void)
{
    bool success;
    Q_UNUSED(success);    // Silence retail unused variable error
    
    success = connect(_manager, SIGNAL(listEntry(const QString&)), this, SLOT(_listEntryReceived(const QString&)));
    Q_ASSERT(success);
    success = connect(_manager, SIGNAL(listComplete(void)), this, SLOT(_listComplete(void)));
    Q_ASSERT(success);
    success = connect(_manager, SIGNAL(errorMessage(const QString&)), this, SLOT(_listErrorMessage(const QString&)));
    Q_ASSERT(success);
}

void QGCUASFileView::_disconnectListSignals(void)
{
    disconnect(_manager, SIGNAL(listEntry(const QString&)), this, SLOT(_listEntryReceived(const QString&)));
    disconnect(_manager, SIGNAL(listComplete(void)), this, SLOT(_listComplete(void)));
    disconnect(_manager, SIGNAL(errorMessage(const QString&)), this, SLOT(_listErrorMessage(const QString&)));
}
