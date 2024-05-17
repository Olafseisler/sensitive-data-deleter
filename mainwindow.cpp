#include "MainWindow.h"
#include <QDir>
#include <QFileSystemModel>
#include <QFileDialog>
#include <QCheckBox>

#define MAX_DEPTH 10

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    ui = new Ui::MainWindow;
    ui->setupUi(this);
    configManager = new ConfigManager();
    watcher = new QFileSystemWatcher(this);
    setupUI();
    fileScanner = new FileScanner();
}

MainWindow::~MainWindow() {
    delete ui;
    delete configManager;
    delete watcher;
    delete fileScanner;
}

void MainWindow::setupUI() {
    fileTreeWidget = ui->fileTreeWidget;
    myRootItem = new QTreeWidgetItem(fileTreeWidget, QStringList("Files and Folders"));
    fileTreeWidget->insertTopLevelItem(0, myRootItem);
    flaggedFilesTreeWidget = ui->flaggedFilesTreeWidget;
    fileTypesTableWidget = ui->fileTypesTableWidget;
    scanPatternsTableWidget = ui->scanPatternsTableWidget;
    fileTreeWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

    flaggedFilesTreeWidget->setColumnCount(1);
    flaggedFilesTreeWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    flaggedFilesTreeWidget->setSelectionMode(QAbstractItemView::MultiSelection);


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
            fileTypesTableWidget->setItem(fileTypesTableWidget->rowCount() - 1, 2,
                                          new QTableWidgetItem(fileTypes[fileType]));
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
            scanPatternsTableWidget->setItem(scanPatternsTableWidget->rowCount() - 1, 1,
                                             new QTableWidgetItem(scanPattern));
            scanPatternsTableWidget->setItem(scanPatternsTableWidget->rowCount() - 1, 2,
                                             new QTableWidgetItem(scanPatterns[scanPattern]));
        }

    scanPatternsTableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    scanPatternsTableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    scanPatternsTableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);

    // Set the file types table to not be editable
    fileTypesTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
//    connect(fileTreeWidget, &QTreeWidget::itemChanged, this, &MainWindow::onItemChanged);
    connect(watcher, &QFileSystemWatcher::directoryChanged, this, &MainWindow::onDirectoryChanged);
}

void MainWindow::onDirectoryChanged(const QString &path) {
    qDebug() << "Directory changed: " << path;

    // Check if the changed item is a parent of any keys in flaggedItems.
    for (const auto &flaggedItemPath: flaggedItems.keys()) {
        if (flaggedItemPath.find(path.toStdString()) != std::string::npos) {
            QFile flaggedItemDir(flaggedItemPath.c_str());
            if (!flaggedItemDir.exists()) { // If the flagged item no longer exists, remove it from the tree
                auto *flaggedItemToRemove = flaggedItems.value(flaggedItemPath);
                removeItemFromTree(flaggedItemToRemove);
                flaggedItems.remove(flaggedItemPath);
                delete flaggedItemToRemove;
            }
        }
    }

    QTreeWidgetItem *scanItem = pathsToScan.value(path);
    if (scanItem) {
        updateTreeItem(scanItem, path);
        if (path == lastUpdatedPath) {
            return; // Skip processing if this directory was just updated
        }
        lastUpdatedPath = path;
    }

}

