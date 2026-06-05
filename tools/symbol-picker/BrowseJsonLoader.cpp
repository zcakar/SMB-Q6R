#include "BrowseJsonLoader.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <memory>

BrowseRoot* BrowseJsonLoader::load(const QString& path)
{
    lastError_.clear();

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        lastError_ = QStringLiteral("cannot open %1 — %2")
                         .arg(path, f.errorString());
        return nullptr;
    }

    QJsonParseError parseErr{};
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &parseErr);
    if (parseErr.error != QJsonParseError::NoError) {
        lastError_ = QStringLiteral("json parse failed at offset %1 — %2")
                         .arg(parseErr.offset).arg(parseErr.errorString());
        return nullptr;
    }
    if (!doc.isObject()) {
        lastError_ = QStringLiteral("expected json object at root");
        return nullptr;
    }
    const QJsonObject root = doc.object();

    // unique_ptr while we build it; release on success so the caller owns
    // the BrowseRoot and gets ~BrowseRoot() cleanup if it drops the ptr.
    auto out = std::make_unique<BrowseRoot>();
    out->generated         = root.value(QStringLiteral("generated")).toString();
    out->endpoint          = root.value(QStringLiteral("endpoint")).toString();
    out->appRootNodeId     = root.value(QStringLiteral("appRootNodeId")).toString();
    out->appNamespaceIndex = root.value(QStringLiteral("appNamespaceIndex")).toInt(-1);

    // Namespace URIs — pick the entry matching appNamespaceIndex.
    const QJsonArray nsArr = root.value(QStringLiteral("namespaces")).toArray();
    for (const QJsonValue& v : nsArr) {
        const QJsonObject ns = v.toObject();
        if (ns.value(QStringLiteral("index")).toInt() == out->appNamespaceIndex)
            out->namespaceUri = ns.value(QStringLiteral("uri")).toString();
    }

    // First pass — materialise every node and index by id so the second
    // pass can wire parent/child pointers in O(N).
    QHash<QString, BrowseNode*> byId;
    const QJsonArray nodesArr = root.value(QStringLiteral("nodes")).toArray();
    out->allNodes.reserve(nodesArr.size());
    for (const QJsonValue& v : nodesArr) {
        const QJsonObject n = v.toObject();
        auto* node       = new BrowseNode;
        node->id         = n.value(QStringLiteral("id")).toString();
        node->parentId   = n.value(QStringLiteral("parentId")).toString();
        node->name       = n.value(QStringLiteral("name")).toString();
        node->nodeClass  = n.value(QStringLiteral("class")).toString();
        node->depth      = n.value(QStringLiteral("depth")).toInt();
        out->allNodes.append(node);
        byId.insert(node->id, node);
    }

    // Second pass — link children to parents. Nodes with no parentId or
    // an unknown parentId become roots (latter shouldn't happen with a
    // well-formed dump but stay resilient anyway).
    for (BrowseNode* n : out->allNodes) {
        if (n->parentId.isEmpty()) {
            out->roots.append(n);
            continue;
        }
        BrowseNode* p = byId.value(n->parentId, nullptr);
        if (!p) {
            out->roots.append(n);
            continue;
        }
        n->parent = p;
        p->children.append(n);
    }

    return out.release();
}
