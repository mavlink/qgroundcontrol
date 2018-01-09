/*!
 * @file
 *   @brief QML Simple File List Controller
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#include "QGCApplication.h"
#include "QGCFileListController.h"

//-----------------------------------------------------------------------------
QGCFileListController::QGCFileListController(QObject* parent)
    : QObject(parent)
    , _selectedCount(0)
{
    static bool initialized = false;
    if(!initialized) {
        initialized = true;
        qmlRegisterType<QGCFileListItem>("QGCFileListItem", 1,0, "QGCFileListItem");
    }
}

//-----------------------------------------------------------------------------
QQmlListProperty<QGCFileListItem>
QGCFileListController::fileList()
{
    return QQmlListProperty<QGCFileListItem>(
        this, this,
        &QGCFileListController::appendFileItem,
        &QGCFileListController::fileCount,
        &QGCFileListController::fileItem,
        &QGCFileListController::clearFileItems
        );
}

//-----------------------------------------------------------------------------
QGCFileListItem*
QGCFileListController::fileItem(int index)
{
    if(index >= 0 && _fileList.size() && index < _fileList.size()) {
        return _fileList.at(index);
    }
    return NULL;
}

//-----------------------------------------------------------------------------
int
QGCFileListController::fileCount()
{
    return _fileList.size();
}

//-----------------------------------------------------------------------------
void
QGCFileListController::clearFileItems()
{
    return _fileList.clear();
}

//-----------------------------------------------------------------------------
void
QGCFileListController::appendFileItem(QGCFileListItem* p)
{
    _fileList.append(p);
}

//-----------------------------------------------------------------------------
QGCFileListItem*
QGCFileListController::fileItem(QQmlListProperty<QGCFileListItem>* list, int i)
{
    return reinterpret_cast<QGCFileListController*>(list->data)->fileItem(i);
}

//-----------------------------------------------------------------------------
int
QGCFileListController::fileCount(QQmlListProperty<QGCFileListItem>* list)
{
    return reinterpret_cast<QGCFileListController*>(list->data)->fileCount();
}

//-----------------------------------------------------------------------------
void
QGCFileListController::appendFileItem(QQmlListProperty<QGCFileListItem>* list, QGCFileListItem* p)
{
    reinterpret_cast<QGCFileListController*>(list->data)->appendFileItem(p);
}

//-----------------------------------------------------------------------------
void
QGCFileListController::clearFileItems(QQmlListProperty<QGCFileListItem>* list)
{
    reinterpret_cast<QGCFileListController*>(list->data)->clearFileItems();
}

//-----------------------------------------------------------------------------
void
QGCFileListController::selectAllFiles(bool selected)
{
    for(int i = 0; i < _fileList.size(); i++) {
        if(_fileList[i]) {
            QGCFileListItem* pItem = qobject_cast<QGCFileListItem*>(_fileList[i]);
            if(pItem) {
                pItem->setSelected(selected);
            }
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCFileListItem::setSelected (bool sel)
{
    if(_selected != sel) {
        _selected = sel;
        emit selectedChanged();
        if(sel) {
            _parent->_selectedCount++;
        } else {
            _parent->_selectedCount--;
        }
        emit _parent->selectedCountChanged();
    }
}
