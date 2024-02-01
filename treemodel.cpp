#include "treemodel.h"

TreeModel::TreeModel(QObject *parent)
        : QAbstractItemModel(parent)
{
    m_rootItem = new TreeItem("root");

}

TreeModel::~TreeModel()
{
    delete m_rootItem;
}

void TreeModel::addItem(TreeItem *item, TreeItem *parent) {
    if (parent) {
        parent->appendChild(item);
        return;
    } // If no parent is specified, add to root
    m_rootItem->appendChild(item);
}

TreeItem *TreeModel::getRootItem() const {
    return m_rootItem;
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) {
        return {};
    }

    TreeItem *parentItem;

    if (!parent.isValid()) {
        parentItem = m_rootItem;
    } else {
        parentItem = static_cast<TreeItem *>(parent.internalPointer());
    }

    TreeItem *childItem = parentItem->child(row);
    if (childItem) {
        return createIndex(row, column, childItem);
    }
    return {};
}

QModelIndex TreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return {};

    TreeItem *childItem = static_cast<TreeItem *>(index.internalPointer());
    TreeItem *parentItem = childItem->parentItem();

    if (parentItem == m_rootItem)
        return {};

    return createIndex(parentItem->row(), 0, parentItem);
}

int TreeModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return m_rootItem->m_childItems.count();
    }

    TreeItem *parentItem = static_cast<TreeItem *>(parent.internalPointer());
    return parentItem->childCount();
}

int TreeModel::columnCount(const QModelIndex &parent) const
{
    return 1;
}

QVariant TreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    if (role != Qt::DisplayRole)
        return {};

    TreeItem *item = static_cast<TreeItem *>(index.internalPointer());
    return item->m_itemData;
}

void TreeModel::showQModelIndex(const QModelIndex &index) const
{
    auto *item = static_cast<TreeItem *>(index.internalPointer());
    qDebug() << "Show QModelIndex " << index << " internal pointer " << index.internalPointer()
             << " Data " << item->m_itemData;
}