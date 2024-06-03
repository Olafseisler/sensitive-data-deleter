#include <QApplication>
#include <QPushButton>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    MainWindow w;
    // Set main window title
    w.setWindowTitle("Sensitive Data Scan & Delete");
    w.show();

    return a.exec();
}
