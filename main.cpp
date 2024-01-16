#include <QApplication>
#include <QPushButton>
#include <QQmlApplicationEngine>
#include <QtQuick>
#include <QUrl>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    // Show main_view.qml
    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/qml/main_view.qml")));

    return a.exec();
}
