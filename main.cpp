/*
 * This file is part of Mr. Plotter (Desktop/Mobile Application).

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

#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include <libbw.h>
#include <libmrplotter.h>

#include "btrdbstreamtreemodel.h"
#include "mrplotterutils.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    initLibMrPlotter();
    initLibBW();

    qmlRegisterType<BTrDBStreamTreeModel>("MrPlotterUI", 1, 0, "BTrDBStreamTreeModel");
    qmlRegisterType<BTrDBDataSource>("MrPlotterUI", 1, 0, "BTrDBDataSource");
    qmlRegisterSingletonType<MrPlotterUtils>("MrPlotterUI", 1, 0, "MrPlotterUtils", &MrPlotterUtils::qmlSingleton);

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

    return app.exec();
}
