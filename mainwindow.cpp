#include "MainWindow.h"
#include <QDir>
#include <QFileSystemModel>
#include <QFileDialog>

#define MAX_DEPTH 10

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    ui = new Ui::MainWindow;
    ui->setupUi(this);
    setupTreeWidget();
    setupFileSystemWatcher();
}

void MainWindow::setupTreeWidget() {
    fileTreeWidget = ui->fileTreeWidget;
    flaggedFilesTreeWidget = ui->flaggedFilesTreeWidget;
    fileTypesTableWidget = ui->fileTypesTableWidget;
    scanPatternsTableWidget = ui->scanPatternsTableWidget;
    flaggedFilesTreeWidget->setHeaderLabel("Flagged Files");
    fileTreeWidget->setHeaderLabel("Files and Folders");

    // Add some example items to the file types table
    fileTypesTableWidget->insertRow(0);
    fileTypesTableWidget->setItem(0, 0, new QTableWidgetItem("*.txt"));
    fileTypesTableWidget->setItem(0, 1, new QTableWidgetItem("Text file"));

    fileTypesTableWidget->insertRow(1);
    fileTypesTableWidget->setItem(1, 0, new QTableWidgetItem("*.docx"));
    fileTypesTableWidget->setItem(1, 1, new QTableWidgetItem("Word document"));

    // Set the table to not be editable
    fileTypesTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

}

void MainWindow::setupFileSystemWatcher() {
    watcher = new QFileSystemWatcher(this);
    // Go one directory up from currentPath and add TestFolder1 to the list of directories to watch
    QStringList directoriesToWatch = {"/Users/olaf/Documents/Code/sensitive-data-deleter/TestFolder1"};

            foreach (const QString &dirPath, directoriesToWatch) {
            auto *rootItem = new QTreeWidgetItem(fileTreeWidget, QStringList(dirPath));
            rootItem->setData(0, Qt::UserRole, dirPath);
            pathsToScan.insert(dirPath, rootItem);
            updateTreeItem(rootItem, dirPath);
            watcher->addPath(dirPath);
        }

    connect(watcher, &QFileSystemWatcher::directoryChanged, this, &MainWindow::onDirectoryChanged);
}

void MainWindow::onDirectoryChanged(const QString &path) {
    QTreeWidgetItem *item = pathsToScan.value(path);
    if (item) {
        updateTreeItem(item, path);
    }
}

void MainWindow::updateTreeItem(QTreeWidgetItem *item, const QString &path) {
    QDir dir(path);
    item->takeChildren(); // Clear existing children

            foreach (const QString &entry,
                     dir.entryList(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs, QDir::DirsFirst)) {
            QString childPath = path + QDir::separator() + entry;
            auto *childItem = new QTreeWidgetItem(item, QStringList(entry));
            childItem->setData(0, Qt::UserRole, childPath);
            if (QFileInfo(childPath).isDir()) {
                updateTreeItem(childItem, childPath); // Recursive call for subdirectories
                watcher->addPath(childPath); // Add subdirectories to the watcher
                pathsToScan.insert(childPath, childItem);
            }
        }
}

QTreeWidgetItem *MainWindow::findItemForPath(QTreeWidgetItem *parentItem, const QString &path) {
    // Check if the current item's path matches
    if (parentItem->text(0) == path) {
        return parentItem;
    }

    // Recursively search in child items
    for (int i = 0; i < parentItem->childCount(); ++i) {
        QTreeWidgetItem *childItem = parentItem->child(i);
        // If child is a directory, search in it
        if (QFileInfo(childItem->text(0)).isDir()) {
            QTreeWidgetItem *foundItem = findItemForPath(childItem, path);
            if (foundItem) {
                return foundItem;
            }
        }
    }

    return nullptr;
}

QTreeWidgetItem *MainWindow::findItemForPath(QTreeWidget *treeWidget, const QString &path) {
    for (const auto &item: pathsToScan) {
        if (item->data(0, Qt::UserRole) == path) {
            return item;
        }
    }

    return nullptr;
}


void MainWindow::constructScanTreeViewRecursively(QTreeWidgetItem *parentItem, QString &currentPath, int depth) {
    if (depth > MAX_DEPTH) {
        qWarning() << "Max depth reached. Cannot scan further than " << MAX_DEPTH << " levels deep";
        return;
    }

    // If a file or folder encountered already exists in the scan list,
    // join it to the directory item and remove the original item from the tree
    QTreeWidgetItem *existingItem = pathsToScan.value(currentPath);
    QString shortName = currentPath.split(QDir::separator()).last();
    if (existingItem) {
        // Change name from full path to file/folder name only
        existingItem->setText(0, shortName);
        fileTreeWidget->invisibleRootItem()->removeChild(existingItem);
        parentItem->addChild(existingItem);
        return;
    }

    QString dirName = depth == 0 ? currentPath : shortName;
    auto *directoryItem = new QTreeWidgetItem(parentItem, QStringList(dirName));
    directoryItem->setData(0, Qt::UserRole, currentPath);
    directoryItem->setCheckState(0, Qt::Unchecked);
    pathsToScan.insert(currentPath, directoryItem);
    watcher->addPath(currentPath);

    // Return early if the item is a file
    if (!QFileInfo(currentPath).isDir()) {
        return;
    }

    QDir dir(currentPath);
    foreach (const QString &entry, dir.entryList(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs, QDir::DirsFirst)) {
        QString childPath = currentPath + QDir::separator() + entry;
        constructScanTreeViewRecursively(directoryItem, childPath, depth + 1);
    }
}

