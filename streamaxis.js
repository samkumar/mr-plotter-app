function idFromStreamPlotterObject(stream) {
    /*
     * The string produced by toString() contains the memory
     * address of the data source, so this works.
     */
    return stream.dataSource.toString() + ";" + stream.uuid;
}

function addAxisToPlot(axis, rightside) {
    var axisList;
    if (rightside) {
        axisList = plotter.rightAxisList;
    } else {
        axisList = plotter.leftAxisList;
    }

    axisList.push(axis);

    if (rightside) {
        plotter.rightAxisList = axisList;
    } else {
        plotter.leftAxisList = axisList;
    }
}

function removeAxisFromPlot(axis, rightside) {
    var axisList;
    if (rightside) {
        axisList = plotter.rightAxisList;
    } else {
        axisList = plotter.leftAxisList;
    }

    var found = false;
    for (var i = 0; i !== axisList.length; i++) {
        if (axis === axisList[i]) {
            axisList.splice(i, 1);
            found = true;
            break;
        }
    }

    if (rightside) {
        plotter.rightAxisList = axisList;
    } else {
        plotter.leftAxisList = axisList;
    }
}

function axisForStream(stream, createNewIfNoMatch, skipAxis) {
    var streamUnit = stream["unit"];
    for (var i = 0; i !== axismodel.count; i++) {
        if (i === skipAxis) {
            continue;
        }

        var axisStreams = axismodel.get(i).streams;

        if (axisStreams.count === 0) {
            // This axis has no streams assigned to it
            return i;
        }

        for (var j = 0; j !== axisStreams.count; j++) {
            if (axisStreams.get(j)["unit"] === streamUnit) {
                // This axis has a stream with the same units as the new stream
                return i;
            }
        }
    }
    if (createNewIfNoMatch) {
        newAxisButton.onClicked();
        return axismodel.count - 1;
    }
    return 0;
}

function addStreamToDataDensityPlot(ddPlotterObject) {
    var ddAxisStreamList = ddAxis.streamList;
    ddAxisStreamList.push(ddPlotterObject);
    ddAxis.streamList = ddAxisStreamList;

    var ddStreams = plotter.dataDensityStreamList;
    ddStreams.push(ddPlotterObject);
    plotter.dataDensityStreamList = ddStreams;

    plotter.leftDataDensityAxisList = [ ddAxis ];
}

function removeStreamFromDataDensityPlot(ddPlotterObject) {
    var ddAxisStreamList = ddAxis.streamList;
    var ddStreams = plotter.dataDensityStreamList;

    var i;
    for (i = 0; i !== ddAxisStreamList.length; i++) {
        if (ddAxisStreamList[i] === ddPlotterObject) {
            ddAxisStreamList.splice(i, 1);
            ddStreams.splice(i, 1);
            break;
        }
    }

    ddAxis.streamList = ddAxisStreamList;
    plotter.dataDensityStreamList = ddStreams;

    if (ddStreams.length === 0) {
        plotter.leftDataDensityAxisList = [];
    }
}

function moveStreamToAxis(fromaxisindex, streamindex, newaxisindex) {
    if (fromaxisindex === newaxisindex) {
        return;
    }

    var currentAxis = axismodel.get(fromaxisindex);
    var newAxis = axismodel.get(newaxisindex);
    var stream = currentAxis.streams.get(streamindex);
    stream.axisindex = newaxisindex;

    var streamPlotterObject = stream.plotterObject;

    // The append has to happen before removing from the old axis
    newAxis.streams.append(stream);
    currentAxis.streams.remove(streamindex);

    // Now we need to move the stream in the actual plotter
    var currentStreams = currentAxis.plotterObject.streamList;
    for (var i = 0; i !== currentStreams.length; i++)
    {
        if (streamPlotterObject === currentStreams[i])
        {
            currentStreams.splice(i, 1);
            break;
        }
    }
    currentAxis.plotterObject.streamList = currentStreams;

    var newStreams = newAxis.plotterObject.streamList;
    newStreams.push(streamPlotterObject);
    newAxis.plotterObject.streamList = newStreams;
    return newAxis.streams.get(newAxis.streams.count - 1);
}

var defaultColors = ["blue", "red", "lime", "aqua", "fuchsia", "yellow", "navy", "maroon", "green", "teal", "purple", "olive"];
var defaultColorIndex = 0;
function nextDefaultColor() {
    var chosen = defaultColors[defaultColorIndex];
    defaultColorIndex = (defaultColorIndex + 1) % defaultColors.length;
    return chosen;
}
