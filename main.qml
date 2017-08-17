/*
 * This file is part of Mr. Plotter (Desktop/Mobile Application).
 *
 * Mr. Plotter is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mr. Plotter is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Mr. Plotter.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQml.Models 2.2

import QtQuick 2.6
import QtQuick.Controls 1.4 as QtQuickControls1
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.3
import QtQuick.Window 2.2

import MrPlotter 0.1
import "mr-plotter-layouts" as MrPlotterLayouts

Window {
    id: toplevel
    visible: true
    width: 800
    height: 600
    title: qsTr("Mr. Plotter")

    QtQuickControls1.SplitView {
        anchors.fill: parent
        orientation: Qt.Vertical

        /* This is where the actual plot will go. */
        PlotPane {
            id: plotpane
            height: parent.height / 2

            /* Ensures that the bottom half is always in the window. */
            Layout.maximumHeight: parent.height
        }

        QtQuickControls1.SplitView {
            anchors.left: parent.left
            anchors.right: parent.right

            orientation: Qt.Horizontal

            /* This will have the menus to select archivers/streams. */
            StreamSelectionPane {
                id: selectionpane
                width: parent.width / 3

                /* Ensure that settings pane remains in window. */
                Layout.maximumWidth: parent.width

                onSelectedStreamsChanged: function (selected, deselected) {
                    settingspane.addStreams(selected);
                    settingspane.removeStreams(deselected);
                }
            }

            /* This will have the rest of the settings. */
            AxisSelectionPane {
                id: settingspane
                plotter: plotpane.plotter
            }
        }
    }
}
