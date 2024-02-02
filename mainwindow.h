//
// Created by Olaf Seisler on 01.02.2024.
//

#ifndef SENSITIVE_DATA_DELETER_MAINWINDOW_H
#define SENSITIVE_DATA_DELETER_MAINWINDOW_H

#include <QMainWindow>
#include <QTreeWidget>
#include <QFileSystemWatcher>

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

private:
    QTreeWidget *fileTreeWidget;
    QTableView *flaggedFilesTableView;
    QFileSystemWatcher *watcher;

    void setupTreeWidget();
    void setupFileSystemWatcher();
    void updateTreeItem(QTreeWidgetItem *item, const QString &path);
    QTreeWidgetItem* findItemForPath(QTreeWidget* treeWidget, const QString& path);
    QTreeWidgetItem* findItemForPath(QTreeWidgetItem* parentItem, const QString& path);
    Ui::MainWindow *ui;
};


#endif //SENSITIVE_DATA_DELETER_MAINWINDOW_H
