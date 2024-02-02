#include "MainWindow.h"
#include <QDir>
#include <QFileSystemModel>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    ui = new Ui::MainWindow;
    ui->setupUi(this);
    setupTreeWidget();
    setupFileSystemWatcher();
}

void MainWindow::setupTreeWidget() {
    fileTreeWidget = ui->fileTreeWidget;
    flaggedFilesTableView = ui->flaggedFilesTableView;
    flaggedFilesTableView->setHorizontalHeader(new QHeaderView(Qt::Horizontal, this));
    fileTreeWidget->setColumnCount(1);
    fileTreeWidget->setHeaderLabel("Files and Folders");

    // Create a filesystemmodel and list the current directory in the table view
    auto *model = new QFileSystemModel(this);
    model->setRootPath(QDir::currentPath());
    flaggedFilesTableView->setModel(model);
    flaggedFilesTableView->setRootIndex(model->index(QDir::currentPath()));

}

void MainWindow::setupFileSystemWatcher() {
    watcher = new QFileSystemWatcher(this);
    QStringList directoriesToWatch = {QDir::currentPath()};

    foreach (const QString &dirPath, directoriesToWatch) {
        auto *rootItem = new QTreeWidgetItem(fileTreeWidget, QStringList(dirPath));
        updateTreeItem(rootItem, dirPath);
        watcher->addPath(dirPath);
    }

    connect(watcher, &QFileSystemWatcher::directoryChanged, this, &MainWindow::onDirectoryChanged);
}

void MainWindow::onDirectoryChanged(const QString &path) {
    QTreeWidgetItem* item = findItemForPath(fileTreeWidget, path);
    if (item) {
        updateTreeItem(item, path);
    }
}

void MainWindow::updateTreeItem(QTreeWidgetItem *item, const QString &path) {
    QDir dir(path);
    item->takeChildren(); // Clear existing children

        foreach (const QString &entry, dir.entryList(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs, QDir::DirsFirst)) {
        auto *childItem = new QTreeWidgetItem(item, QStringList(entry));
        QString childPath = path + QDir::separator() + entry;
        if (QFileInfo(childPath).isDir()) {
            updateTreeItem(childItem, childPath); // Recursive call for subdirectories
            watcher->addPath(childPath); // Add subdirectories to the watcher
        }
    }
}


QTreeWidgetItem* MainWindow::findItemForPath(QTreeWidgetItem* parentItem, const QString& path) {
    if (!parentItem) return nullptr;

    // Check if the current item's path matches
    if (parentItem->text(0) == path) {
        return parentItem;
    }

    // Recursively search in child items
    for (int i = 0; i < parentItem->childCount(); ++i) {
        QTreeWidgetItem* childItem = parentItem->child(i);
        QTreeWidgetItem* foundItem = findItemForPath(childItem, path);
        if (foundItem) {
            return foundItem;
        }
    }

    return nullptr;
}

QTreeWidgetItem* MainWindow::findItemForPath(QTreeWidget* treeWidget, const QString& path) {
    for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem* rootItem = treeWidget->topLevelItem(i);
        QTreeWidgetItem* foundItem = findItemForPath(rootItem, path);
        if (foundItem) {
            return foundItem;
        }
    }

    return nullptr;
}