void MainWindow::updateTreeItem(QTreeWidgetItem *item, const QString &path) {
    QDir dir(path);

    if (!dir.exists()) {
        // Remove the item from the tree if it no longer exists
        removeItemFromTree(item);
        delete item;
        return;
    }

            foreach (const QString &entry,
                     dir.entryList(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs, QDir::DirsFirst)) {

            QString childPath = path + "/" + entry;
            QTreeWidgetItem *childItem = pathsToScan.value(childPath);
            if (!childItem) {
                childItem = createTreeItem(item, childPath, true);
            }
            if (QFileInfo(childPath).isDir()) {
                updateTreeItem(childItem, childPath); // Recursive call for subdirectories
//            watcher->addPath(childPath); // Add subdirectories to the watcher
                pathsToScan.insert(childPath, childItem);
            }
        }
    item->setCheckState(0, Qt::Unchecked);
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
    QString shortName = currentPath.split("/").last();
    if (existingItem) {
        // Change name from full path to file/folder name only
        existingItem->setText(0, shortName);
        myRootItem->removeChild(existingItem);
        parentItem->addChild(existingItem);
        return;
    }

    auto *directoryItem = createTreeItem(parentItem, currentPath, useShortName);

    // Return early if the item is a file
    if (!QFileInfo(currentPath).isDir()) {
        return;
    }

    QDir dir(currentPath);
            foreach (const QString &entry,
                     dir.entryList(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs, QDir::DirsFirst)) {
            QString childPath = currentPath + "/" + entry;
            constructScanTreeViewRecursively(directoryItem, childPath, depth + 1, true);
        }
}

