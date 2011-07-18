#include <QtGui>
#include <QtXml>

#include "DomItem.h"
#include "DomModel.h"

DomModel::DomModel(QDomDocument document, QObject *parent)
    : QAbstractItemModel(parent), domDocument(document)
{
    rootItem = new DomItem(domDocument, 0);
}

DomModel::~DomModel()
{
    delete rootItem;
}

int DomModel::columnCount(const QModelIndex &/*parent*/) const
{
    return 3;
}

QVariant DomModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    DomItem *item = static_cast<DomItem*>(index.internalPointer());

    QDomNode node = item->node();
    QStringList attributes;
    QDomNamedNodeMap attributeMap = node.attributes();

    switch (index.column()) {
    case 0:
        {
            if (node.nodeName() == "message")
            {
                for (int i = 0; i < attributeMap.count(); ++i) {
                    QDomNode attribute = attributeMap.item(i);
                    if (attribute.nodeName() == "name") return attribute.nodeValue();
                }
            }
            else if (node.nodeName() == "field")
            {
                for (int i = 0; i < attributeMap.count(); ++i) {
                    QDomNode attribute = attributeMap.item(i);
                    if (attribute.nodeName() == "name") return attribute.nodeValue();
                }
            }
            else if (node.nodeName() == "enum")
            {
                for (int i = 0; i < attributeMap.count(); ++i) {
                    QDomNode attribute = attributeMap.item(i);
                    if (attribute.nodeName() == "name") return attribute.nodeValue();
                }
            }
            else if (node.nodeName() == "entry")
            {
                for (int i = 0; i < attributeMap.count(); ++i) {
                    QDomNode attribute = attributeMap.item(i);
                    if (attribute.nodeName() == "name") return attribute.nodeValue();
                }
            }
            else if (node.nodeName() == "#text")
            {
                return node.nodeValue().split("\n").join(" ");
            }
            else
            {
                return node.nodeName();
            }
        }
        break;
    case 1:
        if (node.nodeName() == "description")
            {
                return node.nodeValue().split("\n").join(" ");
            }
        else
        {
        for (int i = 0; i < attributeMap.count(); ++i) {
            QDomNode attribute = attributeMap.item(i);

            if (attribute.nodeName() == "id" || attribute.nodeName() == "index" || attribute.nodeName() == "value")
            {
                return QString("(# %1)").arg(attribute.nodeValue());
            }
            else  if (attribute.nodeName() == "type")
            {
                return attribute.nodeValue();
            }
        }
    }
        break;
//    case 2:
//        {
////            if (node.nodeName() != "description")
////            {
////            for (int i = 0; i < attributeMap.count(); ++i) {
////                QDomNode attribute = attributeMap.item(i);
////                attributes << attribute.nodeName() + "=\""
////                        +attribute.nodeValue() + "\"";
////            }
////            return attributes.join(" ");
////            }
////            else
////            {
////            return node.nodeValue().split("\n").join(" ");
////        }
//        }
//        break;
    }
    // Return empty variant if no case applied
    return QVariant();
}

Qt::ItemFlags DomModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant DomModel::headerData(int section, Qt::Orientation orientation,
                              int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case 0:
            return tr("Name                 ");
        case 1:
            return tr("Value");
//        case 2:
//            return tr("Description");
        default:
            return QVariant();
        }
    }

    return QVariant();
}

QModelIndex DomModel::index(int row, int column, const QModelIndex &parent)
        const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    DomItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<DomItem*>(parent.internalPointer());

    DomItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex DomModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return QModelIndex();

    DomItem *childItem = static_cast<DomItem*>(child.internalPointer());
    DomItem *parentItem = childItem->parent();

    if (!parentItem || parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int DomModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;

    DomItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<DomItem*>(parent.internalPointer());

    return parentItem->node().childNodes().count();
}
