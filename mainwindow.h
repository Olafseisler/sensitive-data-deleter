//
// Created by Olaf Seisler on 01.02.2024.
//

#ifndef SENSITIVE_DATA_DELETER_MAINWINDOW_H
#define SENSITIVE_DATA_DELETER_MAINWINDOW_H

#include <QMainWindow>
#include <QTreeWidget>
#include <QTableWidget>
#include <QFileSystemWatcher>
#include <QMap>
#include <QDebug>

#include "ui_mainwindow.h"
#include "configmanager.h"
#include "filescanner.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    // ... other members and methods ...

private slots:
    void onDirectoryChanged(const QString &path);
    void on_addFolderButton_clicked();
    void on_addFileButton_clicked();
    void on_removeSelectedButton_2_clicked();
    void on_scanButton_clicked();
    void on_selectAllFlaggedButton_clicked();
    void on_unflagSelectedButton_clicked();
    void on_addPatternButton_clicked();
    void on_deleteButton_clicked();

private:
    QTreeWidget *fileTreeWidget;
    QTreeWidget *flaggedFilesTreeWidget;
    QTableWidget *fileTypesTableWidget;
    QTableWidget *scanPatternsTableWidget;
    QDateEdit *fromDateEdit;
    QDateEdit *toDateEdit;
    QMap<QString, QTreeWidgetItem *> pathsToScan;
    ConfigManager *configManager;
    QTreeWidgetItem *myRootItem;
    QFileSystemWatcher *watcher;
    FileScanner *fileScanner;
    bool allFlaggedSelected = false;
    QString lastUpdatedPath = "";
    QMap<std::string, QTreeWidgetItem *> flaggedItems;
    Ui::MainWindow *ui;

    void setupUI();
    void constructScanTreeViewRecursively(QTreeWidgetItem *parentItem, QString &path, int depth = 0, bool useShortName = false);
    void updateTreeItem(QTreeWidgetItem *item, const QString &path);
    QTreeWidgetItem* createTreeItem(QTreeWidgetItem *parentItem, const QString &path, bool useShortName);
    void removeItemFromTree(QTreeWidgetItem *item);
    void onItemChanged(QTreeWidgetItem *item, int column);
    QString getParentPath(const QString &dirPath);
    QTreeWidgetItem* findItemForPath(QTreeWidget* treeWidget, const QString& path);
    QTreeWidgetItem* findItemForPath(QTreeWidgetItem* parentItem, const QString& path);
    QDialog* createConfirmationDialog(const QString &title, const QString &labelText, const QString &buttonText);
    void processScanResults(std::map<std::string, std::vector<MatchInfo>> &matches);
};


#endif //SENSITIVE_DATA_DELETER_MAINWINDOW_H
