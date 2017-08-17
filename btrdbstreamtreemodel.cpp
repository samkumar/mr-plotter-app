#include "btrdbstreamtreemodel.h"

#include <QJsonDocument>
#include <QList>
#include <QModelIndex>
#include <QThread>
#include <QTimer>
#include <QUuid>
#include <QVariant>

#include "btrdbdatasource.h"
#include "stream.h"

uint64_t BTrDBStreamTreeModel::nextUniqueID = 0;

BTrDBStreamTreeModel::BTrDBStreamTreeModel(QObject *parent)
    : QAbstractItemModel(parent), btrdbDataSource(nullptr),
      uniqueID(BTrDBStreamTreeModel::nextUniqueID++)
{
    this->rootNode = new RootTreeNode(this);
}

BTrDBStreamTreeModel::~BTrDBStreamTreeModel()
{
    delete this->rootNode;
}

QVariant BTrDBStreamTreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
    {
        return QVariant();
    }

    TreeNode* node = static_cast<TreeNode*>(index.internalPointer());
    StreamTreeNode* leafNode = dynamic_cast<StreamTreeNode*>(node);
    if (role > Qt::UserRole)
    {
    switch (role)
    {
        case StreamTreeRole::NameRole:
            return node->name();
        case StreamTreeRole::TagsRole:
            if (leafNode == nullptr)
            {
                return QVariant();
            }
            return leafNode->tagsString();
        case StreamTreeRole::AnnotationsRole:
            if (leafNode == nullptr)
            {
                return QVariant();
            }
            return leafNode->annotationsString();
        default:
            return QVariant();
        }
    }

    if (role == Qt::DisplayRole)
    {
        return node->data(index.column());
    }
    else if (role == Qt::ToolTipRole && leafNode != nullptr)
    {
        return leafNode->UUIDString();
    }

    return QVariant();
}

Qt::ItemFlags BTrDBStreamTreeModel::flags(const QModelIndex& index) const {
    if (index.isValid())
    {
        TreeNode* node = static_cast<TreeNode*>(index.internalPointer());
        StreamTreeNode* leafNode = dynamic_cast<StreamTreeNode*>(node);
        if (leafNode == nullptr)
        {
            // Flags for collection node
            return Qt::ItemIsEnabled;
        }
        else
        {
            // Flags for stream node
            return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
        }
    }
    else
    {
        return Qt::NoItemFlags;
    }
}

QVariant BTrDBStreamTreeModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        return this->rootNode->data(section);
    }
    return QVariant();
}

QModelIndex BTrDBStreamTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!this->hasIndex(row, column, parent))
    {
        return QModelIndex();
    }

    TreeNode* parentNode;
    if (parent.isValid())
    {
        parentNode = static_cast<TreeNode*>(parent.internalPointer());
    }
    else
    {
        parentNode = this->rootNode;
    }

    TreeNode* childNode = parentNode->child(row);
    if (childNode == nullptr)
    {
        return QModelIndex();
    }
    else
    {
        return this->createIndex(row, column, childNode);
    }
}

QModelIndex BTrDBStreamTreeModel::parent(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return QModelIndex();
    }

    TreeNode* childNode = static_cast<TreeNode*>(index.internalPointer());
    TreeNode* parentNode = childNode->parentNode();

    // Unsafe to call parentNode->row()
    if (parentNode == this->rootNode)
    {
        return QModelIndex();
    }

    return this->createIndex(parentNode->row(), 0, parentNode);
}

int BTrDBStreamTreeModel::rowCount(const QModelIndex& parent) const
{
    if (parent.column() > 0)
    {
        return 0;
    }

    TreeNode* parentNode;
    if (parent.isValid())
    {
        parentNode = static_cast<TreeNode*>(parent.internalPointer());
    }
    else
    {
        parentNode = this->rootNode;
    }

    return parentNode->childCount();
}

int BTrDBStreamTreeModel::columnCount(const QModelIndex& parent) const
{
    TreeNode* parentNode;
    if (parent.isValid())
    {
        parentNode = static_cast<TreeNode*>(parent.internalPointer());
    }
    else
    {
        parentNode = this->rootNode;
    }
    return parentNode->columnCount();
}

QHash<int, QByteArray> BTrDBStreamTreeModel::roleNames() const
{
    QHash<int, QByteArray> roles;// = this->QAbstractItemModel::roleNames();
    roles[StreamTreeRole::NameRole] = "name";
    roles[StreamTreeRole::TagsRole] = "tags";
    roles[StreamTreeRole::AnnotationsRole] = "annotations";
    return roles;
}

BTrDBDataSource* BTrDBStreamTreeModel::getBTrDBDataSource() const
{
    return this->btrdbDataSource;
}

