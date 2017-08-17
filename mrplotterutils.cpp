#include "mrplotterutils.h"
#include <QObject>
#include <QTimeZone>

MrPlotterUtils::MrPlotterUtils(QObject* parent) : QObject(parent)
{
    /* Construct properties. */
    QList<QByteArray> available = QTimeZone::availableTimeZoneIds();
    this->timezones.reserve(available.count());
    for (const QByteArray& tzName : available) {
        this->timezones.push_back(tzName);
    }
}

QObject* MrPlotterUtils::qmlSingleton(QQmlEngine* qmlEngine, QJSEngine* jsEngine) {
    Q_UNUSED(qmlEngine);
    Q_UNUSED(jsEngine);

    static MrPlotterUtils* singleton = new MrPlotterUtils(nullptr);
    return singleton;
}
