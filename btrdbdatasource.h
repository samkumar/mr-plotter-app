#ifndef BTRDBDATASOURCE_H
#define BTRDBDATASOURCE_H

#include <memory>
#include <btrdb/btrdb.h>
#include <QJSEngine>
#include <QObject>
#include <QString>
#include <QUuid>

#include "datasource.h"

template <typename First>
void convertToList(QJSEngine* e, QJSValueList* l, First first) {
    l->append(e->toScriptValue(first));
}

template <typename First, typename Second, typename... Rest>
void convertToList(QJSEngine* e, QJSValueList* l, First first, Second second, Rest... rest...) {
    convertToList(e, l, first);
    convertToList(e, l, second, rest...);
}

class BTrDBDataSource : public DataSource
{
    Q_OBJECT
public:
    explicit BTrDBDataSource(QObject *parent = 0);
    Q_INVOKABLE QString getHostPort() const;
    Q_INVOKABLE bool connect(QString addrport);
    void connectAsync(QString addrport, std::function<void(bool)> on_done);
    Q_INVOKABLE void connectAsync(QString addrport, QJSValue callback);
    bool listCollections(QString prefix, std::function<void(QStringList)> on_done);
    Q_INVOKABLE bool listCollections(QString prefix, QJSValue callback);
    bool lookupStreams(QString collection, bool is_prefix, QMap<QString, QPair<QString, bool>> tags, QMap<QString, QPair<QString, bool>> annotations, std::function<void(QVector<btrdb::Stream*>)> on_done);
    Q_INVOKABLE bool lookupStreams(QString collection, bool is_prefix, QVariantMap tags, QVariantMap annotations, QJSValue callback);

    std::shared_ptr<btrdb::BTrDB> getBTrDB();

    void alignedWindows(const QUuid& uuid, int64_t start, int64_t end, uint8_t pwe, ReqCallback callback) override;
    void brackets(const QList<QUuid> uuids, BracketCallback callback) override;
    void changedRanges(const QUuid& uuid, uint64_t fromGen, uint64_t toGen, uint8_t pwe, ChangedRangesCallback callback) override;

signals:

public slots:

private:
    quint64 store_callback(QJSValue js_callback);

    template <typename... Args>
    void invoke_callback(quint64 key, bool remove, Args... args...) {
        QJSValueList l;
        QJSEngine* jsEngine = qjsEngine(this);
        convertToList(jsEngine, &l, args...);
        Q_ASSERT(this->js_callbacks.contains(key));
        QJSValue& callback = this->js_callbacks[key];
        callback.call(l);
        if (remove) {
            this->js_callbacks.remove(key);
        }
    }

    QString hostport;

    QHash<quint64, QJSValue> js_callbacks;
    quint64 current_js_callback_key;

    std::shared_ptr<btrdb::BTrDB> btrdb;
    bool connected;
};

#endif // BTRDBDATASOURCE_H
