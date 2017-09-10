import QtQuick 2.0
import QtQuick.Dialogs 1.2
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import "mr-plotter-layouts" as MrPlotterLayouts

Rectangle {
    id: root
    property alias plotter: plot
    property var pendingScreenshot

    MrPlotterLayouts.StandardPlot {
        id: plot
        anchors.fill: parent

        scrollZoomable: true
        timeTickPromotion: true
        // timeZone is set by default to the local time zone
        dataDensityScrollZoomable: false
    }

    AxisButton {
        y: root.y + root.height - height
        buttonText: "\u2b73"
        buttonToolTip: "Save image of plot"
        onClicked: {
            plot.grabToImage(function (screenshot) {
                pendingScreenshot = screenshot;
                screenshotSaver.open();
            });
        }
    }

    FileDialog {
        id: screenshotSaver
        title: "Save screenshot"
        selectExisting: false
        visible: false
        onAccepted: {
            if (pendingScreenshot !== undefined) {
                var fileUrlString = fileUrl.toString();
                if (fileUrlString.startsWith("file://")) {
                    var filepath = fileUrlString.slice(7);
                    if (!filepath.endsWith(".png")) {
                        filepath = filepath + ".png";
                    }
                    pendingScreenshot.saveToFile(filepath);
                }
            }
            pendingScreenshot = undefined;
        }
        onRejected: {
            pendingScreenshot = undefined;
        }
    }
}