QString MainWindow::getParentPath(const QString &dirPath) {
    qsizetype lastSeparatorIndex = dirPath.lastIndexOf("/");
    QString parentPath = dirPath.left(lastSeparatorIndex);
    while (!parentPath.isEmpty()) {
        if (pathsToScan.contains(parentPath)) {
            auto *parentItem = pathsToScan.value(parentPath);
            return parentItem->data(0, Qt::UserRole).toString();
        }
        auto lastIndex = parentPath.lastIndexOf("/");
        if (lastIndex == -1)
            parentPath = "";
        else
            parentPath = parentPath.left(lastIndex);
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

    // Existing parent directory, append the new directory to the parent tree
    auto parentPath = getParentPath(dirPath);
    if (!parentPath.isEmpty()) {
        auto *parentItem = pathsToScan.value(parentPath);
        constructScanTreeViewRecursively(parentItem, dirPath);
        return;
    }

    watcher->addPath(dirPath);
    // Create new tree item for the directory
    constructScanTreeViewRecursively(myRootItem, dirPath);

    // Uncheck all items in the tree
    for (const auto &item: pathsToScan) {
        item->setCheckState(0, Qt::Unchecked);
    }
    myRootItem->setExpanded(true);
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

QTreeWidgetItem *MainWindow::createTreeItem(QTreeWidgetItem *parentItem, const QString &path, bool useShortName) {
    QString shortName = useShortName ? path.split("/").last() : path;
    auto *item = new QTreeWidgetItem(parentItem, QStringList(shortName));
    // Set the parent
    item->setData(0, Qt::UserRole, path);
    item->setText(1, "");
    // If the item is a file, set the size and last modified date
    if (!QFileInfo(path).isDir()) {
        item->setText(1, formatFileSize(QFileInfo(path).size()));
        item->setText(2, QFileInfo(path).lastModified().toString("dd/MM/yyyy"));
    }
    item->setCheckState(0, Qt::Unchecked);
    pathsToScan.insert(path, item);
//    watcher->addPath(path);
    return item;
}

void MainWindow::onItemChanged(QTreeWidgetItem *item, int column) {
    qDebug () << "Watcher dirs: " << watcher->directories();
    qDebug() << "Watcher files: "  << watcher->files();
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

        createTreeItem(myRootItem, itemPath, false);
    }

    // Uncheck all items in the tree
    for (const auto &item: pathsToScan) {
        item->setCheckState(0, Qt::Unchecked);
    }

    myRootItem->setExpanded(true);
}

void MainWindow::removeItemFromTree(QTreeWidgetItem *item) {
    if (!item) {
        return;
    }

    auto parent = item->parent();
    if (!parent) {
        return;
    }

    QVariant itemData = item->data(0, Qt::UserRole);
    auto path = itemData.toString();

    // Remove any children of the item
    for (int i = 0; i < item->childCount(); ++i) {
        removeItemFromTree(item->child(i));
    }

    pathsToScan.remove(path);
    watcher->removePath(path);
}

QDialog *
MainWindow::createConfirmationDialog(const QString &title, const QString &labelText, const QString &buttonText) {
    auto *dialog = new QDialog(this);
    dialog->setWindowTitle(title);
    auto *layout = new QVBoxLayout();
    dialog->setLayout(layout);
    auto *label = new QLabel(labelText);
    layout->addWidget(label);
    auto *confirmButton = new QPushButton(buttonText);
    layout->addWidget(confirmButton);
    connect(confirmButton, &QPushButton::clicked, dialog, &QDialog::accept);
    auto *cancelButton = new QPushButton("Cancel");
    layout->addWidget(cancelButton);
    connect(cancelButton, &QPushButton::clicked, dialog, &QDialog::reject);
    dialog->exec();

    return dialog;
}

void MainWindow::on_scanButton_clicked() {
    // Get all checked file types and patterns and convert them to std strings
    std::map<std::string, std::string> checkedFileTypes;
    for (int i = 0; i < fileTypesTableWidget->rowCount(); ++i) {
        if (fileTypesTableWidget->item(i, 0)->checkState() == Qt::Checked) {
            checkedFileTypes[fileTypesTableWidget->item(i, 1)->text().toStdString()] =
                    fileTypesTableWidget->item(i, 2)->text().toStdString();
        }
    }
    std::map<std::string, std::string> checkedScanPatterns;
    for (int i = 0; i < scanPatternsTableWidget->rowCount(); ++i) {
        if (scanPatternsTableWidget->item(i, 0)->checkState() == Qt::Checked) {
            checkedScanPatterns[scanPatternsTableWidget->item(i, 1)->text().toStdString()] =
                    scanPatternsTableWidget->item(i, 2)->text().toStdString();
        }
    }

    // Get all files in pathsToScan
    std::vector<std::string> filePaths;
    for (const auto &item: pathsToScan) {
        //If it is a file, add it to the list of files to scan
        if (!QFileInfo(item->data(0, Qt::UserRole).toString()).isDir()) {
            filePaths.push_back(item->data(0, Qt::UserRole).toString().toStdString());
        }
    }

    // Scan the files
    auto matches = fileScanner->scanFiles(filePaths, checkedScanPatterns, checkedFileTypes);
    // Delete all existing items in the flagged files tree widget then show new ones
    flaggedFilesTreeWidget->clear();
    for (const auto &match: matches) {
        // Create a QWidget with a QHBoxLayout, workaround for selectable text in QTreeWidget
        auto *item = new QTreeWidgetItem(flaggedFilesTreeWidget);
        auto *widget = new QWidget();
        auto *layout = new QHBoxLayout();
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(20);
        widget->setLayout(layout);
        auto *checkBox = new QCheckBox();
        auto qstringPath = QString::fromStdString(match.first);
        auto shortName = qstringPath.split("/").last(); // Get the file name only
        auto *label = new QLabel(shortName);
        label->setTextInteractionFlags(Qt::TextSelectableByMouse);
        QFont font = label->font();
        font.setPointSize(10);
        label->setFont(font);

        layout->addWidget(checkBox);
        layout->addWidget(label);
        layout->addStretch(1);

        flaggedFilesTreeWidget->setItemWidget(item, 0, widget);

        // Add full path of file as a child item
        auto *fullPathItem = new QTreeWidgetItem(item);
        auto *fullPathLabel = new QLabel("Full path: " + qstringPath);
        QFont fullPathFont = fullPathLabel->font();
        fullPathFont.setPointSize(9);
        fullPathLabel->setFont(fullPathFont);

        fullPathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        flaggedFilesTreeWidget->setItemWidget(fullPathItem, 0, fullPathLabel);
        fullPathItem->setFlags(fullPathItem->flags() & ~Qt::ItemIsSelectable);

        for (const auto &matchInfo: match.second) {
            auto *childItem = new QTreeWidgetItem(item);
            auto *childLabel = new QLabel(
                    QString::fromStdString(matchInfo.patternUsed.second) + ": found " +
                    QString::fromStdString(matchInfo.match) +
                    " from index " + QString::number(matchInfo.startIndex) + " to " +
                    QString::number(matchInfo.endIndex)
            );

            // Set child item to not be selectable
            childItem->setFlags(childItem->flags() & ~Qt::ItemIsSelectable);
            childLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
            flaggedFilesTreeWidget->setItemWidget(childItem, 0, childLabel);

            auto childFont = childLabel->font();
            childFont.setPointSize(9);
            childLabel->setFont(childFont);
        }

        flaggedItems[match.first] = item;
    }
}

void MainWindow::on_removeSelectedButton_2_clicked() {
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
        auto parent = item->parent();
        if (parent) {
            delete item;
            // if the parent item has no children, remove it from the tree
            if (parent->childCount() == 0 && parent != myRootItem) {
                removeItemFromTree(parent);
                delete parent;
            }
        }
    }
}


