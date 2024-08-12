#include "mainwindow.h"
#include <QDir>
#include <QFileSystemModel>
#include <QFileDialog>
#include <QCheckBox>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>
#include <QProgressDialog>
#include <QFileIconProvider>
#include <QMessageBox>
#include <filesystem>
#include <set>

#define MAX_DEPTH 10

namespace fs = std::filesystem;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    ui = new Ui::MainWindow;
    ui->setupUi(this);
    configManager = new ConfigManager();
    watcher = new QFileSystemWatcher(this);
    fileScanner = new FileScanner();
    setupUI();
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
    myRootItem->setExpanded(true);

    flaggedFilesTreeWidget = ui->flaggedFilesTreeWidget;
    fileTypesTableWidget = ui->fileTypesTableWidget;
    scanPatternsTableWidget = ui->scanPatternsTableWidget;
    fileTreeWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

    flaggedFilesTreeWidget->setColumnCount(1);
    flaggedFilesTreeWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    flaggedFilesTreeWidget->setSelectionMode(QAbstractItemView::MultiSelection);
    connect(watcher, &QFileSystemWatcher::directoryChanged, this, &MainWindow::onDirectoryChanged);

    fileTypesTableWidget->setColumnCount(4);
    scanPatternsTableWidget->setColumnCount(4);

    auto currentDate = QDate::currentDate();
    QTime latestTimeOnCurrentDate = QTime(23, 59, 0);
    ui->toDateEdit->setDate(currentDate);
    ui->toDateEdit->setDateTime(QDateTime(currentDate, latestTimeOnCurrentDate));

    // Connect the itemExpanded signal on the fileTreeWidget to load the child elements
    connect(fileTreeWidget, &QTreeWidget::itemExpanded, this, [this](QTreeWidgetItem *item) {
        auto path = item->data(0, Qt::UserRole).toString();
        if (pathsToScan.contains(path)) {
            updateTreeItem(item, path);
        }
        fileTreeWidget->resizeColumnToContents(0);
    });
    // Connect the itemCollapsed signal on the fileTreeWidget to remove invisible children
    connect(fileTreeWidget, &QTreeWidget::itemCollapsed, this, [this](QTreeWidgetItem *item) {
        if (item == myRootItem) {
            return;
        }
        auto childIndex = item->childCount() - 1;
        while (childIndex >= 0) {
            auto childItem = item->child(childIndex);
            removeItemFromTree(childItem);
            childIndex--;
        }
    });

    connect(fileScanner, &FileScanner::scanResultsBatchReady, this, &MainWindow::processScanResults);
    flaggedFilesTreeWidget->setHeaderLabel("Flagged Files");
    fileTreeWidget->setHeaderLabel("Files and Folders");

    updateConfigPresentation();
}

