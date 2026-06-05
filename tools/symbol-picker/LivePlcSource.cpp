#include "LivePlcSource.h"

#include <QHash>
#include <QList>
#include <QSet>

extern "C" {
#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
}

namespace {

// "ns=N;s=..." / "ns=N;i=..." canonicalization — mirrors the helper in
// plc_link.cpp so JSON consumers see identical NodeId strings whether the
// source was live or a sidecar dump.
QString nodeIdToString(const UA_NodeId& id)
{
    QString s = QStringLiteral("ns=%1;").arg(id.namespaceIndex);
    switch (id.identifierType) {
        case UA_NODEIDTYPE_NUMERIC:
            s += QStringLiteral("i=%1").arg(id.identifier.numeric);
            break;
        case UA_NODEIDTYPE_STRING:
            s += QStringLiteral("s=%1").arg(QString::fromUtf8(
                reinterpret_cast<const char*>(id.identifier.string.data),
                static_cast<int>(id.identifier.string.length)));
            break;
        default:
            s += QStringLiteral("<%1>").arg(int(id.identifierType));
    }
    return s;
}

UA_NodeId parseNodeId(const QString& s, bool* ok)
{
    *ok = false;
    if (!s.startsWith(QStringLiteral("ns="))) return UA_NODEID_NULL;
    const int semi = s.indexOf(QLatin1Char(';'));
    if (semi < 0) return UA_NODEID_NULL;
    bool nsOk = false;
    const int ns = s.mid(3, semi - 3).toInt(&nsOk);
    if (!nsOk) return UA_NODEID_NULL;
    const QString tail = s.mid(semi + 1);
    if (tail.startsWith(QStringLiteral("s="))) {
        const QByteArray ident = tail.mid(2).toUtf8();
        *ok = true;
        return UA_NODEID_STRING_ALLOC(static_cast<UA_UInt16>(ns), ident.constData());
    }
    if (tail.startsWith(QStringLiteral("i="))) {
        bool numOk = false;
        const quint32 ident = tail.mid(2).toUInt(&numOk);
        if (!numOk) return UA_NODEID_NULL;
        *ok = true;
        return UA_NODEID_NUMERIC(static_cast<UA_UInt16>(ns), ident);
    }
    return UA_NODEID_NULL;
}

QString nodeClassToString(UA_NodeClass c)
{
    switch (c) {
        case UA_NODECLASS_OBJECT:        return QStringLiteral("Object");
        case UA_NODECLASS_VARIABLE:      return QStringLiteral("Variable");
        case UA_NODECLASS_METHOD:        return QStringLiteral("Method");
        case UA_NODECLASS_OBJECTTYPE:    return QStringLiteral("ObjectType");
        case UA_NODECLASS_VARIABLETYPE:  return QStringLiteral("VariableType");
        case UA_NODECLASS_REFERENCETYPE: return QStringLiteral("ReferenceType");
        case UA_NODECLASS_DATATYPE:      return QStringLiteral("DataType");
        case UA_NODECLASS_VIEW:          return QStringLiteral("View");
        default:                         return QStringLiteral("?");
    }
}

void readNamespaceArray(UA_Client* c, QHash<int, QString>& out)
{
    UA_Variant v;
    UA_Variant_init(&v);
    const UA_StatusCode rc = UA_Client_readValueAttribute(
        c, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_NAMESPACEARRAY), &v);
    if (rc != UA_STATUSCODE_GOOD) {
        UA_Variant_clear(&v);
        return;
    }
    if (UA_Variant_isScalar(&v) || v.type != &UA_TYPES[UA_TYPES_STRING]) {
        UA_Variant_clear(&v);
        return;
    }
    const auto* arr = static_cast<const UA_String*>(v.data);
    for (size_t i = 0; i < v.arrayLength; ++i) {
        out.insert(int(i), QString::fromUtf8(
            reinterpret_cast<const char*>(arr[i].data),
            static_cast<int>(arr[i].length)));
    }
    UA_Variant_clear(&v);
}