void MainWindow::on_selectAllFlaggedButton_clicked() {
    for (int i = 0; i < flaggedFilesTreeWidget->topLevelItemCount(); ++i) {
        // Get the checkbox from the layout on the top level item
        auto *topLevelItem = flaggedFilesTreeWidget->topLevelItem(i);
        auto *widget = flaggedFilesTreeWidget->itemWidget(topLevelItem, 0);
        auto *layout = widget->layout();
        auto *checkBox = qobject_cast<QCheckBox *>(layout->itemAt(0)->widget());
        if (allFlaggedSelected) {
            checkBox->setCheckState(Qt::Unchecked);
        } else {
            checkBox->setCheckState(Qt::Checked);
        }
    }

    // Toggle the flag
    allFlaggedSelected = !allFlaggedSelected;
    ui->selectAllFlaggedButton->setText(allFlaggedSelected ? "Deselect All" : "Select All");
}

void MainWindow::on_unflagSelectedButton_clicked() {
    // Get all checked items
    QList<QTreeWidgetItem *> checkedItems;
    for (int i = 0; i < flaggedFilesTreeWidget->topLevelItemCount(); ++i) {
        auto *topLevelItem = flaggedFilesTreeWidget->topLevelItem(i);
        auto *widget = flaggedFilesTreeWidget->itemWidget(topLevelItem, 0);
        auto *layout = widget->layout();
        auto *checkBox = qobject_cast<QCheckBox *>(layout->itemAt(0)->widget());

        if (checkBox->checkState() == Qt::Checked) {
            auto *childLabel = qobject_cast<QLabel *>(flaggedFilesTreeWidget->itemWidget(topLevelItem->child(0), 0));
            auto itemToRemove = childLabel->text().split(":").last().trimmed().toStdString();

            flaggedItems.remove(itemToRemove);
            checkedItems.append(topLevelItem);
        }
    }

    // Remove checked items from the tree
    while (!checkedItems.isEmpty()) {
        auto item = checkedItems.takeFirst();
        removeItemFromTree(item);
        delete item;
    }
}

void MainWindow::on_deleteButton_clicked() {
    if (flaggedItems.empty()) { return; }

    // Show confirmation dialog
    auto *dialog = createConfirmationDialog("Delete Files",
                                            "This will permanently delete the all flagged files. Do wish to proceed?",
                                            "Delete");

    if (dialog->result() == QDialog::Rejected) {
        return;
    }

    QList<std::string> flaggedItemsToRemove = flaggedItems.keys();
    std::vector<std::string> flaggedItemsToRemoveVector(flaggedItemsToRemove.begin(), flaggedItemsToRemove.end());
    fileScanner->deleteFiles(flaggedItemsToRemoveVector);

    // Remove all flagged files
    while (!flaggedItems.empty()) {
        auto item = flaggedItems.begin();
        removeItemFromTree(item.value());
        if (item.value()) {
            delete item.value();
        }
        flaggedItems.remove(item.key());
    }

    // Close the dialog
    dialog->close();
    delete dialog;
}