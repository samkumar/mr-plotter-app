#include "btrdbdatasource.h"

#include <functional>
#include <memory>
#include <vector>
#include <utility>

#include <QJSEngine>
#include <QJsonDocument>
#include <QList>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QThread>

#include <btrdb/btrdb.h>
#include <grpc++/grpc++.h>

#include "requester.h"


/* Mostly copied from Michael's code in qtlibbw/utils.h. */
template<typename... Tz>
void invokeOnThread(QThread* t, std::function<void (Tz...)> f, Tz... args)
{
    QTimer* timer = new QTimer();
    timer->moveToThread(t);
    timer->setSingleShot(true);
    QObject::connect(timer, &QTimer::timeout, [=]() mutable {
        f(args...);
        timer->deleteLater();
    });
    QMetaObject::invokeMethod(timer, "start", Qt::QueuedConnection, Q_ARG(int, 0));
}

template <typename... CbArgs>
inline std::function<void(CbArgs...)> wrap_callback(QObject* to, std::function<void(CbArgs...)> callback) {
    return [=](CbArgs... args) {
        invokeOnThread<CbArgs...>(to->thread(), callback, args...);
    };
}

BTrDBDataSource::BTrDBDataSource(QObject *parent)
    : DataSource(parent), current_js_callback_key(0), connected(false)
{
}

quint64 BTrDBDataSource::store_callback(QJSValue js_callback) {
    quint64 key = this->current_js_callback_key++;
    this->js_callbacks[key] = std::move(js_callback);
    return key;
}

QString BTrDBDataSource::getHostPort() const {
    return this->hostport;
}

bool BTrDBDataSource::connect(QString addrport) {
    this->hostport = addrport;

    std::vector<std::string> endpoints;
    endpoints.push_back(addrport.toStdString());
    this->btrdb = btrdb::BTrDB::connect(btrdb::default_ctx, endpoints);
    return this->btrdb.get() != nullptr;
}

void BTrDBDataSource::connectAsync(QString addrport, std::function<void(bool)> on_done) {
    std::vector<std::string> endpoints;
    endpoints.push_back(addrport.toStdString());
    btrdb::BTrDB::connectAsync(btrdb::default_ctx, endpoints, [=](std::shared_ptr<btrdb::BTrDB> b) {
        bool success = b.get() != nullptr;
        if (success) {
            this->hostport = addrport;
            this->btrdb = b;
        }
        wrap_callback(this, on_done)(success);
    });
}

void BTrDBDataSource::connectAsync(QString addrport, QJSValue callback) {
    quint64 current_key = store_callback(std::move(callback));
    std::function<void(bool)> wrapped = [=](bool success) {
        this->invoke_callback(current_key, true, success);
    };
    this->connectAsync(addrport, wrapped);
}

bool BTrDBDataSource::listCollections(QString prefix, std::function<void(QStringList)> on_done) {
    QStringList* collection_list = new QStringList;
    auto callback = std::function<void(bool, btrdb::Status, const std::vector<std::string>&)>([=](bool finished, btrdb::Status status, const std::vector<std::string>& collections) {
        Q_UNUSED(status);
        for (const std::string& collection : collections) {
            collection_list->push_back(QString::fromStdString(collection));
        }
        if (finished) {
            on_done(*collection_list);
            delete collection_list;
        }
    });
    btrdb::Status status = this->btrdb->listCollectionsAsync(btrdb::default_ctx, wrap_callback(this, callback), prefix.toStdString());
    return !status.isError();
}

bool BTrDBDataSource::listCollections(QString prefix, QJSValue callback) {
    quint64 current_key = store_callback(std::move(callback));
    std::function<void(QStringList)> wrapped = [=](QStringList collections) {
        this->invoke_callback(current_key, true, collections, false);
    };

    return this->listCollections(prefix, wrapped);
}