QString MainWindow::getParentPath(const QString &dirPath) {
    qsizetype lastSeparatorIndex = dirPath.lastIndexOf(QDir::separator());
    QString parentPath = dirPath.left(lastSeparatorIndex);
    while (!parentPath.isEmpty()) {
        if (pathsToScan.contains(parentPath)) {
            auto *parentItem = pathsToScan.value(parentPath);
            return parentItem->data(0, Qt::UserRole).toString();
        }
        parentPath = parentPath.left(parentPath.lastIndexOf(QDir::separator()));
    }

    return nullptr;
}

void MainWindow::on_addFolderButton_clicked() {
    QString dirPath = QFileDialog::getExistingDirectory(this, tr("Select Directory"), QDir::currentPath());
    if (dirPath.isEmpty()) {
        qDebug() << "No directory selected";
        return;
    }

    if (pathsToScan.contains(dirPath)) {
        qDebug() << "Directory already being watched";
        return;
    }

    auto parentPath = getParentPath(dirPath);
    if (!parentPath.isEmpty()) {
        auto *parentItem = pathsToScan.value(parentPath);
        auto *childItem = new QTreeWidgetItem(parentItem, QStringList(QDir(dirPath).dirName()));
        childItem->setData(0, Qt::UserRole, dirPath);
        childItem->setCheckState(0, Qt::Unchecked);
        pathsToScan.insert(dirPath, childItem);
        watcher->addPath(dirPath);
        return;
    }

    constructScanTreeViewRecursively(fileTreeWidget->invisibleRootItem(), dirPath);
}

QString formatFileSize(qint64 size) {
    // Switch on the order of magnitude of the file size
    switch (size) {
        case 0 ... 1023:
            return QString::number(size) + " B";
        case 1024 ... 1048575:
            return QString::number(size / 1024) + " KB";
        case 1048576 ... 1073741823:
            return QString::number(size / 1048576) + " MB";
        default:
            return QString::number(size / 1073741824) + " GB";
    }
}

void MainWindow::createTreeItem(QTreeWidgetItem *parentItem, const QString &path) {
    auto *item = new QTreeWidgetItem(parentItem, QStringList(path));
    item->setData(0, Qt::UserRole, path);
    item->setCheckState(0, Qt::Unchecked);
    pathsToScan.insert(path, item);
    watcher->addPath(path);
}

void MainWindow::on_addFileButton_clicked() {
    QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Select Files"), QDir::currentPath());
    if (fileNames.isEmpty()) {
        qDebug() << "No files selected";
        return;
    }

    // List the selected files
    qDebug() << "Selected files: " << fileNames;

    for (const auto &fileName: fileNames) {
        QString itemPath = QFileInfo(fileName).absoluteFilePath();
        // Print all the paths being watched
        qDebug() << "Paths being watched: " << pathsToScan.keys();

        if (pathsToScan.contains(itemPath)) {
            qDebug() << "File " << fileName << "already being watched";
            continue;
        }

        // Handle the case when a file cannot be read due to permissions
        if (!QFileInfo(fileName).isReadable()) {
            qWarning() << "File " << fileName << " is not readable";
            continue;
        }

        // If a parent directory is already being watched, add the new file to the parent item
        QString parentPath = getParentPath(itemPath);

        if (!parentPath.isEmpty()) {
            auto *parentItem = pathsToScan.value(parentPath);
            auto *childItem = new QTreeWidgetItem(parentItem, QStringList(QFileInfo(fileName).fileName()));
            childItem->setData(0, Qt::UserRole, fileName);
            childItem->setCheckState(0, Qt::Unchecked);
            pathsToScan.insert(itemPath, childItem);
            continue;
        }

        auto *fileItem = new QTreeWidgetItem(fileTreeWidget->invisibleRootItem(), QStringList(itemPath));
        fileItem->setData(0, Qt::UserRole, fileName);
        fileItem->setText(1, formatFileSize(QFileInfo(fileName).size()));
        fileItem->setText(2, QFileInfo(fileName).lastModified().toString("dd/MM/yyyy"));
        fileItem->setCheckState(0, Qt::Unchecked);
        pathsToScan.insert(itemPath, fileItem);
        watcher->addPath(fileName);
    }
}

