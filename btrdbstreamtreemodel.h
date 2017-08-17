#ifndef BTRDBSTREAMTREEMODEL_H
#define BTRDBSTREAMTREEMODEL_H

#include <memory>
#include <btrdb/btrdb.h>
#include <QAbstractItemModel>
#include <QItemSelection>
#include <QList>
#include <QObject>
#include <QVariant>

#include "btrdbdatasource.h"

class TreeNode;
class RootTreeNode;

class BTrDBStreamTreeModel : public QAbstractItemModel
{
    Q_OBJECT
    Q_PROPERTY(BTrDBDataSource* btrdb READ getBTrDBDataSource WRITE setBTrDBDataSource)
public:
    friend class RootTreeNode;
    friend class CollectionTreeNode;

    enum StreamTreeRole
    {
        NameRole = Qt::UserRole + 1,
        TagsRole,
        AnnotationsRole
    };

    explicit BTrDBStreamTreeModel(QObject *parent = 0);
    ~BTrDBStreamTreeModel();

    QVariant data(const QModelIndex& index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    BTrDBDataSource* getBTrDBDataSource() const;
    void setBTrDBDataSource(BTrDBDataSource* b);

    Q_INVOKABLE QVariantList getStreams(const QItemSelection& selection);

signals:

public slots:

private:
    RootTreeNode* rootNode;
    BTrDBDataSource* btrdbDataSource;
    const uint64_t uniqueID;

    static uint64_t nextUniqueID;
};

/*
 * The way that Trees map to models is as follows.
 * Each child of the node is a row in the model.
 */

class TreeNode
{
public:
    explicit TreeNode(const QList<QVariant>& data, TreeNode* parentNode = nullptr);
    explicit TreeNode(TreeNode* parentNode = nullptr);
    virtual ~TreeNode();

    void appendChild(TreeNode* child);
    virtual TreeNode* child(int row);
    virtual int childCount();
    virtual int columnCount();
    virtual QVariant data(int column) const;
    virtual int row();
    virtual TreeNode* parentNode();

    virtual QString name() const;

private:
    TreeNode* parent;
    QVector<TreeNode*> children;
    QList<QVariant> nodeData;
};

/*
 * We differentiate between the top level nodes
 * that store collections, and the lower level nodes
 * that store streams. There is also a special node
 * used for the invisible root.
 */

class RootTreeNode : public TreeNode
{
public:
    explicit RootTreeNode(BTrDBStreamTreeModel* model);
    int columnCount() override;
    QVariant data(int column) const override;

    void requestCollections();
    void setChildrenAndFree(QStringList* collections_list);

private:
    BTrDBStreamTreeModel* tree_model_;
    bool loading_;
};

enum LoadingState {
    NOT_LOADED,
    LOADING,
    LOADED
};


class CollectionTreeNode : public TreeNode
{
public:
    explicit CollectionTreeNode(QString collection_name, BTrDBStreamTreeModel* model, RootTreeNode* parent);
    ~CollectionTreeNode();

    int columnCount() override;
    QVariant data(int column) const override;
    int childCount() override;
    TreeNode* child(int row) override;

    void requestStreams();
    void setChildrenAndFree(QVector<btrdb::Stream*>* streams_list);

private:
    TreeNode* loading_message_;
    QString collection_name_;
    BTrDBStreamTreeModel* tree_model_;
    LoadingState state_;
};

class StreamTreeNode : public TreeNode
{
public:
    explicit StreamTreeNode(btrdb::Stream* stream, CollectionTreeNode* parent);
    ~StreamTreeNode();

    int columnCount() override;
    QVariant data(int column) const override;

    QString UUIDString() const;
    QString collection() const;

    QString name() const override;
    QString tagsString() const;
    QString annotationsString() const;

    QVariantMap tagsMap() const;
    QVariantMap annotationsMap() const;

private:
    btrdb::Stream* stream_;
};

#endif // BTRDBSTREAMTREEMODEL_H
