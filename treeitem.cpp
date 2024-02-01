#include "treeitem.h"

TreeItem::TreeItem(const QString &data, TreeItem *parent)
        : m_itemData{data}
        , m_parentItem{parent}
{
    qDebug() << "data = " << data << " TreeItem " << this << "parent TreeItem: " << parent;
}

TreeItem::~TreeItem()
{
    qDeleteAll(m_childItems);
}

void TreeItem::appendChild(TreeItem *child)
{
    m_childItems.append(child);
}

TreeItem *TreeItem::child(int row)
{
    if (row < 0 || row >= m_childItems.count()) {
        return nullptr;
    }
    return m_childItems.at(row);
}

int TreeItem::childCount() const
{
    return m_childItems.count();
}

int TreeItem::columnCount() const
{
    return m_itemData.length();
}

QVariant TreeItem::data(int column) const
{
    if (column != 1) {
        return QVariant();
    }
    return m_itemData.at(column);
}

int TreeItem::row() const
{
    if (m_parentItem) {
        return m_parentItem->m_childItems.indexOf(const_cast<TreeItem *>(this));
    }
    return 0;
}

TreeItem *TreeItem::parentItem()
{
    return m_parentItem;
}