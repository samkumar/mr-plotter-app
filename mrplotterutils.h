#ifndef MRPLOTTERUTILS_H
#define MRPLOTTERUTILS_H

#include <QObject>
#include <QQmlEngine>
#include <QJSEngine>

class MrPlotterUtils : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList validTimezonesList MEMBER timezones NOTIFY validTimezonesListChanged)
public:
    MrPlotterUtils(QObject* parent = nullptr);

    static QObject* qmlSingleton(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

signals:
    void validTimezonesListChanged();

private:
    QStringList timezones;
};

#endif // MRPLOTTERUTILS_H
