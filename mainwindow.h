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

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    // ... other members and methods ...

private slots:
    void onDirectoryChanged(const QString &path);
    void on_addFolderButton_clicked();
    void on_addFileButton_clicked();
    void on_deleteSelectedButton_clicked();

private:
    QTreeWidget *fileTreeWidget;
    QTreeWidget *flaggedFilesTreeWidget;
    QTableWidget *fileTypesTableWidget;
    QTableWidget *scanPatternsTableWidget;
    QMap<QString, QTreeWidgetItem *> pathsToScan;

    QFileSystemWatcher *watcher;

    void setupTreeWidget();
    void setupFileSystemWatcher();
    void constructScanTreeViewRecursively(QTreeWidgetItem *parentItem, QString &path, int depth = 0, bool useShortName = false);
    void updateTreeItem(QTreeWidgetItem *item, const QString &path);
    void createTreeItem(QTreeWidgetItem *parentItem, const QString &path, bool useShortName);
    void removeItemFromTree(QTreeWidgetItem *item);
    void onItemChanged(QTreeWidgetItem *item, int column);
    QString getParentPath(const QString &dirPath);
    QTreeWidgetItem* findItemForPath(QTreeWidget* treeWidget, const QString& path);
    QTreeWidgetItem* findItemForPath(QTreeWidgetItem* parentItem, const QString& path);
    Ui::MainWindow *ui;
};


#endif //SENSITIVE_DATA_DELETER_MAINWINDOW_H
