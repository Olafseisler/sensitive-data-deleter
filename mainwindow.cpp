#include "MainWindow.h"
#include <QDir>
#include <QFileSystemModel>
#include <QFileDialog>

#define MAX_DEPTH 10

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    ui = new Ui::MainWindow;
    ui->setupUi(this);
    configManager = new ConfigManager();
    setupUI();
    watcher = new QFileSystemWatcher(this);
}

void MainWindow::setupUI() {
    fileTreeWidget = ui->fileTreeWidget;
    flaggedFilesTreeWidget = ui->flaggedFilesTreeWidget;
    fileTypesTableWidget = ui->fileTypesTableWidget;
    scanPatternsTableWidget = ui->scanPatternsTableWidget;
    fileTreeWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

    flaggedFilesTreeWidget->setHeaderLabel("Flagged Files");
    fileTreeWidget->setHeaderLabel("Files and Folders");

    // Load the config
    QMap<QString, QString> fileTypes = configManager->getFileTypes();
    foreach (const auto &fileType, fileTypes.keys()) {
        fileTypesTableWidget->insertRow(fileTypesTableWidget->rowCount());
        auto *checkBox = new QTableWidgetItem();
        checkBox->setCheckState(Qt::Checked);
        fileTypesTableWidget->setItem(fileTypesTableWidget->rowCount() - 1, 0, checkBox);
        fileTypesTableWidget->setItem(fileTypesTableWidget->rowCount() - 1, 1, new QTableWidgetItem(fileType));
        fileTypesTableWidget->setItem(fileTypesTableWidget->rowCount() - 1, 2, new QTableWidgetItem(fileTypes[fileType]));
    }

    // Set column widths. The first column should be as small as possible, the second column should be 30% of the table width
    fileTypesTableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    fileTypesTableWidget->setColumnWidth(2, fileTypesTableWidget->width() * 0.3);
    fileTypesTableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);

    QMap<QString, QString> scanPatterns = configManager->getScanPatterns();
    foreach (const auto &scanPattern, scanPatterns.keys()) {
        scanPatternsTableWidget->insertRow(scanPatternsTableWidget->rowCount());
        auto *checkBox = new QTableWidgetItem();
        checkBox->setCheckState(Qt::Checked);
        scanPatternsTableWidget->setItem(scanPatternsTableWidget->rowCount() - 1, 0, checkBox);
        scanPatternsTableWidget->setItem(scanPatternsTableWidget->rowCount() - 1, 1, new QTableWidgetItem(scanPattern));
        scanPatternsTableWidget->setItem(scanPatternsTableWidget->rowCount() - 1, 2, new QTableWidgetItem(scanPatterns[scanPattern]));
    }

    scanPatternsTableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    scanPatternsTableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    scanPatternsTableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);

    // Set the file types table to not be editable
    fileTypesTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(fileTreeWidget, &QTreeWidget::itemChanged, this, &MainWindow::onItemChanged);
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

    foreach (const QString &entry, dir.entryList(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs, QDir::DirsFirst)) {
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


void MainWindow::constructScanTreeViewRecursively(QTreeWidgetItem *parentItem, QString &currentPath,
                                                  int depth, bool useShortName) {
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

    QString dirName = useShortName ? currentPath : shortName;
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
        constructScanTreeViewRecursively(directoryItem, childPath, depth + 1, false);
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

    return {};
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
        constructScanTreeViewRecursively(parentItem, dirPath);
        return;
    }

    constructScanTreeViewRecursively(fileTreeWidget->invisibleRootItem(), dirPath);
    // Uncheck all items in the tree
    for (const auto &item: pathsToScan) {
        item->setCheckState(0, Qt::Unchecked);
    }
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

void MainWindow::createTreeItem(QTreeWidgetItem *parentItem, const QString &path, bool useShortName) {
    QString shortName = useShortName ? path.split(QDir::separator()).last() : path;
    auto *item = new QTreeWidgetItem(parentItem, QStringList(shortName));
    item->setData(0, Qt::UserRole, path);
    item->setText(1, formatFileSize(QFileInfo(path).size()));
    // If the item is a file, set the size and last modified date
    if (!QFileInfo(path).isDir()) {
        item->setText(1, formatFileSize(QFileInfo(path).size()));
        item->setText(2, QFileInfo(path).lastModified().toString("dd/MM/yyyy"));
    }
    item->setCheckState(0, Qt::Unchecked);
    pathsToScan.insert(path, item);
    watcher->addPath(path);
}

void MainWindow::onItemChanged(QTreeWidgetItem *item, int column) {
    if (column != 0) {
        return;
    }

    qDebug() << pathsToScan;

//    QString path = item->data(0, Qt::UserRole).toString();
//    if (item->checkState(0) == Qt::Checked) {
//        qDebug() << "Item " << path << " checked";
//        // If the item represents a directory, check all its children
//        if (QFileInfo(path).isDir()) {
//            for (int i = 0; i < item->childCount(); ++i) {
//                item->child(i)->setCheckState(0, Qt::Checked);
//            }
//        }
//    } else {
//        qDebug() << "Item " << path << " unchecked";
//        // If the item represents a directory, uncheck all its children
//        if (QFileInfo(path).isDir()) {
//            for (int i = 0; i < item->childCount(); ++i) {
//                item->child(i)->setCheckState(0, Qt::Unchecked);
//            }
//        }
//    }
}

void MainWindow::on_addFileButton_clicked() {
    QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Select Files"), QDir::currentPath());
    if (fileNames.isEmpty()) {
        qDebug() << "No files selected";
        return;
    }

    for (const auto &fileName: fileNames) {
        QString itemPath = QFileInfo(fileName).absoluteFilePath();

        if (pathsToScan.contains(itemPath)) {
            qDebug() << "File " << itemPath << " already being watched";
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
            createTreeItem(parentItem, itemPath, true);
            continue;
        }

        createTreeItem(fileTreeWidget->invisibleRootItem(), itemPath, false);
    }

    // Uncheck all items in the tree
    for (const auto &item: pathsToScan) {
        item->setCheckState(0, Qt::Unchecked);
    }
}

void MainWindow::removeItemFromTree(QTreeWidgetItem *item) {
    if (!item) {
        return;
    }

    QVariant itemData = item->data(0, Qt::UserRole);

    auto path = itemData.toString();

    // If it is a single file
    if (!QFileInfo(path).isDir()) {
        if (item->parent()) {
            item->parent()->removeChild(item);
        }
        pathsToScan.remove(path);
        watcher->removePath(path);
        return;
    }

    // If it is a directory, iterate over subdirectories
    for (int i = 0; i < item->childCount(); ++i) {
        removeItemFromTree(item->child(i));
    }

    if (item->parent()) {
        item->parent()->removeChild(item);
    }

    pathsToScan.remove(path);
    watcher->removePath(path);
}

void MainWindow::on_deleteSelectedButton_clicked()
{
    // Get all checked items
    QList<QTreeWidgetItem *> checkedItems;
    for (const auto &item: pathsToScan) {
        if (item->checkState(0) == Qt::Checked) {
            checkedItems.append(item);
        }
    }

    // Remove checked items from the tree and the pathsToScan map
    while (!checkedItems.isEmpty()) {
        auto item = checkedItems.takeFirst();
        removeItemFromTree(item);
        delete item;
    }

}

