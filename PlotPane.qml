import QtQuick 2.0
import QtQuick.Layouts 1.3
import "mr-plotter-layouts" as MrPlotterLayouts

Rectangle {
    id: root
    property alias plotter: plot

    MrPlotterLayouts.StandardPlot {
        id: plot
        anchors.fill: parent

        scrollZoomable: true
        timeTickPromotion: true
        // timeZone is set by default to the local time zone
        dataDensityScrollZoomable: false
    }
}
