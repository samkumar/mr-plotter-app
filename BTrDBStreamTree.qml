import QtQml.Models 2.2
import QtQuick 2.0
import QtQuick.Controls 1.4 as QtQuickControls1
import QtQuick.Layouts 1.3

QtQuickControls1.TreeView {
    id: root
    property var streamTreeModel;
    signal selectedStreamsChanged(var selected, var deselected);

    anchors.left: parent.left
    anchors.right: parent.right

    Layout.preferredHeight: 300

    /*
     * This is a workaround to what I believe is a bug in Qt.
     * If the scrollbar is present and you click and drag up to start a
     * selection, and then drag your mouse out of the TreeView, the
     * process segfaults.
     * My solution is to always hide the scrollbars...
     */
    verticalScrollBarPolicy: Qt.ScrollBarAlwaysOff
    horizontalScrollBarPolicy: Qt.ScrollBarAlwaysOff

    model: streamTreeModel
    selectionMode: QtQuickControls1.SelectionMode.MultiSelection
    selection: ItemSelectionModel {
        id: itemSelectionModel
        model: streamTreeModel
        onSelectionChanged: function (selected, deselected) {
            /*
             * In this function, SELECTED and DESELECTED are of the type
             * QItemSelectionRange. What we actually want are objects
             * describing the streams that were selected and deselected.
             * So, we find the streams selected/deselected, and send
             * out that information with the selectedStreamsChanged signal.
             */
            var selectedStreams = streamTreeModel.getStreams(selected);
            var deselectedStreams = streamTreeModel.getStreams(deselected);
            if (selectedStreams.length !== 0 || deselectedStreams.length !== 0) {
                root.selectedStreamsChanged(selectedStreams, deselectedStreams);
            }
        }
    }

    QtQuickControls1.TableViewColumn {
        title: "Name"
        role: "name"
        width: 100
    }

    QtQuickControls1.TableViewColumn {
        title: "Tags"
        role: "tags"
        width: 100
    }

    QtQuickControls1.TableViewColumn {
        title: "Annotations"
        role: "annotations"
        width: 100
    }
}
