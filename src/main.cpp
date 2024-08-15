#include <QApplication>
#include <QtMessageHandler>
#include "mainwindow.h"
#include <QDebug>

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
#ifndef QT_DEBUG
    if (type == QtDebugMsg || type == QtWarningMsg || type == QtCriticalMsg) {
        return; // Suppress output in release mode
    }
#else
    // Print with regular qt message handler in debug mode
    QMessageLogger(context.file, context.line, context.function).debug().noquote() << msg;
#endif
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
