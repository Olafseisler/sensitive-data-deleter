#include <QApplication>
#include <QtMessageHandler>
#include "mainwindow.h"
#include <QDebug>

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    // Print with regular qt message handler 
    QMessageLogger(context.file, context.line, context.function).debug().noquote() << msg;
}

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    qInstallMessageHandler(messageHandler);
    MainWindow w;
    // Set main window title
    w.setWindowTitle("Sensitive Data Scan & Delete");
    w.show();

    return a.exec();
}