void BTrDBStreamTreeModel::setBTrDBDataSource(BTrDBDataSource* b)
{
    this->beginResetModel();
    delete this->rootNode;
    this->rootNode = new RootTreeNode(this);
    this->btrdbDataSource = b;
    this->endResetModel();
    if (b != nullptr)
    {
        this->rootNode->requestCollections();
    }
}

QVariantList BTrDBStreamTreeModel::getStreams(const QItemSelection& selection)
{
    QVariantList streamObjectList;
    QModelIndexList indexList = selection.indexes();
    for (int i = 0; i != indexList.count(); i++)
    {
        const QModelIndex& index = indexList[i];
        TreeNode* node = static_cast<TreeNode*>(index.internalPointer());
        StreamTreeNode* leafNode = dynamic_cast<StreamTreeNode*>(node);
        if (leafNode != nullptr)
        {
            QString UUIDString = leafNode->UUIDString();
            QString collection = leafNode->collection();
            QVariantMap tags = leafNode->tagsMap();
            QVariantMap annotations = leafNode->annotationsMap();

            QString sourceString = QStringLiteral("BTrDB@%1").arg(this->btrdbDataSource->getHostPort());

            QVariantMap details;
            details["UUID"] = UUIDString;
            details["collection"] = collection;
            details["tags"] = tags;
            details["annotations"] = annotations;

            QVariantMap streamObject;
            streamObject["details"] = details;
            streamObject["name"] = leafNode->name();
            streamObject["labelText"] = QStringLiteral("%1: %2").arg(leafNode->collection()).arg(leafNode->name());
            streamObject["dataSource"] = QVariant::fromValue<QObject*>(this->btrdbDataSource);
            streamObject["sourceText"] = sourceString;
            streamObject["id"] = QStringLiteral("BTrDB;%1;%2").arg(this->uniqueID).arg(UUIDString);

            /* Generate a ToolTip. */
            QString toolTipTemplate = QStringLiteral("Source: %1\nUUID: %2\nCollection: %3\nTags: %4\nAnnotations: %5");
            QJsonDocument tagsDoc = QJsonDocument::fromVariant(tags);
            QJsonDocument annotationsDoc = QJsonDocument::fromVariant(annotations);
            streamObject["toolTipText"] = toolTipTemplate.arg(sourceString).arg(UUIDString).arg(collection).arg(QString(tagsDoc.toJson().trimmed())).arg(QString(annotationsDoc.toJson().trimmed()));

            /* Deduce the units. */
            if (annotations.contains("unit")) {
                streamObject["unit"] = annotations["unit"].toString();
            } else if (tags.contains("unit")) {
                streamObject["unit"] = tags["unit"].toString();
            } else {
                streamObject["unit"] = QStringLiteral("");
            }


            /*Stream* plotterObject = new Stream(this);
            plotterObject->setUUID(leafNode->UUIDString());
            plotterObject->setDataSource(this->btrdbDataSource);
            streamObject["plotterObject"] = QVariant::fromValue<QObject*>(plotterObject);

            Stream* ddPlotterObject = new Stream(this);
            ddPlotterObject->setUUID(leafNode->UUIDString());
            ddPlotterObject->setDataSource(this->btrdbDataSource);
            ddPlotterObject->dataDensity = true;
            streamObject["ddPlotterObject"] = QVariant::fromValue<QObject*>(ddPlotterObject);*/

            streamObjectList.append(streamObject);
        }
    }
    return streamObjectList;
}

/* TreeNode implementation. */

TreeNode::TreeNode(const QList<QVariant>& data, TreeNode* parentNode)
    : parent(parentNode), nodeData(data)
{
}

TreeNode::TreeNode(TreeNode* parentNode) : parent(parentNode)
{
}

TreeNode::~TreeNode()
{
    for (TreeNode* child : this->children)
    {
        delete child;
    }
}

void TreeNode::appendChild(TreeNode* child)
{
    this->children.append(child);
}

TreeNode* TreeNode::child(int row)
{
    return this->children.value(row);
}

int TreeNode::childCount()
{
    return this->children.count();
}

int TreeNode::columnCount()
{
    return this->nodeData.count();
}

QVariant TreeNode::data(int column) const
{
    return this->nodeData.value(column);
}

int TreeNode::row()
{
    if (this->parent != nullptr)
    {
        return this->parent->children.indexOf(const_cast<TreeNode*>(this));
    }

    return 0;
}

TreeNode* TreeNode::parentNode()
{
    return this->parent;
}

QString TreeNode::name() const
{
    return this->data(0).toString();
}


RootTreeNode::RootTreeNode(BTrDBStreamTreeModel* model)
    : TreeNode(), tree_model_(model), loading_(false)
{
}

int RootTreeNode::columnCount()  {
    return 1;
}

QVariant RootTreeNode::data(int column) const
{
    Q_UNUSED(column);
    return QVariant();
}

