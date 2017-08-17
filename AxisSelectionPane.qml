import QtQuick 2.8
import QtQuick.Controls 2.2
import QtQuick.Dialogs 1.2 as Dialogs
import QtQuick.Layouts 1.3

import MrPlotter 0.1
import MrPlotterUI 1.0
import "streamaxis.js" as AxisUtils

Rectangle {
    id: root

    property var plotter

    Component {
        id: plotterstream

        Stream {
        }
    }

    function addStreams(streams) {
        var plotterStreams = plotter.streamList;

        for (var i = 0; i !== streams.length; i++) {
            // Generate plotter objects
            var plotterObject = plotterstream.createObject(null);
            plotterObject.uuid = streams[i].details.UUID;
            plotterObject.dataSource = streams[i].dataSource;
            plotterObject.color = AxisUtils.nextDefaultColor();
            streams[i].plotterObject = plotterObject;

            var ddPlotterObject = plotterstream.createObject(null);
            ddPlotterObject.dataDensity = true;
            ddPlotterObject.uuid = streams[i].details.UUID;
            ddPlotterObject.dataSource = streams[i].dataSource;
            ddPlotterObject.color = Qt.binding(function () { return plotterObject.color; });
            ddPlotterObject.timeOffset = Qt.binding(function () { return plotterObject.timeOffset; });
            streams[i].ddPlotterObject = ddPlotterObject;

            // Decide which axis to add this stream
            var axisIndex = AxisUtils.axisForStream(streams[i], true, -1);
            var axis = axismodel.get(axisIndex);

            // Add the stream to that axis
            streams[i].axisindex = axisIndex;
            axis.streams.append(streams[i]);

            // Add it to the plotter axis object
            var axisStreamList = axis.plotterObject.streamList;
            axisStreamList.push(streams[i].plotterObject);
            axis.plotterObject.streamList = axisStreamList;

            // Add it to the main plotter object
            plotterStreams.push(streams[i].plotterObject);
        }

        plotter.streamList = plotterStreams;
    }

    function removeStreams(streams) {
        if (streams.length === 0) {
            return;
        }

        var streamIDs = {};
        for (var k = 0; k !== streams.length; k++) {
            var streamID = streams[k].id;
            streamIDs[streamID] = 0;
        }

        var plotterObjects = {};
        for (var i = 0; i !== axismodel.count; i++) {
            var axisStreams = axismodel.get(i).streams;
            for (var j = axisStreams.count - 1; j !== -1; j--) {
                var stream = axisStreams.get(j);

                if (stream.id in streamIDs) {
                    plotterObjects[stream.plotterObject.toString()] = 0;

                    if (stream.plotterObject.selected) {
                        AxisUtils.removeStreamFromDataDensityPlot(stream.ddPlotterObject);
                    }

                    axisStreams.remove(j, 1);
                }
            }
        }

        var plotterStreams = plotter.streamList;
        for (var a = plotterStreams.length - 1; a !== -1; a--) {
            if (plotterStreams[a].toString() in plotterObjects) {
                plotterStreams.splice(a, 1);
            }
        }
        plotter.streamList = plotterStreams;
    }

    Dialog {
        id: plotSettingsDialog
        title: "Settings"
        standardButtons: Dialog.Ok

        width: 300

        ColumnLayout {
            anchors.fill: parent

            Text {
                text: "Timezone"
            }

            ComboBox {
                implicitWidth: 250
                editable: true
                model: MrPlotterUtils.validTimezonesList
                onAccepted: {
                    if (find(editText) === -1) {
                        editText = plotter.timeZone;
                        return;
                    }

                    plotter.timeZone = editText;
                }
                onActivated: function (index) {
                    plotter.timeZone = model[index];
                }

                Component.onCompleted: {
                    var i = find(plotter.timeZone);
                    currentIndex = i;
                }
            }

            CheckBox {
                text: "Promote time ticks"
                checked: plotter.timeTickPromotion
                onCheckedChanged: plotter.timeTickPromotion = checked;

                ToolTip {
                    delay: 1000
                    text: "E.g., display '2017' instead of 'January'"
                    visible: parent.hovered
                }
            }
        }
    }

    YAxis {
        id: ddAxis
        dynamicAutoscale: true
        name: "Count"
        domain: [0, 2]
        minTicks: 2
    }

    Component {
        id: streamrepresentation

        RowLayout {
            id: streaminfo

            anchors.left: parent.left
            anchors.right: parent.right

            Button {
                text: "\u270d" // Unicode drawing hand symbol
                onClicked: colorpicker.visible = true;
                background: Rectangle {
                    id: colorbuttonbackground
                    implicitWidth: contentItem.implicitWidth
                    implicitHeight: contentItem.implicitHeight
                    color: colorpicker.color
                    onColorChanged: {
                        plotterObject.color = color;
                    }
                }
                ToolTip {
                    delay: 1000
                    text: "Choose a Color"
                    visible: parent.hovered
                }
            }

            Dialogs.ColorDialog {
                id: colorpicker
                title: "Choose a Color"
                color: plotterObject.color // initially
                showAlphaChannel: false
                visible: false
            }

            Text {
                id: streampath
                Layout.fillWidth: true
                Layout.minimumWidth: 50
                text: labelText
                wrapMode: Text.WrapAnywhere

                MouseArea {
                    id: streammousearea
                    anchors.fill: parent

                    cursorShape: Qt.PointingHandCursor

                    // If performance is an issue, remove this
                    hoverEnabled: true
                    onClicked: {
                        plotterObject.selected = !plotterObject.selected;

                        if (plotterObject.selected) {
                            AxisUtils.addStreamToDataDensityPlot(ddPlotterObject);
                        } else {
                            AxisUtils.removeStreamFromDataDensityPlot(ddPlotterObject);
                        }
                    }
                }

                ToolTip {
                    delay: 1000
                    text: toolTipText
                    visible: streammousearea.containsMouse
                }

                Rectangle {
                    id: colorbox
                    anchors.fill: parent
                    color: {
                        if (plotterObject.selected) {
                            var c1 = colorpicker.color;
                            return Qt.rgba(c1.r, c1.g, c1.b, 0.5);
                        } else if (streammousearea.containsMouse) {
                            var c2 = colorpicker.color;
                            return Qt.rgba(c2.r, c2.g, c2.b, 0.3);
                        } else {
                            return "transparent";
                        }
                    }
                }
            }

            Button {
                text: "\u2699" // Unicode gear symbol
                background.implicitWidth: contentItem.implicitWidth
                background.implicitHeight: contentItem.implicitHeight
                ToolTip {
                    delay: 1000
                    text: "Stream settings"
                    visible: parent.hovered
                }
                onClicked: {
                    //var settingsDialog = streamsettings.createObject(root, {"stream": axismodel.get(axisindex).streams.get(index), "index": index, "visible": true});
                    //settingsDialog.parent = root;
                    settingsDialog.visible = true;
                    settingsDialog.parent = root; // so it appears in the middle
                }
            }

           Dialog {
                id: settingsDialog

                title: "Stream settings"
                standardButtons: Dialog.Ok

                ColumnLayout {
                    ComboBox {
                        model: axismodel
                        textRole: "axisname"
                        currentIndex: axisindex

                        onActivated: function (newaxisindex) {
                            AxisUtils.moveStreamToAxis(axisindex, index, newaxisindex);
                        }
                    }

                    CheckBox {
                        text: "Always connect points"
                        checked: plotterObject.alwaysConnect
                        onCheckedChanged: plotterObject.alwaysConnect = checked;
                    }

                    CheckBox {
                        text: "Highlight data density plot"
                        checked: ddPlotterObject.selected
                        onCheckedChanged: ddPlotterObject.selected = checked;
                    }
                }
            }

            // According to Valgrind, this causes memory errors?
            /*ComboBox {
                id: axischoice
                model: axismodel
                textRole: "axisname"
                currentIndex: axisindex

                onActivated: function (newaxisindex) {
                    AxisUtils.moveStreamToAxis(axisindex, index, newaxisindex);
                }
            }*/
        }
    }

    Component {
        id: axisrepresentation

        Rectangle {
            id: axisholder
            width: axislist.width
            height: content.implicitHeight

            border.width: 2

            RowLayout {
                id: content
                width: parent.width

                TextField {
                    id: name
                    Layout.margins: 3
                    background.implicitWidth: axisholder.width / 6
                    placeholderText: "Name"

                    text: axisname
                    onEditingFinished: {
                        axisname = name.text;
                        plotterObject.name = axisname;
                    }
                }

                ListView {
                    id: streamlist

                    Layout.preferredHeight: Math.min(contentItem.height, 300)
                    Layout.fillWidth: true

                    clip: true

                    Layout.margins: 3

                    model: streams
                    delegate: streamrepresentation
                }

                ColumnLayout {
                    Layout.fillWidth: false
                    RowLayout {
                        id: axisbuttons

                        AxisButton {
                            id: autoscalebutton
                            buttonText: "\u21d5" // Unicode thick up/down arrow icon
                            buttonToolTip: "Autoscale to show all data"
                            highlighted: plotterObject.dynamicAutoscale
                            onClicked: {
                                plotterObject.dynamicAutoscale = false;
                                plotterObject.autoscale(plotter.timeDomain);
                            }
                            onPressAndHold: {
                                plotterObject.dynamicAutoscale = true;
                                plotterObject.autoscale(plotter.timeDomain);
                            }
                        }

                        RowLayout {
                            Layout.margins: 2
                            spacing: 0

                            ButtonGroup {
                                buttons: [leftbutton, hidebutton, rightbutton]
                            }

                            AxisButton {
                                id: leftbutton
                                buttonText: "\u2190" // Unicode <- icon
                                buttonToolTip: "Display left of plot"
                                checkable: true

                                // Initially, display axes to the right of plot
                                checked: true

                                onClicked: {
                                    if (!hidden && rightSide) {
                                        AxisUtils.removeAxisFromPlot(plotterObject, true);
                                    }
                                    if (hidden || rightSide) {
                                        AxisUtils.addAxisToPlot(plotterObject, false);
                                    }
                                    hidden = false;
                                    rightSide = false;
                                }
                            }

                            AxisButton {
                                id: hidebutton
                                buttonText: "\u2205" // Unicode null set icon
                                buttonToolTip: "Do not display axis"
                                checkable: true

                                onClicked: {
                                    if (!hidden) {
                                        AxisUtils.removeAxisFromPlot(plotterObject, rightSide);
                                    }
                                    hidden = true;
                                }
                            }

                            AxisButton {
                                id: rightbutton
                                buttonText: "\u2192" // Unicode -> icon
                                buttonToolTip: "Display right of plot"
                                checkable: true

                                onClicked: {
                                    if (!hidden && !rightSide) {
                                        AxisUtils.removeAxisFromPlot(plotterObject, false);
                                    }
                                    if (hidden || !rightSide) {
                                        AxisUtils.addAxisToPlot(plotterObject, true);
                                    }
                                    hidden = false;
                                    rightSide = true;
                                }
                            }
                        }

                        AxisButton {
                            id: removebutton
                            buttonText: "\u2717" // Unicode X icon
                            buttonToolTip: "Remove axis"
                            Layout.margins: 2

                            onClicked: {
                                if (axismodel.count === 1) {
                                    return;
                                }

                                var streamPlotterObjects = plotterObject.streamList;
                                plotterObject.streamList = [];

                                for (var i = 0; i !== streams.count; i++) {
                                    var stream = streams.get(i);
                                    var axisIndex = AxisUtils.axisForStream(stream, false, index);
                                    stream.axisindex = axisIndex;

                                    // Add to the axis model
                                    var axis = axismodel.get(axisIndex);
                                    axis.streams.append(stream);

                                    // Add to the axis plotter object
                                    var axisStreams = axis.plotterObject.streamList;
                                    axisStreams.push(streamPlotterObjects[i]);
                                    axis.streamList = axisStreams;
                                }

                                if (!hidden) {
                                    AxisUtils.removeAxisFromPlot(plotterObject, rightSide)
                                }
                                var oldindex = index;
                                axismodel.remove(index, 1);


                                /*
                                 * The axis indices have changed, so we need to update
                                 * the axisindex of all streams on all subsequent axes.
                                 */
                                for (var j = oldindex; j !== axismodel.count; j++) {
                                    var streamsToFix = axismodel.get(j).streams;
                                    for (var k = 0; k !== streamsToFix.count; k++) {
                                        streamsToFix.get(k).axisindex = j;
                                    }
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.margins: 2

                        Layout.maximumWidth: axisbuttons.width

                        Text {
                            text: "Scale: "
                        }

                        TextField {
                            id: domainLo
                            placeholderText: "Min"

                            Layout.fillWidth: true

                            text: plotterObject.domainLo.toString();
                            onEditingFinished: {
                                var textNum = parseFloat(text);
                                if (!isNaN(textNum)) {
                                    plotterObject.domainLo = textNum;
                                }
                                text = plotterObject.domainLo.toString();
                            }
                        }

                        Text {
                            text: "to"
                        }

                        TextField {
                            id: domainHi
                            placeholderText: "Max"

                            Layout.fillWidth: true

                            text: plotterObject.domainHi.toString();
                            onEditingFinished: {
                                var textNum = parseFloat(text);
                                if (!isNaN(textNum)) {
                                    plotterObject.domainHi = textNum;
                                }
                                text = plotterObject.domainHi.toString();
                            }
                        }
                    }
                }
            }
        }
    }

    ListModel {
        id: axismodel
        dynamicRoles: true
    }

    /*Component.onCompleted: {
        var stream1 = {
            plotterObject: { UUID: "fake-uuid" },
            label: "test: stream-name",
            axisindex: 0
        };
        axismodel.append({
                             axisid: "y1",
                             plotterObject: {
                                 name: "Axis 1",
                                 streamList: [stream1.plotterObject],
                                 selected: false
                             },
                             axisname: "Axis 1",
                             streams: [stream1]
                         });
        axismodel.append({
                             axisid: "y2",
                             plotterObject: { name: "Another axis" },
                             axisname: "Another axis",
                             streams: []
                         });
    }*/

    Component {
        id: plotteraxis

        YAxis {
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 5

        RowLayout {
            id: titlerow
            spacing: 5

            Text {
                id: sectiontitle
                Layout.fillWidth: true
                text: "Assign Streams to Axes"
                font.pointSize: 20
            }

            AxisButton {
                buttonText: "\u2699" // Unicode gear icon
                buttonToolTip: "Settings"
                onClicked: plotSettingsDialog.visible = true;
            }

            AxisButton {
                buttonText: "\u21d4" // Unicode thick left/right arrow icon
                buttonToolTip: "Autozoom to show all data"
                onClicked: plotter.autozoom();
            }

            AxisButton {
                id: newAxisButton
                buttonText: "New Axis"
                buttonToolTip: "Create a new axis"
                property int id: 0;
                onClicked: {
                    var plotterObj = plotteraxis.createObject(newAxisButton);
                    plotterObj.name = "y" + id++;
                    plotterObj.streamList = [];
                    var axis = {
                        plotterObject: plotterObj,
                        axisname: plotterObj.name,
                        streams: [],
                        hidden: false,
                        rightSide: false
                    };

                    var leftAxisList = plotter.leftAxisList;
                    leftAxisList.push(plotterObj);
                    plotter.leftAxisList = leftAxisList;

                    axismodel.append(axis);
                }
            }
        }

        ListView {
            id: axislist
            anchors.left: parent.left
            anchors.right: parent.right

            Layout.fillHeight: true

            clip: true

            spacing: 5

            model: axismodel
            delegate: axisrepresentation
        }
    }
}
