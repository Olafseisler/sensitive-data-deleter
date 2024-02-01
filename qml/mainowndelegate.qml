import QtQuick

Window {
    height: 400
    visible: true
    width: 600

    TreeView {
        id: _view

        anchors.fill: parent

        // The model needs to be a QAbstractItemModel
        model: _model

        delegate: Item {
            id: treeDelegate

            required property int depth
            required property bool expanded
            required property int hasChildren
            readonly property real indent: 20
            required property bool isTreeNode
            readonly property real padding: 5

            // Assigned to by TreeView:
            required property TreeView treeView

            implicitHeight: label.implicitHeight * 1.5
            implicitWidth: padding + label.x + label.implicitWidth + padding

            TapHandler {
                onTapped: {
                    treeView.toggleExpanded(row);
                    var qi = _view.index(row, 0);
                    console.log(qi);
                    _view.model.showQModelIndex(qi);
                }
            }
            Text {
                id: indicator

                anchors.verticalCenter: label.verticalCenter
                rotation: treeDelegate.expanded ? 90 : 0
                text: "▸"
                visible: treeDelegate.isTreeNode && treeDelegate.hasChildren
                x: padding + (treeDelegate.depth * treeDelegate.indent)
            }
            Text {
                id: label

                clip: true
                text: model.display
                width: treeDelegate.width - treeDelegate.padding - x
                x: padding + (treeDelegate.isTreeNode ? (treeDelegate.depth + 1) * treeDelegate.indent : 0)
            }
        }
    }
}