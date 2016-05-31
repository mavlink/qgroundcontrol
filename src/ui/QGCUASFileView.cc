/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "QGCUASFileView.h"
#include "FileManager.h"
#include "QGCFileDialog.h"
#include "UAS.h"

#include <QFileDialog>
#include <QDir>
#include <QDebug>

QGCUASFileView::QGCUASFileView(QWidget *parent, Vehicle* vehicle)
    : QWidget(parent)
    , _manager(vehicle->uas()->getFileManager())
    , _currentCommand(commandNone)
{
    _ui.setupUi(this);

    if (vehicle->px4Firmware()) {
        _ui.progressBar->reset();

        // Connect UI signals
        connect(_ui.listFilesButton,    &QPushButton::clicked,              this, &QGCUASFileView::_refreshTree);
        connect(_ui.downloadButton,     &QPushButton::clicked,              this, &QGCUASFileView::_downloadFile);
        connect(_ui.uploadButton,       &QPushButton::clicked,              this, &QGCUASFileView::_uploadFile);
        connect(_ui.treeWidget,         &QTreeWidget::currentItemChanged,   this, &QGCUASFileView::_currentItemChanged);

        // Connect signals from FileManager
        connect(_manager, &FileManager::commandProgress,    this, &QGCUASFileView::_commandProgress);
        connect(_manager, &FileManager::commandComplete,    this, &QGCUASFileView::_commandComplete);
        connect(_manager, &FileManager::commandError,       this, &QGCUASFileView::_commandError);
        connect(_manager, &FileManager::listEntry,  this, &QGCUASFileView::_listEntryReceived);
    } else {
        _setAllButtonsEnabled(false);
        _ui.statusText->setText(QStringLiteral("Onboard Files not supported by this Vehicle"));
    }
}

/// @brief Downloads the file currently selected in the tree view
void QGCUASFileView::_downloadFile(void)
{
    if (_currentCommand != commandNone) {
        qWarning() << QString("Download attempted while another command was in progress: _currentCommand(%1)").arg(_currentCommand);
        return;
    }
    
    _ui.statusText->clear();
    
    QString downloadToHere = QGCFileDialog::getExistingDirectory(this,
                                                                 "Download Directory",
                                                                 QDir::homePath(),
                                                                 QGCFileDialog::ShowDirsOnly | QGCFileDialog::DontResolveSymlinks);
    
    // And now download to this location
    
    QString path;
    QString downloadFilename;
    
    QTreeWidgetItem* item = _ui.treeWidget->currentItem();
    if (item && item->type() == _typeFile) {
        do {
            QString name = item->text(0).split("\t")[0];    // Strip off file sizes
            
            // If this is the file name and not a directory keep track of the download file name
            if (downloadFilename.isEmpty()) {
                downloadFilename = name;
            }
            
            path.prepend("/" + name);
            item = item->parent();
        } while (item);
        
        _setAllButtonsEnabled(false);
        _currentCommand = commandDownload;
        
        _ui.statusText->setText(QString("Downloading: %1").arg(downloadFilename));
                                
        _manager->streamPath(path, QDir(downloadToHere));
    }
}

/// @brief uploads a file into the currently selected directory the tree view
void QGCUASFileView::_uploadFile(void)
{
    if (_currentCommand != commandNone) {
        qWarning() << QString("Upload attempted while another command was in progress: _currentCommand(%1)").arg(_currentCommand);
        return;
    }

    _ui.statusText->clear();

    // get and check directory from list view
    QTreeWidgetItem* item = _ui.treeWidget->currentItem();
    if (item && item->type() != _typeDir) {
        return;
    }

    // Find complete path for upload directory
    QString path;
    do {
        QString name = item->text(0).split("\t")[0];    // Strip off file sizes
        path.prepend("/" + name);
        item = item->parent();
    } while (item);

    QString uploadFromHere = QGCFileDialog::getOpenFileName(this, "Upload File", QDir::homePath());

    _ui.statusText->setText(QString("Uploading: %1").arg(uploadFromHere));
                            
    qDebug() << "Upload: " << uploadFromHere << "to path" << path;
    
    _setAllButtonsEnabled(false);
    _currentCommand = commandUpload;

    _manager->uploadPath(path, uploadFromHere);
}

/// @brief Called to update the progress of the download.
///     @param value Progress bar value
void QGCUASFileView::_commandProgress(int value)
{
    _ui.progressBar->setValue(value);
}

/// @brief Called when an error occurs during a download.
///     @param msg Error message
void QGCUASFileView::_commandError(const QString& msg)
{
    _setAllButtonsEnabled(true);
    _currentCommand = commandNone;
    _ui.statusText->setText(QString("Error: %1").arg(msg));
}

/// @brief Refreshes the directory list tree.
void QGCUASFileView::_refreshTree(void)
{
    if (_currentCommand != commandNone) {
        qWarning() << QString("List attempted while another command was in progress: _currentCommand(%1)").arg(_currentCommand);
        return;
    }
    
    _ui.treeWidget->clear();
    _ui.statusText->clear();

    _walkIndexStack.clear();
    _walkItemStack.clear();
    _walkIndexStack.append(0);
    _walkItemStack.append(_ui.treeWidget->invisibleRootItem());
    
    _setAllButtonsEnabled(false);
    _currentCommand = commandList;

    _requestDirectoryList("/");
}

/// @brief Adds the specified directory entry to the tree view.
void QGCUASFileView::_listEntryReceived(const QString& entry)
{
    if (_currentCommand != commandList) {
        qWarning() << QString("List entry received while no list command in progress: _currentCommand(%1)").arg(_currentCommand);
        return;
    }
    
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

/// @brief Called when a command completes successfully
void QGCUASFileView::_commandComplete(void)
{
    QString statusText;
    
    if (_currentCommand == commandDownload) {
        _currentCommand = commandNone;
        _setAllButtonsEnabled(true);
        statusText = "Download complete";
    } else if (_currentCommand == commandDownload) {
        _currentCommand = commandNone;
        _setAllButtonsEnabled(true);
        statusText = "Upload complete";
    } else if (_currentCommand == commandList) {
        _listComplete();
    }
    
    _ui.statusText->setText(statusText);
    _ui.progressBar->reset();
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
        _requestDirectoryList(dir);
    } else {
        // We have run out of items at the this level, pop the stack and keep going at that level
        _walkIndexStack.removeLast();
        _walkItemStack.removeLast();
        if (_walkIndexStack.count() != 0) {
            goto Again;
        } else {
            _setAllButtonsEnabled(true);
            _currentCommand = commandNone;
        }
    }
}

void QGCUASFileView::_currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous)
{
    Q_UNUSED(previous);
    
    _ui.downloadButton->setEnabled(current ? (current->type() == _typeFile) : false);
    _ui.uploadButton->setEnabled(current ? (current->type() == _typeDir) : false);
}

void QGCUASFileView::_requestDirectoryList(const QString& dir)
{
    _manager->listDirectory(dir);
}

void QGCUASFileView::_setAllButtonsEnabled(bool enabled)
{
    _ui.treeWidget->setEnabled(enabled);
    _ui.downloadButton->setEnabled(enabled);
    _ui.listFilesButton->setEnabled(enabled);
    _ui.uploadButton->setEnabled(enabled);
    
    if (enabled) {
        _currentItemChanged(_ui.treeWidget->currentItem(), NULL);
    }
}
