#include "QmlObjectListItem.h"

QmlObjectListItem::QmlObjectListItem(QObject* parent)
    : QObject(parent)
{

}

QmlObjectListItem::~QmlObjectListItem()
{

}

void QmlObjectListItem::setDirty(bool dirty)
{
    if (dirty != _dirty) {
        _dirty = dirty;
        emit dirtyChanged(dirty);
    }
}

void QmlObjectListItem::_setDirty(void)
{
    if (!_ignoreDirtyChangeSignals) {
        setDirty(true);
    }
}

void QmlObjectListItem::_setIfDirty(bool dirty)
{
    if (dirty) {
        setDirty(true);
    }
}