bool BTrDBDataSource::lookupStreams(QString collection, bool is_prefix, QMap<QString, QPair<QString, bool>> tags, QMap<QString, QPair<QString, bool>> annotations, std::function<void(QVector<btrdb::Stream*>)> on_done) {
    QVector<btrdb::Stream*>* stream_list = new QVector<btrdb::Stream*>;
    std::function<void(bool, btrdb::Status, std::vector<btrdb::Stream*>&)> callback = [=](bool finished, btrdb::Status status, std::vector<btrdb::Stream*>& streams) {
        Q_UNUSED(status);
        for (btrdb::Stream* stream : streams) {
            stream_list->append(stream);
        }
        if (finished) {
            on_done(std::move(*stream_list));
            delete stream_list;
        }
    };

    std::map<std::string, std::pair<std::string, bool>> std_tags;
    for (auto i = tags.begin(); i != tags.end(); i++) {
        std_tags[i.key().toStdString()] = std::make_pair(i.value().first.toStdString(), i.value().second);
    }
    std::map<std::string, std::pair<std::string, bool>> std_annotations;
    for (auto i = annotations.begin(); i != annotations.end(); i++) {
        std_annotations[i.key().toStdString()] = std::make_pair(i.value().first.toStdString(), i.value().second);
    }

    /* Avoid unique_ptr issues with the STREAMS argument using additional wrapping. */
    auto wrapped = wrap_callback(this, callback);
    std::function<void(bool, btrdb::Status, std::vector<std::unique_ptr<btrdb::Stream>>&)> compatible_callback = [=](bool finished, btrdb::Status status, std::vector<std::unique_ptr<btrdb::Stream>>& streams) {
        std::vector<btrdb::Stream*> plain_streams(streams.size());
        for (std::vector<btrdb::Stream*>::size_type i = 0; i != plain_streams.size(); i++) {
            plain_streams[i] = streams[i].release();
        }
        wrapped(finished, status, plain_streams);
    };

    btrdb::Status status = this->btrdb->lookupStreamsAsync(btrdb::default_ctx, compatible_callback, collection.toStdString(), is_prefix, std_tags, std_annotations);
    return !status.isError();
}

QMap<QString, QPair<QString, bool>> optionalMapFromVariantMap(QVariantMap variantMap) {
    QMap<QString, QPair<QString, bool>> optionalMap;
    for (auto it = variantMap.begin(); it != variantMap.end(); it++) {
        QVariant value = it.value();
        bool is_string = value.canConvert(QMetaType::QString);
        optionalMap[it.key()] = QPair<QString, bool>(value.toString(), is_string);
    }
    return optionalMap;
}

QVariantMap toVariantMap(const std::map<std::string, std::string>& string_map)
{
    QVariantMap variant_map;
    for (auto it = string_map.begin(); it != string_map.end(); it++)
    {
        variant_map[QString::fromStdString(it->first)] = QString::fromStdString(it->second);
    }
    return variant_map;
}

