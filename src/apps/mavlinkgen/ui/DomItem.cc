#include <QtXml>

#include "DomItem.h"

DomItem::DomItem(QDomNode &node, int row, DomItem *parent)
{
    domNode = node;
    // Record the item's location within its parent.
    rowNumber = row;
    parentItem = parent;
}

DomItem::~DomItem()
{
    QHash<int,DomItem*>::iterator it;
    for (it = childItems.begin(); it != childItems.end(); ++it)
        delete it.value();
}

QDomNode DomItem::node() const
{
    return domNode;
}

DomItem *DomItem::parent()
{
    return parentItem;
}

DomItem *DomItem::child(int i)
{
    if (childItems.contains(i))
        return childItems[i];

    if (i >= 0 && i < domNode.childNodes().count()) {
        QDomNode childNode = domNode.childNodes().item(i);
        DomItem *childItem = new DomItem(childNode, i, this);
        childItems[i] = childItem;
        return childItem;
    }
    return 0;
}

int DomItem::row()
{
    return rowNumber;
}
