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
#include <QProgressDialog>
#include <QFileIconProvider>
#include <QTimer>
#include <map>

#include "ui_mainwindow.h"
#include "configmanager.h"
#include "filescanner.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    ~MainWindow() override;

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

    void on_addFiletypeButton_clicked();

    void on_newConfigButton_clicked();

    void on_startConfigButton_clicked();

    void on_flaggedSearchBox_textEdited();

    void startScanOperation(const std::vector<std::string> &filePaths, const std::map<std::string, std::string> &checkedScanPatterns,
                                    const std::vector<std::pair<std::string, std::string>> &checkedFileTypes);

public slots:

    void onFlaggedFilesScrollBarMoved(int value);

    void onSearchBoxTextEdited(const QString &newText);

    void processScanResults(const std::map<std::string, std::pair<ScanResult, std::vector<MatchInfo>>> &results);

private:
    QTreeWidget *fileTreeWidget;
    QTreeWidget *flaggedFilesTreeWidget;
    QTableWidget *fileTypesTableWidget;
    QTableWidget *scanPatternsTableWidget;
    QDateEdit *fromDateEdit;
    QDateEdit *toDateEdit;
    QMap<QString, QTreeWidgetItem *> pathsToScan;

    QMap<std::string, QTreeWidgetItem *> flaggedItems;
    QMap<std::string, std::pair<ScanResult, std::vector<MatchInfo>>> scanResults;

    ConfigManager *configManager;
    QTreeWidgetItem *myRootItem;
    QFileSystemWatcher *watcher;
    FileScanner *fileScanner;
    bool allFlaggedSelected = false;
    bool maxDepthReached = false;
    QString lastUpdatedPath = "";
    QFileIconProvider iconProvider = QFileIconProvider();
    Ui::MainWindow *ui;
    uint8_t scanResultBits = 0;
    int numFlaggedItemsLoaded = 0;
    int numFlaggedFiles = 0;

    QTimer *searchDebounceTimer;

    void setupUI();

    void constructScanTreeViewRecursively(QTreeWidgetItem *parentItem, const QString &path);

    void updateTreeItem(QTreeWidgetItem *item, const QString &path);

    QTreeWidgetItem *createTreeItem(QTreeWidgetItem *parentItem, const QString &path, bool useShortName);

    void removeItemFromTree(QTreeWidgetItem *item);

    QString getParentPath(const QString &dirPath);

    void expandToFlaggedItem(const QString &path);

    QDialog *createConfirmationDialog(const QString &title, const QString &labelText, const QString &buttonText);

    QDialog *createInfoDialog(const QString &title, const QString &labelText);

    void getScanResultBits(const std::pair<std::string, std::pair<ScanResult, std::vector<MatchInfo>>> &match);

    void setRowBackgroundColor(QTreeWidgetItem *item, const QColor &color, int columnCount);

    void addFlaggedItemWidget(const QString &path, const std::vector<MatchInfo> &matches);

    void loadNextFlaggedItemsBatch(const QString &searchText = "");

    void handleFlaggedScanItem(const std::string &flaggedPath);

    bool isStringInMatchInfo(const MatchInfo &match, const std::string &path, const std::string &searchString);

    std::vector<std::string> addFilesToScanList();
   
    void updateConfigPresentation();
};

#endif //SENSITIVE_DATA_DELETER_MAINWINDOW_H
