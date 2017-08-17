import QtQuick 2.7
import QtQuick.Controls 2.2
import QtQuick.Dialogs 1.2 as Dialogs
import QtQuick.Layouts 1.3

import MrPlotterUI 1.0
import MrPlotter 0.1

Flickable {
    id: root
    signal selectedStreamsChanged(var selected, var deselected);

    boundsBehavior: Flickable.StopAtBounds
    clip: true

    contentWidth: root.width
    contentHeight: sourcepane.implicitHeight

    Component {
        id: btrdbStreamSelector

        BTrDBStreamTree2 {
            id: streamTree

            width: parent.width
            height: 300

            onSelectedStreamsChanged: function (selected, deselected) {
                root.selectedStreamsChanged(selected, deselected);
            }
        }
    }

    Component {
        id: btrdbDataSource

        BTrDBDataSource {
        }
    }

    Dialogs.MessageDialog {
        id: messageDialog
    }

    ColumnLayout {
        id: sourcepane
        anchors.fill: parent
        anchors.margins: 5
        spacing: 5

        Text {
            text: "Add Data Sources"
            font.pointSize: 20
        }

        /*Text {
            text: "Add a BOSSWAVE Archiver"
            font.bold: true
        }

        TextFieldWithButton {
            id: archiverSelector
            defaultText: "BOSSWAVE URI"
            buttonText: "+"
            toolTipText: "Example: gabe.pantry/durandal/s.giles/_/i.archiver"
            onButtonClicked: function (uri) {
                console.log(uri);
            }

            anchors.left: parent.left
            anchors.right: parent.right
        }*/

        Text {
            text: "Add a BTrDB Cluster"
            font.bold: true
        }

        TextFieldWithButton {
            id: btrdbSelector
            defaultText: "host:port"
            buttonText: "+"
            toolTipText: "Example: localhost:4410"
            onButtonClicked: function (addrport) {
                var btrdb = btrdbDataSource.createObject(null);
                var streamTree = btrdbStreamSelector.createObject(streamSelectionView);
                btrdb.connectAsync(addrport, function (res) {
                    if (res) {
                        streamTree.btrdb = btrdb;
                    } else {
                        streamTree.destroy();
                        messageDialog.title = "Error"
                        messageDialog.text = "Could not connect to BTrDB cluster at " + addrport;
                        messageDialog.visible = true;
                    }
                });
            }

            anchors.left: parent.left
            anchors.right: parent.right
        }

        Text {
            id: selectstreamstext
            text: "Select Streams"
            font.pointSize: 20
        }

        ColumnLayout {
            id: streamSelectionView

            anchors.top: selectstreamstext.bottom
            anchors.left: parent.left
            anchors.right: parent.right
        }
    }
}