/* Copied from Michael's code in qtlibbw/utils.h. */
template<typename... Tz>
void invokeOnThread(QThread* t, std::function<void (Tz...)> f, Tz... args)
{
    QTimer* timer = new QTimer();
    timer->moveToThread(t);
    timer->setSingleShot(true);
    QObject::connect(timer, &QTimer::timeout, [=]{
        f(args...);
        timer->deleteLater();
    });
    QMetaObject::invokeMethod(timer, "start", Qt::QueuedConnection, Q_ARG(int, 0));
}

void RootTreeNode::requestCollections()
{
    if (this->loading_)
    {
        return;
    }
    this->loading_ = true;

    QStringList* collections_list = new QStringList();
    QThread* t = QThread::currentThread();
    std::shared_ptr<btrdb::BTrDB> btrdb = this->tree_model_->btrdbDataSource->getBTrDB();
    btrdb->listCollectionsAsync(btrdb::default_ctx, [=](bool finished, btrdb::Status status, const std::vector<std::string>& collections)
    {
        for (const std::string& coll : collections)
        {
            collections_list->append(QString::fromStdString(coll));
        }
        if (status.isError())
        {
            qDebug("Error requesting collections: %s", status.message().c_str());
        }
        if (finished)
        {
            // Actually add the new collections...
            invokeOnThread(t, std::function<void()>([=]()
            {
                this->setChildrenAndFree(collections_list);
                this->loading_ = false;
            }));
        }
    }, "");

}

void RootTreeNode::setChildrenAndFree(QStringList* collections_list)
{
    // TODO: delete any existing children
    Q_ASSERT(this->childCount() == 0);
    if (collections_list->count() != 0)
    {
        this->tree_model_->beginInsertRows(QModelIndex(), this->childCount(), this->childCount() + collections_list->count() - 1);
        for (int i = 0; i != collections_list->count(); i++)
        {
            QString coll_name = collections_list->value(i);
            TreeNode* child = new CollectionTreeNode(coll_name, this->tree_model_, this);
            this->appendChild(child);
        }
        this->tree_model_->endInsertRows();
    }
    delete collections_list;
}

CollectionTreeNode::CollectionTreeNode(QString collection_name, BTrDBStreamTreeModel* model, RootTreeNode* parent)
    : TreeNode(parent), collection_name_(collection_name), tree_model_(model), state_(LoadingState::NOT_LOADED)
{
    QList<QVariant> data;
    data << "Loading...";
    this->loading_message_ = new TreeNode(data, this);
}

CollectionTreeNode::~CollectionTreeNode()
{
    delete this->loading_message_;
}

int CollectionTreeNode::columnCount()
{
    return 1;
}

QVariant CollectionTreeNode::data(int column) const
{
    Q_UNUSED(column);
    return this->collection_name_;
}

int CollectionTreeNode::childCount()
{
    if (this->state_ == LoadingState::LOADED)
    {
        return this->TreeNode::childCount();
    }
    else
    {
        return 1;
    }
}

TreeNode* CollectionTreeNode::child(int row)
{
    if (this->state_ == LoadingState::LOADED)
    {
        return this->TreeNode::child(row);
    }
    else
    {
        if (this->state_ == LoadingState::NOT_LOADED)
        {
            // Initiate request to load data
            this->state_ = LoadingState::LOADING;
            this->requestStreams();
        }
        return this->loading_message_;
    }
}

void CollectionTreeNode::requestStreams()
{
    QVector<btrdb::Stream*>* streams_list = new QVector<btrdb::Stream*>;
    QThread* t = QThread::currentThread();
    std::shared_ptr<btrdb::BTrDB> btrdb = this->tree_model_->btrdbDataSource->getBTrDB();
    std::map<std::string, std::pair<std::string, bool>> empty_map;
    btrdb->lookupStreamsAsync(btrdb::default_ctx, std::function<void(bool, btrdb::Status, std::vector<std::unique_ptr<btrdb::Stream>>&)>([=](bool finished, btrdb::Status status, std::vector<std::unique_ptr<btrdb::Stream>>& streams)
    {
        for (std::unique_ptr<btrdb::Stream>& stream : streams)
        {
            streams_list->append(stream.release());
        }
        if (status.isError())
        {
            qDebug("Error requesting streams: %s", status.message().c_str());
        }
        if (finished)
        {
            invokeOnThread(t, std::function<void()>([=]()
            {
                this->setChildrenAndFree(streams_list);
            }));
        }
    }), this->collection_name_.toStdString(), false, empty_map, empty_map);
}

