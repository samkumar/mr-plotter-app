import QtQuick 2.7
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import "streamtree.js" as StreamTreeUtils

Rectangle {
    id: root
    property var btrdb;
    property var collectionSearchTerms;
    property var streamSearchTerms;
    signal selectedStreamsChanged(var selected, var deselected);

    Component.onCompleted: {
        collectionSearchTerms = [];
        streamSearchTerms = [];
    }

    /* Loading sign when connecting to BTrDB. */
    BusyIndicator {
        id: connectingSign
        running: true
    }

    ColumnLayout {
        id: mainColumn
        anchors.fill: parent
        visible: false

        /* Some textfields used to search */
        TextFieldWithButton {
            id: collectionSearch
            defaultText: "Search collection"
            buttonText: "\uD83D\uDD0D Collections"
            toolTipText: "Enter keywords that appear in the collection name, separated by spaces.\nExample: Hunter"
            onButtonClicked: function (enteredText) {
                var keywords = enteredText.split(/(\s+)/);
                StreamTreeUtils.filterTrivialKeywords(keywords);
                collectionSearchTerms = keywords;
                StreamTreeUtils.applyCollectionSearch(keywords);
            }
        }

        TextFieldWithButton {
            id: streamSearch
            defaultText: "Search streams"
            buttonText: "\uD83D\uDD0D Streams"
            toolTipText: "Use keywords, separated by spaces, to search for a stream.\nExamples: L1; V; ad44e37"
            onButtonClicked: function (enteredText) {
                var keywords = enteredText.split(/(\s+)/);
                StreamTreeUtils.filterTrivialKeywords(keywords);
                streamSearchTerms = keywords;
                StreamTreeUtils.applyStreamSearch(keywords);
            }
        }

        /* Loading sign for the tree */
        BusyIndicator {
            id: loadingSign
            running: false
        }

        /* The actual tree */
        ListView {
            id: collectionsView

            anchors {
                left: parent.left
                right: parent.right
            }

            Layout.fillHeight: true

            clip: true

            model: collectionsModel
            delegate: ColumnLayout {
                visible: collectionMatchesQuery
                RowLayout {
                    visible: collectionMatchesQuery
                    Text {
                        id: arrow
                        text: opened ? "\u25bc" : "\u25b6"
                        font.pointSize: 12
                    }

                    Text {
                        id: label
                        text: name
                        font.pointSize: 12
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            opened = !opened;

                            if (streamsRequestPending || streamsLoaded) {
                                return;
                            }

                            streamsRequestPending = true;
                            btrdb.lookupStreams(name, false, {}, {}, function (collStreams) {
                                for (var i = 0; i !== collStreams.length; i++) {
                                    var modelObject = {
                                        "streamObject": collStreams[i],
                                        "streamMatchesQuery": true,
                                        "selected": false
                                    }

                                    modelObject.streamMatchesQuery = StreamTreeUtils.streamMatchesKeywords(modelObject, streamSearchTerms);

                                    streamList.append(modelObject);
                                }

                                streamsRequestPending = false;
                                streamsLoaded = true;
                            });
                        }
                    }
                }

                ListView {
                    id: collectionStreamsView

                    visible: collectionMatchesQuery && opened
                    width: 100
                    //Layout.preferredHeight: Math.min(contentItem.height, 300)
                    Layout.preferredHeight: contentItem.height

                    Layout.leftMargin: 20

                    model: streamList
                    delegate: CheckBox {
                        id: selectionBox
                        visible: streamMatchesQuery

                        checked: selected

                        text: streamObject.name

                        ToolTip {
                            delay: 1000
                            text: streamObject.toolTipText
                            visible: parent.hovered
                        }

                        onCheckedChanged: {
                            selected = selectionBox.checked;
                            if (selectionBox.checked) {
                                selectedStreamsChanged([streamObject], []);
                            } else {
                                selectedStreamsChanged([], [streamObject]);
                            }
                        }
                    }
                }
            }
        }

        ListModel {
            id: collectionsModel
        }
    }

    onBtrdbChanged: {
        if (btrdb === undefined || btrdb === null) {
            return;
        }

        connectingSign.running = false;
        mainColumn.visible = true;
        loadingSign.running = true;
        btrdb.listCollections("", function (collectionNames) {
            StreamTreeUtils.setCollectionsModel(collectionNames);
            StreamTreeUtils.applyCollectionSearch(collectionSearchTerms);
            loadingSign.running = false;
            loadingSign.visible = false;
        });
    }
}
