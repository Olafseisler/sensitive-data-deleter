#include <QApplication>
#include <QPushButton>
#include <QQmlApplicationEngine>
#include "treemodel.h"
#include <QQmlContext>


int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    qputenv("QSG_INFO", "1" );
    qputenv("QSG_RHI_BACKEND", "opengl");

    auto treeModel = new TreeModel();

    // Show main_view.qml
    QQmlApplicationEngine engine;
//    engine.load(QUrl(QStringLiteral("qrc:/qml/main_view.qml")));
    QQmlContext* context = engine.rootContext();
    context->setContextProperty("_model", treeModel);

    const QUrl url(u"qrc:/qml/main_view.qml"_qs);


    QObject::connect(
            &engine,
            &QQmlApplicationEngine::objectCreated,
            &a,
            [url](QObject *obj, const QUrl &objUrl) {
                if (!obj && url == objUrl)
                    QCoreApplication::exit(-1);
            },
            Qt::QueuedConnection);


    // Add items to the tree model
    const auto item1 = new TreeItem("item1", treeModel->getRootItem());
    auto item2 = new TreeItem("item2", treeModel->getRootItem());
    auto item3 = new TreeItem("item3", treeModel->getRootItem());

    auto item11 = new TreeItem("item11", item1);
    auto item12 = new TreeItem("item12", item1);



    treeModel->addItem(item1);
    treeModel->addItem(item2);
    treeModel->addItem(item3);

    treeModel->addItem(item11, item1);
    treeModel->addItem(item12, item1);


    engine.load(url);

    return a.exec();
}