// Walk Objects looking for a |var|...Application leaf. Same heuristic
// PlcLink::findApplicationRoot uses: skip ns=0 numeric refs so the
// budget isn't burned on the standard server/diagnostics hierarchy.
bool findAppRoot(UA_Client* c, QString& outId, int& outNs)
{
    struct Pending { UA_NodeId id; int depth; };
    QList<Pending> queue;
    {
        UA_NodeId seed = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
        UA_NodeId copy; UA_NodeId_init(&copy);
        UA_NodeId_copy(&seed, &copy);
        queue.append({copy, 0});
    }
    int visited = 0;
    const int maxVisited = 1000;
    const int maxDepth   = 6;
    while (!queue.isEmpty() && visited < maxVisited && outId.isEmpty()) {
        Pending p = queue.takeFirst();
        UA_BrowseRequest req; UA_BrowseRequest_init(&req);
        req.requestedMaxReferencesPerNode = 0;
        req.nodesToBrowse = UA_BrowseDescription_new();
        req.nodesToBrowseSize = 1;
        UA_NodeId_copy(&p.id, &req.nodesToBrowse[0].nodeId);
        req.nodesToBrowse[0].browseDirection = UA_BROWSEDIRECTION_FORWARD;
        req.nodesToBrowse[0].includeSubtypes = true;
        req.nodesToBrowse[0].referenceTypeId = UA_NODEID_NUMERIC(0, 0);
        req.nodesToBrowse[0].nodeClassMask   = 0;
        req.nodesToBrowse[0].resultMask      = UA_BROWSERESULTMASK_BROWSENAME;

        UA_BrowseResponse rsp = UA_Client_Service_browse(c, req);
        if (rsp.responseHeader.serviceResult == UA_STATUSCODE_GOOD
            && rsp.resultsSize > 0) {
            const UA_BrowseResult& r = rsp.results[0];
            for (size_t i = 0; i < r.referencesSize
                 && visited < maxVisited && outId.isEmpty(); ++i) {
                const UA_ReferenceDescription& ref = r.references[i];
                if (ref.nodeId.nodeId.namespaceIndex == 0
                    && ref.nodeId.nodeId.identifierType == UA_NODEIDTYPE_NUMERIC) {
                    continue;
                }
                ++visited;
                if (ref.nodeId.nodeId.identifierType == UA_NODEIDTYPE_STRING) {
                    const QString s = QString::fromUtf8(
                        reinterpret_cast<const char*>(
                            ref.nodeId.nodeId.identifier.string.data),
                        static_cast<int>(
                            ref.nodeId.nodeId.identifier.string.length));
                    if (s.startsWith(QStringLiteral("|var|"))
                        && s.endsWith(QStringLiteral(".Application"))) {
                        outNs = ref.nodeId.nodeId.namespaceIndex;
                        outId = QStringLiteral("ns=%1;s=%2").arg(outNs).arg(s);
                        break;
                    }
                }
                if (p.depth + 1 < maxDepth) {
                    UA_NodeId copy; UA_NodeId_init(&copy);
                    UA_NodeId_copy(&ref.nodeId.nodeId, &copy);
                    queue.append({copy, p.depth + 1});
                }
            }
        }
        UA_NodeId_clear(&p.id);
        UA_BrowseResponse_clear(&rsp);
        UA_BrowseRequest_clear(&req);
    }
    while (!queue.isEmpty()) {
        Pending p = queue.takeFirst();
        UA_NodeId_clear(&p.id);
    }
    return !outId.isEmpty();
}

} // namespace

LivePlcSource::LivePlcSource(QString endpointUrl)
    : endpointUrl_(std::move(endpointUrl)) {}

