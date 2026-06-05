#pragma once

#include <QHash>
#include <QList>
#include <QString>

// In-memory representation of one node from plc-browse.json. Owns its
// children — the picker tree model wraps these pointers directly rather
// than storing indices, since the dataset is read-only after load.
struct BrowseNode {
    QString          id;            // canonical "ns=N;..." node id
    QString          parentId;      // canonical parent id; "" for root
    QString          name;          // OPC UA browse name
    QString          nodeClass;     // "Variable" / "Object" / ...
    int              depth = 0;
    BrowseNode*      parent = nullptr;
    QList<BrowseNode*> children;
};

struct BrowseRoot {
    QString    generated;
    QString    endpoint;
    QString    appRootNodeId;
    int        appNamespaceIndex = -1;
    QString    namespaceUri;        // URI of appNamespaceIndex, if known
    QList<BrowseNode*> roots;       // top-level nodes (typically one: Application)

    // Owns every node ever allocated; the picker hands out raw pointers
    // because nothing mutates after load, but cleanup goes through here.
    QList<BrowseNode*> allNodes;

    ~BrowseRoot() { qDeleteAll(allNodes); }
};

class BrowseJsonLoader
{
public:
    // Parse plc-browse.json. Returns nullptr + sets lastError() on
    // malformed input.
    BrowseRoot* load(const QString& path);
    QString lastError() const { return lastError_; }

private:
    QString lastError_;
};