bool BTrDBDataSource::lookupStreams(QString collection, bool is_prefix, QVariantMap tags, QVariantMap annotations, QJSValue callback) {
    QMap<QString, QPair<QString, bool>> tagsMap = optionalMapFromVariantMap(tags);
    QMap<QString, QPair<QString, bool>> annotationsMap = optionalMapFromVariantMap(annotations);

    quint64 current_key = store_callback(callback);
    std::function<void(QVector<btrdb::Stream*>)> wrapped = [=](QVector<btrdb::Stream*> streams) {
        QVariantList streamObjects;
        for (int i = 0; i != streams.count(); i++) {
            btrdb::Stream* stream = streams[i];
            QVariantMap streamObject;

            /* Extract information about the stream. */
            const void* uuid_bytes = stream->UUID();
            QByteArray uuid_byte_array = QByteArray::fromRawData(reinterpret_cast<const char*>(uuid_bytes), 16);
            QUuid uuidObject = QUuid::fromRfc4122(uuid_byte_array);
            QString uuid_string_full = uuidObject.toString();
            QString uuid = uuid_string_full.mid(1, uuid_string_full.length() - 2);

            const std::string* collection_string;
            stream->collection(nullptr, &collection_string);
            QString collection = QString::fromStdString(*collection_string);

            const std::map<std::string, std::string>* tags_map;
            stream->tags(nullptr, &tags_map);
            QVariantMap tags = toVariantMap(*tags_map);

            const std::map<std::string, std::string>* annotations_map;
            std::uint64_t annotations_ver;
            stream->cachedAnnotations(nullptr, &annotations_map, &annotations_ver);
            QVariantMap annotations = toVariantMap(*annotations_map);

            /* Fill in details. */
            QVariantMap details;
            details["UUID"] = uuid;
            details["collection"] = collection;
            details["tags"] = tags;
            details["annotations"] = annotations;

            /* Deduce the stream name. */
            QString name;
            if (annotations.contains("name")) {
                name = annotations["name"].toString();
            } else if (tags.contains("name")) {
                name = tags["name"].toString();
            } else {
                name = uuid;
            }

            /* Generate top-level stream information. */
            QString sourceString = QStringLiteral("BTrDB@%1").arg(this->getHostPort());
            streamObject["details"] = details;
            streamObject["name"] = name;
            streamObject["labelText"] = QStringLiteral("%1: %2").arg(collection).arg(name);
            streamObject["dataSource"] = QVariant::fromValue<QObject*>(this);
            streamObject["sourceText"] = sourceString;
            streamObject["id"] = QStringLiteral("BTrDB#asdf%1:%2").arg(this->uniqueID).arg(uuid);

            /* Generate a ToolTip. */
            QString toolTipTemplate = QStringLiteral("Source: %1\nUUID: %2\nCollection: %3\nTags: %4\nAnnotations: %5");
            QJsonDocument tagsDoc = QJsonDocument::fromVariant(tags);
            QJsonDocument annotationsDoc = QJsonDocument::fromVariant(annotations);
            streamObject["toolTipText"] = toolTipTemplate.arg(sourceString).arg(uuid).arg(collection).arg(QString(tagsDoc.toJson().trimmed())).arg(QString(annotationsDoc.toJson().trimmed()));

            /* Deduce the units. */
            if (annotations.contains("unit")) {
                streamObject["unit"] = annotations["unit"].toString();
            } else if (tags.contains("unit")) {
                streamObject["unit"] = tags["unit"].toString();
            } else {
                streamObject["unit"] = QStringLiteral("");
            }

            streamObjects.push_back(streamObject);
            delete stream;
        }
        this->invoke_callback(current_key, true, streamObjects);
    };

    return this->lookupStreams(collection, is_prefix, tagsMap, annotationsMap, wrapped);
}

std::shared_ptr<btrdb::BTrDB> BTrDBDataSource::getBTrDB() {
    return this->btrdb;
}

class RequestData {
public:
    RequestData(std::unique_ptr<btrdb::Stream> s) : stream(std::move(s)), version(0) {}
    std::unique_ptr<btrdb::Stream> stream;
    std::uint64_t version;
};

void BTrDBDataSource::alignedWindows(const QUuid& uuid, int64_t start, int64_t end, uint8_t pwe, ReqCallback callback)
{
    QThread* t = QThread::currentThread();

    QByteArray uuidArr = uuid.toRfc4122();
    std::unique_ptr<btrdb::Stream> s = this->btrdb->streamFromUUID(uuidArr.constData());

    QVector<struct statpt>* streamdata = new QVector<struct statpt>();
    RequestData* request_data = new RequestData(std::move(s));
    btrdb::Status status = request_data->stream->alignedWindowsAsync(btrdb::default_ctx, [=](bool finished, btrdb::Status stat, std::vector<struct btrdb::StatisticalPoint>& data, std::uint64_t version)
    {
        Q_UNUSED(stat);
        if (data.size() != 0)
        {
            for (struct btrdb::StatisticalPoint& point : data)
            {
                struct statpt newpoint;
                newpoint.time = point.time;
                newpoint.min = point.min;
                newpoint.mean = point.mean;
                newpoint.max = point.max;
                newpoint.count = point.count;
                streamdata->push_back(newpoint);
            }
            request_data->version = version;
        }

        if (finished)
        {
            std::uint64_t saved_version = request_data->version;
            invokeOnThread(t, std::function<void()>([=]()
            {
                callback(streamdata->data(), streamdata->count(), saved_version);
                delete streamdata;
            }));
            delete request_data;
        }
    }, start, end, pwe);

    Q_UNUSED(status);
}