BrowseRoot* LivePlcSource::load()
{
    lastError_.clear();

    UA_Client* client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    const QByteArray url = endpointUrl_.toUtf8();
    UA_StatusCode rc = UA_Client_connect(client, url.constData());
    if (rc != UA_STATUSCODE_GOOD) {
        lastError_ = QStringLiteral("connect %1 failed: %2")
            .arg(endpointUrl_, QString::fromLatin1(UA_StatusCode_name(rc)));
        UA_Client_delete(client);
        return nullptr;
    }

    auto* root = new BrowseRoot;
    root->endpoint = endpointUrl_;

    // Namespace table
    QHash<int, QString> nsMap;
    readNamespaceArray(client, nsMap);

    // App root
    if (!findAppRoot(client, root->appRootNodeId, root->appNamespaceIndex)) {
        lastError_ = QStringLiteral(
            "could not locate |var|...Application node — wrong device?");
        UA_Client_disconnect(client);
        UA_Client_delete(client);
        delete root;
        return nullptr;
    }
    root->namespaceUri = nsMap.value(root->appNamespaceIndex);

    // BFS browse — mirrors PlcLink::doBrowse() including the ns filter.
    struct Pending { UA_NodeId id; QString idStr; int depth; BrowseNode* node; };
    QList<Pending> queue;

    // Seed entry
    auto* seedNode = new BrowseNode;
    seedNode->id        = root->appRootNodeId;
    seedNode->name      = QStringLiteral("Application");
    seedNode->nodeClass = QStringLiteral("Object");
    seedNode->depth     = 0;
    root->roots.append(seedNode);
    root->allNodes.append(seedNode);

    {
        bool ok = false;
        UA_NodeId seedId = parseNodeId(root->appRootNodeId, &ok);
        if (!ok) {
            lastError_ = QStringLiteral("bad appRootNodeId");
            UA_Client_disconnect(client);
            UA_Client_delete(client);
            delete root;
            return nullptr;
        }
        UA_NodeId copy; UA_NodeId_init(&copy);
        UA_NodeId_copy(&seedId, &copy);
        UA_NodeId_clear(&seedId);
        queue.append({copy, root->appRootNodeId, 0, seedNode});
    }

    int seen = 1;
    while (!queue.isEmpty() && seen < kMaxNodes) {
        Pending p = queue.takeFirst();

        UA_BrowseRequest req; UA_BrowseRequest_init(&req);
        req.requestedMaxReferencesPerNode = 0;
        req.nodesToBrowse = UA_BrowseDescription_new();
        req.nodesToBrowseSize = 1;
        UA_NodeId_copy(&p.id, &req.nodesToBrowse[0].nodeId);
        req.nodesToBrowse[0].browseDirection = UA_BROWSEDIRECTION_FORWARD;
        req.nodesToBrowse[0].includeSubtypes = true;
        req.nodesToBrowse[0].referenceTypeId = UA_NODEID_NUMERIC(0, 0);
        req.nodesToBrowse[0].nodeClassMask   = 0;
        req.nodesToBrowse[0].resultMask =
            UA_BROWSERESULTMASK_NODECLASS | UA_BROWSERESULTMASK_BROWSENAME;

        UA_BrowseResponse rsp = UA_Client_Service_browse(client, req);
        if (rsp.responseHeader.serviceResult == UA_STATUSCODE_GOOD
            && rsp.resultsSize > 0) {
            const UA_BrowseResult& r = rsp.results[0];
            for (size_t i = 0; i < r.referencesSize && seen < kMaxNodes; ++i) {
                const UA_ReferenceDescription& ref = r.references[i];
                const int childNs = ref.nodeId.nodeId.namespaceIndex;
                const bool isNumeric =
                    ref.nodeId.nodeId.identifierType == UA_NODEIDTYPE_NUMERIC;
                if (childNs == 0 && isNumeric) continue;
                if (root->appNamespaceIndex >= 0
                    && childNs != root->appNamespaceIndex) continue;

                const QString id = nodeIdToString(ref.nodeId.nodeId);
                const QString name = QString::fromUtf8(
                    reinterpret_cast<const char*>(ref.browseName.name.data),
                    static_cast<int>(ref.browseName.name.length));

                auto* child       = new BrowseNode;
                child->id         = id;
                child->parentId   = p.idStr;
                child->name       = name;
                child->nodeClass  = nodeClassToString(ref.nodeClass);
                child->depth      = p.depth + 1;
                child->parent     = p.node;
                p.node->children.append(child);
                root->allNodes.append(child);
                ++seen;

                if (p.depth + 1 < kMaxDepth) {
                    UA_NodeId copy; UA_NodeId_init(&copy);
                    UA_NodeId_copy(&ref.nodeId.nodeId, &copy);
                    queue.append({copy, id, p.depth + 1, child});
                }
            }
        }
        UA_NodeId_clear(&p.id);
        UA_BrowseResponse_clear(&rsp);
        UA_BrowseRequest_clear(&req);
    }
    while (!queue.isEmpty()) {
        Pending p = queue.takeFirst();
        UA_NodeId_clear(&p.id);
    }

    UA_Client_disconnect(client);
    UA_Client_delete(client);

    root->generated = QString();  // filled in by the caller (PickerWindow)
    return root;
}
