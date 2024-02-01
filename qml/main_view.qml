import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

ApplicationWindow {
    id: window

    height: 600
    visible: true
    width: 1000

    TabBar {
        id: tabBar

        currentIndex: 0
        height: 40
        width: parent.width

        TabButton {
            width: 150

            // add margins to the text
            contentItem: Text {
                bottomPadding: 5
                color: control.down ? "#17a81a" : "#21be2b"
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignHCenter
                // add margins to the text
                leftPadding: 10
                opacity: control.opacity
                rightPadding: 10
                text: "Scan and Delete"
                topPadding: 5
                verticalAlignment: Text.AlignVCenter
            }
        }
        TabButton {
            width: 150

            // add margins to the text
            contentItem: Text {
                bottomPadding: 5
                color: control.down ? "#17a81a" : "#21be2b"
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignHCenter
                // add margins to the text
                leftPadding: 10
                opacity: control.opacity
                rightPadding: 10
                text: "Configuration"
                topPadding: 5
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
    StackLayout {
        id: stackLayout

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: tabBar.bottom
        currentIndex: tabBar.currentIndex

         Rectangle { // Left pane
            id: scanAndDeleteView
            width: parent.width - 20
            height: parent.height - 20
            anchors.left: parent.left
            anchors.right: parent.horizontalCenter

            Text {
                text: "Scan and Delete"
                anchors.top: parent.bottom
                anchors.leftMargin: 10
                anchors.topMargin: 10
            }
            // Add TreeView
            TreeView {
                id: treeView

                anchors.left: parent.left
                anchors.right: parent.right
                width: parent.width
                height: parent.height - 20
                model: _model

                delegate: TreeViewDelegate {
                    id: treeDelegate

                    onClicked: {
                        var mi = treeDelegate.treeView.index(row, column);
                        console.log("model.index =", mi);
                        treeView.model.showQModelIndex(mi);
                    }
                }
            }
        }
    }
}