void MainWindow::updateConfigPresentation() {
    // Clear current items in the file types and scan patterns tables
    fileTypesTableWidget->clearContents();
    fileTypesTableWidget->setRowCount(0);
    scanPatternsTableWidget->clearContents();
    scanPatternsTableWidget->setRowCount(0);

    QList<QPair<QString, QString>> fileTypes = configManager->fileTypes;
    for (const auto &fileType: fileTypes) {
        int row = fileTypesTableWidget->rowCount();
        fileTypesTableWidget->insertRow(row);

        auto *checkBox = new QTableWidgetItem();
        checkBox->setCheckState(Qt::Checked);
        fileTypesTableWidget->setItem(row, 0, checkBox);
        fileTypesTableWidget->setItem(row, 1, new QTableWidgetItem(fileType.first));
        fileTypesTableWidget->setItem(row, 2, new QTableWidgetItem(fileType.second));

        // Add a button to the column to the right to remove the row
        auto *removeButton = new QPushButton("Remove");
        fileTypesTableWidget->setCellWidget(row, 3, removeButton);
        fileTypesTableWidget->setHorizontalHeaderItem(3, new QTableWidgetItem(""));

        // Connect the button to the slot to remove the row
        connect(removeButton, &QPushButton::clicked, this, [this, row]() {
            auto *confirmationDialog = createConfirmationDialog("Remove File Type",
                                                                "Are you sure you want to remove the file type for " +
                                                                this->fileTypesTableWidget->item(row, 2)->text() +
                                                                "?",
                                                                "Remove");
            if (confirmationDialog->result() == QDialog::Rejected) {
                return;
            }

            auto fileTypeToRemove = this->fileTypesTableWidget->item(row, 1)->text();
            this->fileTypesTableWidget->removeRow(row);
            this->configManager->removeFileType(fileTypeToRemove);
        });
    }


    // Connect the itemChanged slot for file types
    connect(fileTypesTableWidget, &QTableWidget::itemChanged, [this](QTableWidgetItem *item) {
        auto *column1 = fileTypesTableWidget->item(item->row(), 1);
        auto *column2 = fileTypesTableWidget->item(item->row(), 2);

        if (column1 == nullptr || column1->text().isEmpty()) {
            // Set the background to red if the file type is empty
            if (fileTypesTableWidget->item(item->row(), 1)) {
                fileTypesTableWidget->item(item->row(), 1)->setToolTip("File type can not be empty");
                fileTypesTableWidget->item(item->row(), 1)->setBackground(QBrush(QColor(255, 0, 0, 50)));
            }
            return;
        } else {
            // Set the background to white if the file type is not empty
            if (fileTypesTableWidget->item(item->row(), 1)) {
                fileTypesTableWidget->item(item->row(), 1)->setBackground(QBrush(QColor(0, 0, 0, 0)));
                fileTypesTableWidget->item(item->row(), 1)->setToolTip("");
            }
        }

        if (column2 == nullptr || column2->text().isEmpty()) {
            if (fileTypesTableWidget->item(item->row(), 2)) {
                fileTypesTableWidget->item(item->row(), 2)->setToolTip("Description can not be empty");
                fileTypesTableWidget->item(item->row(), 2)->setBackground(QBrush(QColor(255, 0, 0, 50)));
            }
            return;
        } else {
            // Set the background to transparent if the description is not empty
            if (fileTypesTableWidget->item(item->row(), 2)) {
                fileTypesTableWidget->item(item->row(), 2)->setBackground(QBrush(QColor(0, 0, 0, 0)));
                fileTypesTableWidget->item(item->row(), 2)->setToolTip("");
            }
        }

        // TODO:Check if the file type already exists in the file and inform the user

        QString newFileType = column1->text();
        QString newDescription = column2->text();
        configManager->editFileType(item->row(), newFileType, newDescription);
    });

    // Set column widths. The first column should be as small as possible, the second column should be 30% of the table width
    fileTypesTableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    fileTypesTableWidget->setColumnWidth(2, (int) (fileTypesTableWidget->width() * 0.3));
    fileTypesTableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);

    QList<QPair<QString, QString>> scanPatterns = configManager->scanPatterns;
    for (const auto &scanPattern: scanPatterns) {
        int row = scanPatternsTableWidget->rowCount();
        scanPatternsTableWidget->insertRow(row);

        auto *checkBox = new QTableWidgetItem();
        checkBox->setCheckState(Qt::Checked);
        scanPatternsTableWidget->setItem(row, 0, checkBox);
        scanPatternsTableWidget->setItem(row, 1, new QTableWidgetItem(scanPattern.first));
        scanPatternsTableWidget->setItem(row, 2, new QTableWidgetItem(scanPattern.second));

        // Set title for 4th column
        scanPatternsTableWidget->setHorizontalHeaderItem(3, new QTableWidgetItem(""));

        // Add a button to the column to the right to remove the row
        auto *removeButton = new QPushButton("Remove");
        scanPatternsTableWidget->setCellWidget(row, 3, removeButton);

        // Connect the button to the slot to remove the row
        connect(removeButton, &QPushButton::clicked, this, [this, row]() {
            auto *confirmationDialog = createConfirmationDialog("Remove Scan Pattern",
                                                                "Are you sure you want to remove this scan pattern?",
                                                                "Remove");
            if (confirmationDialog->result() == QDialog::Rejected) {
                return;
            }

            auto patternToRemove = this->scanPatternsTableWidget->item(row, 1)->text();
            this->scanPatternsTableWidget->removeRow(row);
            this->configManager->removeScanPattern(patternToRemove);
        });
    }

    scanPatternsTableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    scanPatternsTableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    scanPatternsTableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);

    // Update the config file if any item in fileTypesTableWidget is edited
    connect(scanPatternsTableWidget, &QTableWidget::itemChanged, [this](QTableWidgetItem *item) {
        auto *column1 = scanPatternsTableWidget->item(item->row(), 1);
        auto *column2 = scanPatternsTableWidget->item(item->row(), 2);

        if (column1 == nullptr || column1->text().isEmpty()) {
            if (scanPatternsTableWidget->item(item->row(), 1)) {
                scanPatternsTableWidget->item(item->row(), 1)->setToolTip("Scan pattern can not be empty");
                scanPatternsTableWidget->item(item->row(), 1)->setBackground(QBrush(QColor(255, 0, 0, 50)));
            }
            return;
        } else {
            if (scanPatternsTableWidget->item(item->row(), 1)) {
                scanPatternsTableWidget->item(item->row(), 1)->setBackground(QBrush(QColor(0, 0, 0, 0)));
                scanPatternsTableWidget->item(item->row(), 1)->setToolTip("");
            }
        }

        if (column2 == nullptr || column2->text().isEmpty()) {
            if (scanPatternsTableWidget->item(item->row(), 2)) {
                scanPatternsTableWidget->item(item->row(), 2)->setToolTip("Description can not be empty");
                scanPatternsTableWidget->item(item->row(), 2)->setBackground(QBrush(QColor(255, 0, 0, 50)));
            }
            return;
        } else {
            // Set the background to transparent if the description is not empty
            if (scanPatternsTableWidget->item(item->row(), 2)) {
                scanPatternsTableWidget->item(item->row(), 2)->setBackground(QBrush(QColor(0, 0, 0, 0)));
                scanPatternsTableWidget->item(item->row(), 2)->setToolTip("");
            }
        }

        if (!configManager->isValidRegex(column1->text())) {
            qDebug() << "The scan pattern is not a valid regex expression.";
            scanPatternsTableWidget->item(item->row(), 1)->setBackground(QBrush(QColor(255, 0, 0, 50)));
            scanPatternsTableWidget->item(item->row(), 1)->setToolTip(
                    "The scan pattern is not a valid regex expression.");
            return;
        } else {
            scanPatternsTableWidget->item(item->row(), 1)->setBackground(QBrush(QColor(0, 0, 0, 0)));
            scanPatternsTableWidget->item(item->row(), 1)->setToolTip("");
        }

        // Check if the scan pattern already exists
        for (int i = 0; i < scanPatternsTableWidget->rowCount(); ++i) {
            if (i == item->row()) {
                continue;
            }
            if (scanPatternsTableWidget->item(i, 1)->text() == column1->text()) {
                qDebug() << "The scan pattern already exists.";
                return;
            }
        }

        QString newScanPattern = column1->text();
        QString newDescription = column2->text();
        configManager->editScanPattern(item->row(), newScanPattern, newDescription);
    });
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
    if (!dir.exists() && item != myRootItem) {
        // Remove the item from the tree if it no longer exists
        removeItemFromTree(item);
        return;
    }

    // Clear the existing subtree items
    int childToRemoveIndex = item->childCount() - 1;
    while (childToRemoveIndex >= 0) {
        auto childItem = item->child(childToRemoveIndex);
        removeItemFromTree(childItem);
        childToRemoveIndex--;
    }

    for (const QString &entry: dir.entryList(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs,
                                             QDir::SortFlags(Qt::AscendingOrder))) {
        QString childPath = path + "/" + entry;
        QTreeWidgetItem *childItem = pathsToScan.value(childPath);
        if (!childItem) {
            childItem = createTreeItem(item, childPath, true);
            if (QFileInfo(childPath).isDir()) {
                childItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
            }
        }

        pathsToScan.insert(childPath, childItem);
        handleFlaggedScanItem(childPath.toStdString());
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

bool isSystemFile(const fs::path &path) {
    static const std::set<std::string> ignoredFiles = {
            ".DS_Store", "desktop.ini", "Thumbs.db"
    };

    // Check if the file is in the ignored list
    return ignoredFiles.find(path.filename().string()) != ignoredFiles.end();
}


void MainWindow::constructScanTreeViewRecursively(QTreeWidgetItem *parentItem, const QString &currentPath) {

    // Use std filesystem recursive iterator to add all folders and files into pathsToScan
    for (auto it = fs::recursive_directory_iterator(currentPath.toStdString());
         it != fs::recursive_directory_iterator(); ++it) {

        if (isSystemFile(it->path())) {
            continue;
        }
        int current_depth = it.depth();
        if (current_depth > MAX_DEPTH) {
            it.pop();  // Skip deeper directories
            continue;
        }

        // If a file or folder encountered already exists in the scan list,
        // it will be joined to the parent item and shown when parent is expanded
        QString path = QString::fromStdString(it->path().string());
        if (pathsToScan.contains(path)) {
            auto *existingItem = pathsToScan.value(path);

            if (existingItem) {
                removeItemFromTree(existingItem);
            }
        }

        if (it->is_directory()) {
            watcher->addPath(QString::fromStdString(it->path().string()));
        }
        pathsToScan.insert(path, nullptr);
    }

    // Only create the tree item if the path is that of the parent item
    // and the parent item is expanded
    if (parentItem->isExpanded()) {
        bool useShortName = parentItem != myRootItem;
        auto newItem = createTreeItem(parentItem, currentPath, useShortName);
        parentItem->sortChildren(0, Qt::AscendingOrder);
        pathsToScan[currentPath] = newItem;
        newItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    }
}

QString MainWindow::getParentPath(const QString &dirPath) {
    qsizetype lastSeparatorIndex = dirPath.lastIndexOf("/");
    QString parentPath = dirPath.left(lastSeparatorIndex);
    while (!parentPath.isEmpty()) {
        if (pathsToScan.contains(parentPath)) {
            auto *parentItem = pathsToScan.value(parentPath);
            return parentPath;
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
        QMessageBox::information(this, "No directory selected", "No directory selected.");
        return;
    }

    if (pathsToScan.contains(dirPath)) {
        qDebug() << "Directory already being watched";
        QMessageBox::information(this, "Directory already being watched",
                                 "The directory is already being watched.");
        return;
    }

    // Existing parent directory, append the new directory to the parent tree
    auto parentPath = getParentPath(dirPath);
    if (!parentPath.isEmpty()) {

        // Create new tree item for the directory
        auto parentItem = pathsToScan.value(parentPath);
        constructScanTreeViewRecursively(parentItem, dirPath);

        // Uncheck all items in the tree
        for (const auto &item: pathsToScan) {
            if (item)
                item->setCheckState(0, Qt::Unchecked);
        }

        watcher->addPath(dirPath);
        fileTreeWidget->resizeColumnToContents(0);
        return;
    }

    // Create a new tree item for the directory
    constructScanTreeViewRecursively(myRootItem, dirPath);
    fileTreeWidget->resizeColumnToContents(0);
    watcher->addPath(dirPath);
}

QString formatFileSize(qint64 size) {
    // Use if-else statements to check the order of magnitude of the file size
    if (size >= 0 && size <= 1023) {
        return QString::number(size) + " B";
    } else if (size >= 1024 && size <= 1048575) {
        return QString::number(size / 1024) + " KB";
    } else if (size >= 1048576 && size <= 1073741823) {
        return QString::number(size / 1048576) + " MB";
    } else {
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
        QFileInfo fileInfo(path);
        item->setIcon(0, iconProvider.icon(fileInfo));
        item->setText(1, formatFileSize(fileInfo.size()));
        item->setText(2, QFileInfo(path).lastModified().toString("dd/MM/yyyy"));
    } else {
        item->setIcon(0, iconProvider.icon(QFileIconProvider::Folder));
    }
    item->setCheckState(0, Qt::Unchecked);

    return item;
}

void MainWindow::on_addFileButton_clicked() {
    QStringList filePaths = QFileDialog::getOpenFileNames(this, tr("Select Files"), QDir::currentPath());
    if (filePaths.isEmpty()) {
        qDebug() << "No files selected";
        return;
    }

    for (const auto &filePath: filePaths) {
        if (pathsToScan.contains(filePath)) {
            qDebug() << "File " << filePath << " already being watched";
            continue;
        }

        // If a parent directory is already being watched, add the new file to the parent item
        QString parentPath = getParentPath(filePath);
        if (!parentPath.isEmpty()) {
            auto *parentItem = pathsToScan.value(parentPath);
            if (parentItem) {
                createTreeItem(parentItem, filePath, true);
            }
            continue;
        }

        createTreeItem(myRootItem, filePath, false);
    }

    // Uncheck all items in the tree
    for (const auto &item: pathsToScan) {
        item->setCheckState(0, Qt::Unchecked);
    }

    myRootItem->setExpanded(true);
    fileTreeWidget->resizeColumnToContents(1);
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
    int childToRemoveIndex = item->childCount() - 1;
    while (childToRemoveIndex >= 0) {
        auto childItem = item->child(childToRemoveIndex);
        removeItemFromTree(childItem);
        childToRemoveIndex--;
    }

    parent->removeChild(item);
    pathsToScan[path] = nullptr;
    delete item;
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

QDialog *MainWindow::createInfoDialog(const QString &title, const QString &labelText) {
    auto *dialog = new QDialog(this);
    dialog->setWindowTitle(title);
    auto *layout = new QVBoxLayout();
    dialog->setLayout(layout);
    auto *label = new QLabel(labelText);
    layout->addWidget(label);
    auto *closeButton = new QPushButton("Close");
    layout->addWidget(closeButton);
    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);
    dialog->exec();

    return dialog;
}

void MainWindow::expandToFlaggedItem(const QString &path) {
    QStringList pathParts = path.split("/");
    QString currentPath = pathParts[0];
    int index = 1;
    while (currentPath != path) {
        auto *item = pathsToScan.value(currentPath);
        if (item && !item->isExpanded()) {
            item->setExpanded(true);
        }
        currentPath += "/" + pathParts[index];
        index++;
    }

    // Resize the columns to fit the content
    fileTreeWidget->resizeColumnToContents(0);
}

void
MainWindow::handleFlaggedScanItem(const std::string &flaggedPath) {
    int columnCount = fileTreeWidget->columnCount();
    // Set the color of the row of the corresponding element in fileTreeWidget
    QTreeWidgetItem *scanTreeItem = pathsToScan.value(QString::fromStdString(flaggedPath));

    switch (scanResults.value(flaggedPath).first) {
        case ScanResult::CLEAN:
            setRowBackgroundColor(scanTreeItem, QColor(0, 255, 0, 50), columnCount);
            scanTreeItem->setToolTip(0, "No sensitive data found");
            break;
        case ScanResult::DIRECTORY_TOO_DEEP:
            setRowBackgroundColor(scanTreeItem, QColor(128, 128, 128, 50), columnCount);
            scanTreeItem->setToolTip(0, "Directory is too deep to scan");
            break;
        case ScanResult::FLAGGED:
            setRowBackgroundColor(scanTreeItem, QColor(255, 255, 0, 50), columnCount);
            scanTreeItem->setToolTip(0, "Sensitive data found");
            break;
        case ScanResult::UNSUPPORTED_TYPE:
            setRowBackgroundColor(scanTreeItem, QColor(150, 150, 150, 50), columnCount);
            scanTreeItem->setToolTip(0, "Unsupported file type");
            break;
        case ScanResult::UNREADABLE:
            setRowBackgroundColor(scanTreeItem, QColor(255, 0, 0, 50), columnCount);
            scanTreeItem->setToolTip(0, "Could not read file likely due to permissions");
            break;
        case ScanResult::FLAGGED_BUT_UNWRITABLE:
            setRowBackgroundColor(scanTreeItem, QColor(255, 120, 0, 50), columnCount);
            scanTreeItem->setToolTip(0,
                                     "File has sensitive data but cannot be written to or deleted due to permissions. This may cause issues.");
            break;
        default:
            break;
    }

}

void MainWindow::setRowBackgroundColor(QTreeWidgetItem *item, const QColor &color, int columnCount) {
    for (int column = 0; column < columnCount; ++column) {
        item->setBackground(column, QBrush(color));
    }
}

void MainWindow::getScanResultBits(const std::pair<std::string, std::pair<ScanResult, std::vector<MatchInfo>>> &match) {
    switch (match.second.first) {
        case ScanResult::CLEAN:
            scanResultBits = scanResultBits | 0b00000001;
            break;
        case ScanResult::DIRECTORY_TOO_DEEP:
            scanResultBits = scanResultBits | 0b00000010;
            break;
        case ScanResult::FLAGGED:
            scanResultBits = scanResultBits | 0b00000100;
            break;
        case ScanResult::UNSUPPORTED_TYPE:
            scanResultBits = scanResultBits | 0b00001000;
            break;
        case ScanResult::UNREADABLE:
            scanResultBits = scanResultBits | 0b00010000;
            break;
        case ScanResult::FLAGGED_BUT_UNWRITABLE:
            scanResultBits = scanResultBits | 0b00100000;
            break;
        default:
            break;
    }
}

QString getWarningMessage(uint8_t scanResultBits) {
    if (scanResultBits == 1) {
        return "No issues found during the scan.";
    }

    QString warningMessage = "The following issues were encountered during the scan:\n";
    if (scanResultBits & 0b00000010) {
        warningMessage += "Some directories were too deep to scan.\n";
    }
    if (scanResultBits & 0b00000100) {
        warningMessage += "Sensitive data was found in some files.\n";
    }
    if (scanResultBits & 0b00001000) {
        warningMessage += "Some files had unsupported file types.\n";
    }
    if (scanResultBits & 0b00010000) {
        warningMessage += "Some files could not be read due to permissions.\n";
    }
    if (scanResultBits & 0b00100000) {
        warningMessage += "Some scanned files can not be written to or deleted due to permissions.\n";
    }
    return warningMessage;
}

void
MainWindow::processScanResults(const std::map<std::string, std::pair<ScanResult, std::vector<MatchInfo>>> &results) {
    for (const auto &result: results) {
        scanResults[result.first] = result.second;
        QString qstringPath = QString::fromStdString(result.first);
        getScanResultBits(result);
        // If the item at given path does not exist, expand to it and it will be created
        if (!pathsToScan.value(qstringPath)) {
            if (result.second.first != ScanResult::CLEAN &&
                result.second.first != ScanResult::UNSUPPORTED_TYPE) {
                expandToFlaggedItem(qstringPath);
            }
        } else {
            handleFlaggedScanItem(result.first);
        }

        if (result.second.second.empty()) {
            continue;
        }
        // Create a QWidget with a QHBoxLayout, workaround for selectable text in QTreeWidget
        auto *item = new QTreeWidgetItem(flaggedFilesTreeWidget);

        auto *widget = new QWidget();
        auto *layout = new QHBoxLayout();
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(20);
        widget->setLayout(layout);
        auto *checkBox = new QCheckBox();
        auto shortName = qstringPath.split("/").last(); // Get the file name only
        auto *label = new QLabel(shortName);
        label->setTextInteractionFlags(Qt::TextSelectableByMouse);
        QFont font = label->font();
        font.setPointSize(10);
        label->setFont(font);

        layout->addWidget(checkBox);
        layout->addWidget(label);
        layout->addStretch(1);

        flaggedItems[result.first] = item;

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

        for (const auto &matchInfo: result.second.second) {
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
    }
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
    std::vector<std::pair<std::string, std::string>> checkedScanPatterns;
    for (int i = 0; i < scanPatternsTableWidget->rowCount(); ++i) {
        if (scanPatternsTableWidget->item(i, 0)->checkState() == Qt::Checked) {
            checkedScanPatterns.emplace_back(scanPatternsTableWidget->item(i, 1)->text().toStdString(),
                                             scanPatternsTableWidget->item(i, 2)->text().toStdString());
        }
    }

    // If there are no file types or scan patterns selected, show a popup and return
    if (checkedFileTypes.empty() || checkedScanPatterns.empty()) {
        createInfoDialog("No file types or scan patterns selected",
                         "Please select at least one file type and one scan pattern to scan.");
        return;
    }

    // Get all files in pathsToScan that have been last edited in the given time period
    std::vector<std::string> filePaths;
    for (auto it = pathsToScan.constBegin(); it != pathsToScan.constEnd(); ++it) {
        const auto &key = it.key();
        // Check if the path is a file and if the last edited date is within range
        QFileInfo fileInfo(key);
        if (fileInfo.isFile() &&
            (fileInfo.lastModified() >= ui->fromDateEdit->dateTime() &&
             fileInfo.lastModified() <= ui->toDateEdit->dateTime())) {
            filePaths.push_back(key.toStdString());
        }
    }

    size_t originalFilePathsSize = filePaths.size();
    if (filePaths.empty()) {
        qDebug() << "No files to scan.";
        // Show popup for no files to scan
        QMessageBox::warning(this, "No files to scan",
                             "No files to scan. Please add files or directories to scan.");
        return;
    }
    flaggedFilesTreeWidget->clear();
    flaggedItems.clear();
    scanResults.clear();
    ui->flaggedSearchBox->clear();
    scanResultBits = 0;

    // Scan the files in a separate thread,
    auto *futureWatcher = new QFutureWatcher<std::map<std::string, std::pair<ScanResult, std::vector<MatchInfo>>>>(
            this);

    if (futureWatcher->isRunning()) {
        return;
    }

    auto future = QtConcurrent::run(&FileScanner::scanFiles, fileScanner, filePaths, checkedScanPatterns,
                                    checkedFileTypes);

    auto *progressDialog = new QProgressDialog("Scanning in progress", "Cancel", 0, 100);
    progressDialog->setAutoReset(false);
    progressDialog->setMinimumDuration(0);
    QObject::connect(progressDialog, &QProgressDialog::canceled, [futureWatcher, progressDialog]() {
        if (futureWatcher->future().isFinished()) { return; }
        futureWatcher->future().cancel();
        progressDialog->close();
        progressDialog->deleteLater();
        futureWatcher->deleteLater();
    });

    QObject::connect(futureWatcher,
                     &QFutureWatcher<std::map<std::string, std::vector<MatchInfo>>>::progressValueChanged,
                     [progressDialog](int progress) {
                         // Update progress
                         progressDialog->setValue(progress);
                     });

    QObject::connect(futureWatcher, &QFutureWatcher<std::map<std::string, std::vector<MatchInfo>>>::finished,
                     [futureWatcher, this, originalFilePathsSize, progressDialog]() {
                         try {
                             auto results = futureWatcher->result();  // Get the results when finished
                             qDebug() << "Scan task returned";
                             if (futureWatcher->future().isCanceled()) { return; }
                             progressDialog->setValue(100);
                             progressDialog->setLabelText("Constructing results...");
                             processScanResults(results);

                             progressDialog->setLabelText("Done. " + getWarningMessage(scanResultBits));


                             futureWatcher->deleteLater();
                             // Disconnect the "cancel button" signal and connect the close button
                             progressDialog->disconnect();
                             progressDialog->setCancelButtonText("Close");
                             QObject::connect(progressDialog, &QProgressDialog::canceled, [progressDialog]() {
                                 progressDialog->close();
                                 progressDialog->deleteLater();
                             });
                             qDebug() << "Processed " << originalFilePathsSize << " files.";
                         } catch (const std::exception &e) {
                             qDebug() << "An error occurred while processing the scan results: " << e.what();
                             futureWatcher->deleteLater();
                             progressDialog->close();
                             progressDialog->deleteLater();
                             QMessageBox::critical(this, "Error", QString::fromStdString(e.what()));
                         }
                     });
    futureWatcher->setFuture(future);
}

void MainWindow::on_removeSelectedButton_2_clicked() {
    // Get all checked items, only considering topmost checked items
    QList<QTreeWidgetItem *> checkedItems;

    for (auto &item: pathsToScan) {
        if (item && item->checkState(0) == Qt::Checked) {
            bool isTopmost = true;
            // Check if the current item has any ancestors in the checkedItems list
            QTreeWidgetItem *parent = item->parent();
            while (parent) {
                if (checkedItems.contains(parent)) {
                    isTopmost = false;
                    break;
                }
                parent = parent->parent();
            }

            if (isTopmost) {
                checkedItems.append(item);
            }
        }
    }

    if (checkedItems.isEmpty()) {
        qDebug() << "No items selected for removal.";
        QMessageBox::information(this, "No items selected", "No items selected for removal.");
        return;
    }

    // Remove the checked items and their children
    while (!checkedItems.isEmpty()) {
        auto item = checkedItems.takeFirst();
        QString itemPath = item->data(0, Qt::UserRole).toString();
        removeItemFromTree(item);
        pathsToScan.remove(itemPath);
        // Set the scan result to clean
        scanResults[itemPath.toStdString()].first = ScanResult::UNDEFINED;
        watcher->removePath(itemPath);
        // Remove all flagged items that are children of the removed item
        for (auto iter = pathsToScan.begin(); iter != pathsToScan.end();) {
            if (iter.key().startsWith(itemPath)) {
                scanResults[iter.key().toStdString()].first = ScanResult::UNDEFINED;
                iter = pathsToScan.erase(iter);
            } else {
                ++iter;
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
            auto *childLabel = qobject_cast<QLabel *>(
                    flaggedFilesTreeWidget->itemWidget(topLevelItem->child(0), 0));
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

void MainWindow::on_addPatternButton_clicked() {
    // Add a new row to the scan patterns table
    int row = scanPatternsTableWidget->rowCount();
    scanPatternsTableWidget->insertRow(row);

    // Initialize the new checkbox item with check state
    auto *newCheckBox = new QTableWidgetItem("");
    newCheckBox->setFlags(newCheckBox->flags() | Qt::ItemIsUserCheckable); // Ensure the item is checkable
    newCheckBox->setCheckState(Qt::Unchecked); // Initialize with unchecked state

    // Debug output to verify item initialization
    qDebug() << "Inserting new item at row:" << row << "with initial check state:" << newCheckBox->checkState();

    // Set the new checkbox item in the first column
    scanPatternsTableWidget->setItem(row, 0, newCheckBox);

    // Add other items to the new row
    scanPatternsTableWidget->setItem(row, 1, new QTableWidgetItem(""));
    scanPatternsTableWidget->setItem(row, 2, new QTableWidgetItem(""));

    // Set the new row to be checked by default
    scanPatternsTableWidget->item(row, 0)->setCheckState(Qt::Checked);
    // Add a remove button to the new row
    auto *removeButton = new QPushButton("Remove");
    scanPatternsTableWidget->setCellWidget(row, 3, removeButton);

    // Connect the button to the slot to remove the row
    connect(removeButton, &QPushButton::clicked, this, [this, row]() {
        auto *confirmationDialog = createConfirmationDialog("Remove Scan Pattern",
                                                            "Are you sure you want to remove this scan pattern?",
                                                            "Remove");
        if (confirmationDialog->result() == QDialog::Rejected) {
            return;
        }

        auto patternToRemove = this->scanPatternsTableWidget->item(row, 1)->text();
        this->scanPatternsTableWidget->removeRow(row);
        this->configManager->removeScanPattern(patternToRemove);
    });

    // Start typing in the new row
    scanPatternsTableWidget->editItem(scanPatternsTableWidget->item(row, 1));

    // Debug output to verify item state after modification
    qDebug() << "New item at row:" << row << "with final check state:"
             << scanPatternsTableWidget->item(row, 0)->checkState();
}

void MainWindow::on_addFiletypeButton_clicked() {
    // Add a new row to the file types table
    int row = fileTypesTableWidget->rowCount();
    fileTypesTableWidget->insertRow(row);

    // Initialize the new checkbox item with check state
    auto *newCheckBox = new QTableWidgetItem("");
    newCheckBox->setFlags(newCheckBox->flags() | Qt::ItemIsUserCheckable); // Ensure the item is checkable
    newCheckBox->setCheckState(Qt::Unchecked); // Initialize with unchecked state

    // Debug output to verify item initialization
    qDebug() << "Inserting new item at row:" << row << "with initial check state:" << newCheckBox->checkState();

    // Set the new checkbox item in the first column
    fileTypesTableWidget->setItem(row, 0, newCheckBox);

    // Add other items to the new row
    fileTypesTableWidget->setItem(row, 1, new QTableWidgetItem(""));
    fileTypesTableWidget->setItem(row, 2, new QTableWidgetItem(""));

    // Set the new row to be checked by default
    fileTypesTableWidget->item(row, 0)->setCheckState(Qt::Checked);
    // Add a remove button to the new row
    auto *removeButton = new QPushButton("Remove");
    fileTypesTableWidget->setCellWidget(row, 3, removeButton);

    // Connect the button to the slot to remove the row
    connect(removeButton, &QPushButton::clicked, this, [this, row]() {
        auto *confirmationDialog = createConfirmationDialog("Remove File Type",
                                                            "Are you sure you want to remove this file type?",
                                                            "Remove");
        if (confirmationDialog->result() == QDialog::Rejected) {
            return;
        }

        auto fileTypeToRemove = this->fileTypesTableWidget->item(row, 1)->text();
        this->fileTypesTableWidget->removeRow(row);
        this->configManager->removeFileType(fileTypeToRemove);
    });

    // Start typing in the new row
    fileTypesTableWidget->editItem(fileTypesTableWidget->item(row, 1));

    // Debug output to verify item state after modification
    qDebug() << "New item at row:" << row << "with final check state:"
             << fileTypesTableWidget->item(row, 0)->checkState();
}

/**
 * Select an alternative existing config file
 */
void MainWindow::on_newConfigButton_clicked() {
    // Open a file dialog to load alternative config file
    QString fileName = QFileDialog::getOpenFileName(this, tr("Load alternative configuration File"),
                                                    QDir::currentPath(),
                                                    tr("JSON Files (*.json)"));
    if (fileName.isEmpty()) {
        qDebug() << "No file selected";
        createInfoDialog("No file selected", "No file selected. Configuration not saved.");
        return;
    }

    QFile file(fileName);
    // If the file is empty or nonexistent
    if (!file.exists()) {
        qDebug() << "File " << fileName << " does not exist";
        createInfoDialog("File does not exist",
                         "The selected file does not exist or could not be found or opened.");
        return;
    }

    configManager->setConfigFilePath(fileName);
    configManager->loadConfigFromFile(fileName);
    updateConfigPresentation();
}


/**
 * Start a new fresh config
 */
void MainWindow::on_startConfigButton_clicked() {
    // Open a file dialog to save the new config file
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save new configuration File"),
                                                    QDir::currentPath(),
                                                    tr("JSON Files (*.json)"));
    if (fileName.isEmpty()) {
        qDebug() << "No file selected";
        createInfoDialog("No file selected", "No file selected. Configuration not saved.");
        return;
    }

    if (!fileName.endsWith(".json")) {
        qDebug() << "Invalid file extension";
        createInfoDialog("Invalid file extension", "Please select a file with a .json extension.");
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Could not open file " << fileName << " for writing";
        createInfoDialog("Could not open file", "Could not open the file for writing. Please try again.");
        return;
    }
    file.close();


    configManager->setConfigFilePath(fileName);
    configManager->scanPatterns.clear();
    configManager->fileTypes.clear();
    configManager->updateConfigFile();
    configManager->loadConfigFromFile(fileName);
    updateConfigPresentation();
}


void MainWindow::on_flaggedSearchBox_textEdited(const QString &newText) {
    std::string newTextStdString = newText.toStdString();
    std::transform(newTextStdString.begin(), newTextStdString.end(), newTextStdString.begin(), ::tolower);

    for (const auto &key: scanResults.keys()) {
        auto matchPair = scanResults[key];
        for (const auto &matchItem: matchPair.second) {
            std::string filepathString = key;
            std::string matchString = matchItem.match;
            std::string patternString = matchItem.patternUsed.second;
            std::transform(filepathString.begin(), filepathString.end(), filepathString.begin(), ::tolower);
            std::transform(matchString.begin(), matchString.end(), matchString.begin(), ::tolower);
            std::transform(patternString.begin(), patternString.end(), patternString.begin(), ::tolower);
            if (matchString.find(newTextStdString) != std::string::npos ||
                patternString.find(newTextStdString) != std::string::npos ||
                filepathString.find(newTextStdString) != std::string::npos) {
                flaggedItems[key]->setHidden(false);
                break;
            } else {
                flaggedItems[key]->setHidden(true);
            }
        }
    }
}

