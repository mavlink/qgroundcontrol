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
            if (node.nodeName() == "field")
            {
                for (int i = 0; i < attributeMap.count(); ++i) {
                    QDomNode attribute = attributeMap.item(i);
                    if (attribute.nodeName() == "name") return attribute.nodeValue();
                }
            }
            else
            {
                return node.nodeName();
            }
        }
    case 1:
        for (int i = 0; i < attributeMap.count(); ++i) {
            QDomNode attribute = attributeMap.item(i);
            attributes << attribute.nodeName() + "=\""
                       +attribute.nodeValue() + "\"";
        }
        return attributes.join(" ");
    case 2:
        return node.nodeValue().split("\n").join(" ");
    default:
        return QVariant();
    }
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
            return tr("Name");
        case 1:
            return tr("Attributes");
        case 2:
            return tr("Value");
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
