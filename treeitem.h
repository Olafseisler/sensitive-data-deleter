#pragma once
#include <QVariant>

class TreeItem
{
public:
    explicit TreeItem(const QString &data, TreeItem *parent = nullptr);
    ~TreeItem();

    void appendChild(TreeItem *child);

    TreeItem *child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    TreeItem *parentItem();

    QList<TreeItem *> m_childItems;
    QString m_itemData;
    TreeItem *m_parentItem;
};