struct bracketinfo
{
    std::vector<std::unique_ptr<btrdb::Stream>> streams;
    QHash<QUuid, struct brackets> overall_brackets;
    int requests_pending;
};

void BTrDBDataSource::brackets(const QList<QUuid> uuids, BracketCallback callback)
{
    QThread* t = QThread::currentThread();
    struct bracketinfo* reqinfo = new struct bracketinfo;
    reqinfo->requests_pending = uuids.count() << 1;
    reqinfo->streams.reserve(reqinfo->requests_pending);
    for (const QUuid& uuid : uuids)
    {
        QByteArray uuidArr = uuid.toRfc4122();
        reqinfo->streams.push_back(this->btrdb->streamFromUUID(uuidArr.constData()));
        std::unique_ptr<btrdb::Stream>& s = reqinfo->streams.back();

        s->nearestAsync(btrdb::default_ctx, [=](btrdb::Status stat, const btrdb::RawPoint& point, std::uint64_t version)
        {
            Q_UNUSED(version);
            struct brackets& bracks = reqinfo->overall_brackets[uuid];
            bracks.lowerbound = stat.isError() ? btrdb::BTrDB::MAX_TIME : point.time;
            reqinfo->requests_pending--;
            if (reqinfo->requests_pending == 0)
            {
                invokeOnThread(t, callback, reqinfo->overall_brackets);
                delete reqinfo;
            }
        }, btrdb::BTrDB::MIN_TIME, false);

        s->nearestAsync(btrdb::default_ctx, [=](btrdb::Status stat, const btrdb::RawPoint& point, std::uint64_t version)
        {
            Q_UNUSED(version);
            struct brackets& bracks = reqinfo->overall_brackets[uuid];
            bracks.upperbound = stat.isError() ? btrdb::BTrDB::MIN_TIME : point.time;
            reqinfo->requests_pending--;
            if (reqinfo->requests_pending == 0)
            {
                invokeOnThread(t, callback, reqinfo->overall_brackets);
                delete reqinfo;
            }
        }, btrdb::BTrDB::MAX_TIME, true);
    }
}

void BTrDBDataSource::changedRanges(const QUuid& uuid, uint64_t fromGen, uint64_t toGen, uint8_t pwe, ChangedRangesCallback callback)
{
    QThread* t = QThread::currentThread();

    QByteArray uuidArr = uuid.toRfc4122();
    std::unique_ptr<btrdb::Stream> s = this->btrdb->streamFromUUID(uuidArr.constData());

    QVector<struct timerange>* ranges = new QVector<struct timerange>();

    // Set initial value to what we should return if there are no changed ranges
    RequestData* request_data = new RequestData(std::move(s));
    btrdb::Status status = request_data->stream->changesAsync(btrdb::default_ctx, [=](bool finished, btrdb::Status stat, std::vector<struct btrdb::ChangedRange>& data, std::uint64_t version)
    {
        Q_UNUSED(stat);
        if (data.size() != 0)
        {
            for (struct btrdb::ChangedRange& point : data)
            {
                struct timerange newpoint;
                newpoint.start = point.start;
                newpoint.end = point.end;
                ranges->push_back(newpoint);
            }
            request_data->version = version;
        }

        if (finished)
        {
            std::uint64_t saved_version = request_data->version;
            invokeOnThread(t, std::function<void()>([=]()
            {
                callback(ranges->data(), ranges->count(), saved_version);
                delete ranges;
            }));
            delete request_data;
        }
    }, fromGen, toGen, pwe);

    Q_UNUSED(status);
}
