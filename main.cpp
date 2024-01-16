#include <QApplication>
#include <QPushButton>
#include <QQmlApplicationEngine>
#include <QtQuick>
#include <QUrl>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    qputenv("QSG_INFO", "1" );
    qputenv("QSG_RHI_BACKEND", "opengl");

    // Show main_view.qml
    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/qml/main_view.qml")));

    return a.exec();
}