void CollectionTreeNode::setChildrenAndFree(QVector<btrdb::Stream*>* streams_list)
{
    /* First, get the index of this node. */
    QModelIndex parent_index = this->tree_model_->index(this->row(), 0, QModelIndex());

    /* Next, "delete" the loading message child node. */
    this->tree_model_->beginRemoveRows(parent_index, 0, 0);
    this->state_ = LoadingState::LOADED;
    this->tree_model_->endRemoveRows();

    /* Now, add the nodes corresponding to the streams. */
    this->tree_model_->beginInsertRows(parent_index, 0, streams_list->count() - 1);
    for (int i = 0; i != streams_list->count(); i++)
    {
        btrdb::Stream* stream = streams_list->value(i);
        StreamTreeNode* child = new StreamTreeNode(stream, this);
        this->appendChild(child);
    }
    this->tree_model_->endInsertRows();
    delete streams_list;
}

StreamTreeNode::StreamTreeNode(btrdb::Stream* stream, CollectionTreeNode* parent)
    : TreeNode(parent), stream_(stream)
{
}

StreamTreeNode::~StreamTreeNode()
{
    delete this->stream_;
}

int StreamTreeNode::columnCount()
{
    return 3;
}

QVariant StreamTreeNode::data(int column) const
{
    switch (column)
    {
    case 0:
        return this->name();
    case 1:
        return this->tagsString();
    case 2:
        return this->annotationsString();
    default:
        return QVariant();
    }
}

QString StreamTreeNode::UUIDString() const
{
    const void* uuid_bytes = this->stream_->UUID();
    QByteArray uuid_byte_array = QByteArray::fromRawData(reinterpret_cast<const char*>(uuid_bytes), 16);
    QUuid uuid = QUuid::fromRfc4122(uuid_byte_array);
    QString uuid_string = uuid.toString();
    return uuid_string.mid(1, uuid_string.length() - 2);
}

QString StreamTreeNode::collection() const
{
    const std::string* collection_string;
    this->stream_->collection(nullptr, &collection_string);
    return QString::fromStdString(*collection_string);
}

QString StreamTreeNode::name() const
{
    /* First, search for a "name" key in annotations, and then tags. */
    const std::map<std::string, std::string>* string_map;
    std::uint64_t annotations_ver;
    btrdb::Status status = this->stream_->cachedAnnotations(nullptr, &string_map, &annotations_ver);
    if (!status.isError())
    {
        auto it = string_map->find("name");
        if (it != string_map->end())
        {
            return QString::fromStdString(it->second);
        }
    }
    status = this->stream_->tags(nullptr, &string_map);
    if (!status.isError())
    {
        auto it = string_map->find("name");
        if (it != string_map->end())
        {
            return QString::fromStdString(it->second);
        }
    }

   return this->UUIDString();
}

QString stringify_map(const std::map<std::string, std::string>& string_map)
{
    QString stringified;
    bool first_iteration = true;
    for (auto it = string_map.begin(); it != string_map.end(); it++)
    {
        if (first_iteration)
        {
            first_iteration = false;
        }
        else
        {
            stringified.append(',');
        }
        stringified.append(it->first.c_str());
        stringified.append('=');
        stringified.append(it->second.c_str());
    }
    return stringified;
}

QString StreamTreeNode::tagsString() const
{
    const std::map<std::string, std::string>* tags_map;
    btrdb::Status status = this->stream_->tags(nullptr, &tags_map);
    if (status.isError())
    {
        return QString::fromStdString(status.message());
    }
    return stringify_map(*tags_map);
}

QString StreamTreeNode::annotationsString() const
{
    const std::map<std::string, std::string>* annotations_map;
    std::uint64_t annotations_ver;
    btrdb::Status status = this->stream_->cachedAnnotations(nullptr, &annotations_map, &annotations_ver);
    if (status.isError())
    {
        return QString::fromStdString(status.message());
    }
    QString str = stringify_map(*annotations_map);
    QString ver = QStringLiteral("[ver=%1]").arg(annotations_ver);
    if (str.length() == 0)
    {
        return ver;
    }
    str.append(" ");
    str.append(ver);
    return str;
}

QVariantMap to_variant_map(const std::map<std::string, std::string>& string_map)
{
    QVariantMap variant_map;
    for (auto it = string_map.begin(); it != string_map.end(); it++)
    {
        variant_map[QString::fromStdString(it->first)] = QString::fromStdString(it->second);
    }
    return variant_map;
}

QVariantMap StreamTreeNode::tagsMap() const
{
    const std::map<std::string, std::string>* tags_map;
    btrdb::Status status = this->stream_->tags(nullptr, &tags_map);
    if (status.isError())
    {
        return QVariantMap();
    }
    return to_variant_map(*tags_map);
}

QVariantMap StreamTreeNode::annotationsMap() const
{
    const std::map<std::string, std::string>* annotations_map;
    std::uint64_t annotations_ver;
    btrdb::Status status = this->stream_->cachedAnnotations(nullptr, &annotations_map, &annotations_ver);
    if (status.isError())
    {
        return QVariantMap();
    }
    return to_variant_map(*annotations_map);
}
