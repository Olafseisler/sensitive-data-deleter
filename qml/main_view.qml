import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

ApplicationWindow {
    id: window
    height: 600
    width: 1000
    visible: true

    TabBar {
        id: tabBar
        width: parent.width
        height: 40
        currentIndex: 0
        TabButton {
            width: 150
            // add margins to the text
            contentItem: Text {
                text: "Scan and Delete"
                font: control.font
                opacity: control.opacity
                color: control.down ? "#17a81a" : "#21be2b"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
                // add margins to the text
                leftPadding: 10
                rightPadding: 10
                topPadding: 5
                bottomPadding: 5
            }
        }
        TabButton {
            width: 150
            // add margins to the text
            contentItem: Text {
                text: "Configuration"
                font: control.font
                opacity: control.opacity
                color: control.down ? "#17a81a" : "#21be2b"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
                // add margins to the text
                leftPadding: 10
                rightPadding: 10
                topPadding: 5
                bottomPadding: 5
            }
        }
    }

StackLayout {
        id: stackLayout
        anchors.top: tabBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        currentIndex: tabBar.currentIndex
        Rectangle {
            id: scanAndDeleteView
            Text {
                anchors.centerIn: parent
                text: "Scan and Delete"
            }
            // Add Tree Widget
        }
    }
}
