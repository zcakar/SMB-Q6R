#include "plc_link.h"

#include <QThread>
#include <QTimer>
#include <QDebug>
#include <QMetaObject>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>

#ifdef SMB_Q6R_HAS_OPCUA
extern "C" {
#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_subscriptions.h>
}
#endif

namespace smbq6r {

namespace {

#ifdef SMB_Q6R_HAS_OPCUA

// Render a UA_NodeId back into the canonical "ns=N;s=..." / "ns=N;i=..."
// form so the discovered node IDs round-trip through our string-based
// QML API. We don't handle GUID/ByteString identifiers — they don't
// appear in CodeSys symbol exports.
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

// Parse "ns=N;s=STRING" or "ns=N;i=NUMBER" into a UA_NodeId. Caller owns
// any heap memory allocated for string identifiers and must release it
// via UA_NodeId_clear() once the call that uses it has finished.
//
// We deliberately keep the parser tiny — strict format, no whitespace
// tolerance. Anything more elaborate is the caller's job.
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

// Convert a UA_Variant scalar value into a QVariant. Arrays are returned
// as an empty QVariantList for now — extending later is straightforward
// but we don't have a use case yet on the CodeSys side.
QVariant variantFromUa(const UA_Variant& v)
{
    if (UA_Variant_isEmpty(&v)) return {};
    if (!UA_Variant_isScalar(&v)) return QVariantList{};

    const UA_DataType* t = v.type;
    if (t == &UA_TYPES[UA_TYPES_BOOLEAN])
        return *static_cast<const UA_Boolean*>(v.data);
    if (t == &UA_TYPES[UA_TYPES_SBYTE])
        return *static_cast<const UA_SByte*>(v.data);
    if (t == &UA_TYPES[UA_TYPES_BYTE])
        return *static_cast<const UA_Byte*>(v.data);
    if (t == &UA_TYPES[UA_TYPES_INT16])
        return *static_cast<const UA_Int16*>(v.data);
    if (t == &UA_TYPES[UA_TYPES_UINT16])
        return *static_cast<const UA_UInt16*>(v.data);
    if (t == &UA_TYPES[UA_TYPES_INT32])
        return *static_cast<const UA_Int32*>(v.data);
    if (t == &UA_TYPES[UA_TYPES_UINT32])
        return *static_cast<const UA_UInt32*>(v.data);
    if (t == &UA_TYPES[UA_TYPES_INT64])
        return qint64(*static_cast<const UA_Int64*>(v.data));
    if (t == &UA_TYPES[UA_TYPES_UINT64])
        return quint64(*static_cast<const UA_UInt64*>(v.data));
    if (t == &UA_TYPES[UA_TYPES_FLOAT])
        return *static_cast<const UA_Float*>(v.data);
    if (t == &UA_TYPES[UA_TYPES_DOUBLE])
        return *static_cast<const UA_Double*>(v.data);
    if (t == &UA_TYPES[UA_TYPES_STRING]) {
        const auto* s = static_cast<const UA_String*>(v.data);
        return QString::fromUtf8(reinterpret_cast<const char*>(s->data),
                                 static_cast<int>(s->length));
    }
    // Unknown / complex type — return type name so the UI shows *something*
    // instead of an empty QVariant.
    return QString::fromLatin1("<%1>").arg(QString::fromLatin1(t->typeName));
}

void dataChangeTrampoline(UA_Client* /*client*/, UA_UInt32 /*subId*/,
                          void* /*subContext*/, UA_UInt32 monId, void* monContext,
                          UA_DataValue* value)
{
    auto* self = static_cast<PlcLink*>(monContext);
    if (!self || !value || !value->hasValue) return;
    const QString id = self->property(("monId_" + QByteArray::number(monId)).constData()).toString();
    if (id.isEmpty()) return;
    QMetaObject::invokeMethod(self, "valueChanged",
        Q_ARG(QString, id),
        Q_ARG(QVariant, variantFromUa(value->value)));
}

// Tree-dump scaffolding ----------------------------------------------------
// doBrowse builds a flat list of PlcLink::BrowseEntry while doing BFS plus
// a parentNodeId -> [child entry indices] map; renderTreeNode() walks that
// map to produce the unicode-branch indented form written to plc-browse.txt.
// BrowseEntry itself lives in plc_link.h so writeBrowseDump() can be a
// member function.

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

void renderTreeNode(QTextStream& out,
                    const QList<PlcLink::BrowseEntry>& entries,
                    const QHash<QString, QList<int>>& childrenByParent,
                    int idx,
                    const QString& linePrefix,
                    const QString& childPrefix)
{
    const PlcLink::BrowseEntry& e = entries[idx];

    // Left column: tree-branches + browse name, padded to a fixed width so
    // the node IDs line up. We pad in QChar units (not bytes) — fine for
    // ASCII identifier names. Box-drawing chars in the prefix count as 1
    // QChar each.
    QString left = linePrefix + e.browseName;
    const int targetWidth = 64;
    if (left.length() < targetWidth)
        left.append(QString(targetWidth - left.length(), QLatin1Char(' ')));

    // Output the left column + node id; wrap the class tag and brackets
    // in a QStringLiteral so the brackets are still QString units (they're
    // ASCII so the literal is not strictly required, but it keeps the
    // output path uniformly QString-only).
    out << left << QStringLiteral("  ") << e.nodeId;
    if (e.nodeClass != QStringLiteral("Object"))
        out << QStringLiteral("  [") << e.nodeClass << QStringLiteral("]");
    out << QStringLiteral("\n");

    // Branch glyphs are pre-materialised once and reused — same UTF-16
    // round-trip rationale as the header rules above.
    static const QString branchLast   = QStringLiteral("└─ ");
    static const QString branchMid    = QStringLiteral("├─ ");
    static const QString spineLast    = QStringLiteral("   ");
    static const QString spineMid     = QStringLiteral("│  ");

    const QList<int> kids = childrenByParent.value(e.nodeId);
    for (int i = 0; i < kids.size(); ++i) {
        const bool last = (i == kids.size() - 1);
        renderTreeNode(out, entries, childrenByParent, kids[i],
                       childPrefix + (last ? branchLast : branchMid),
                       childPrefix + (last ? spineLast  : spineMid));
    }
}

#endif // SMB_Q6R_HAS_OPCUA

} // namespace

PlcLink::PlcLink(QObject* parent)
    : QObject(parent)
{
    // Cross-thread queued connections need a metatype for every parameter.
    // Q_ENUM does not auto-register for Qt::QueuedConnection.
    qRegisterMetaType<PlcLink::State>("smbq6r::PlcLink::State");

    workerThread_ = new QThread(this);
    workerThread_->setObjectName(QStringLiteral("PlcLinkWorker"));
    moveToThread(workerThread_);

    iterateTimer_ = new QTimer();   // parent set after moveToThread
    iterateTimer_->setInterval(50); // 20 Hz polling — good balance for UI updates
    iterateTimer_->moveToThread(workerThread_);
    connect(iterateTimer_, &QTimer::timeout, this, &PlcLink::onIterate, Qt::DirectConnection);

    workerThread_->start();
}

PlcLink::~PlcLink()
{
    // Stop the worker thread cleanly. Do this synchronously so the
    // destructor doesn't outpace the open62541 cleanup on shutdown.
    QMetaObject::invokeMethod(this, "doDisconnect", Qt::BlockingQueuedConnection);
    workerThread_->quit();
    workerThread_->wait(1000);
    delete iterateTimer_;
}

UA_Client* PlcLink::makeClient()
{
#ifdef SMB_Q6R_HAS_OPCUA
    UA_Client* c = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(c));
    return c;
#else
    return nullptr;
#endif
}

void PlcLink::setState(State s)
{
    if (state_ == s) return;
    state_ = s;
    emit stateChanged(s);
}

void PlcLink::connectToServer(const QString& endpointUrl)
{
    // Cross-thread queue. The worker picks this up on its event loop.
    QMetaObject::invokeMethod(this, "doConnect", Qt::QueuedConnection,
                              Q_ARG(QString, endpointUrl));
}

void PlcLink::disconnectFromServer()
{
    QMetaObject::invokeMethod(this, "doDisconnect", Qt::QueuedConnection);
}

void PlcLink::readNode(const QString& nodeId)
{
    QMetaObject::invokeMethod(this, "doRead", Qt::QueuedConnection,
                              Q_ARG(QString, nodeId));
}

void PlcLink::subscribeNode(const QString& nodeId)
{
    QMetaObject::invokeMethod(this, "doSubscribe", Qt::QueuedConnection,
                              Q_ARG(QString, nodeId));
}

void PlcLink::browseNamespace(int maxDepth, int maxNodes)
{
    QMetaObject::invokeMethod(this, "doBrowse", Qt::QueuedConnection,
                              Q_ARG(int, maxDepth), Q_ARG(int, maxNodes));
}

void PlcLink::writeNode(const QString& nodeId, const QVariant& value)
{
    QMetaObject::invokeMethod(this, "doWrite", Qt::QueuedConnection,
                              Q_ARG(QString, nodeId),
                              Q_ARG(QVariant, value));
}

void PlcLink::doConnect(QString endpointUrl)
{
#ifdef SMB_Q6R_HAS_OPCUA
    // Idempotent: if we're already connecting to or connected on the
    // requested URL just keep the existing client. Re-pressing the UI
    // button should not silently tear the live session down.
    if (client_ && (state_ == State::Connecting || state_ == State::Connected)
        && serverUrl_ == endpointUrl) {
        return;
    }
    serverUrl_ = endpointUrl;
    if (client_) destroyClient();

    setState(State::Connecting);
    client_ = makeClient();

    const QByteArray url = endpointUrl.toUtf8();
    const UA_StatusCode rc = UA_Client_connect(client_, url.constData());
    if (rc != UA_STATUSCODE_GOOD) {
        lastError_ = QString::fromLatin1(UA_StatusCode_name(rc));
        qWarning() << "PlcLink: connect" << endpointUrl << "failed —" << lastError_;
        destroyClient();
        setState(State::Error);
        emit connectionFailed(lastError_);
        return;
    }

    setState(State::Connected);
    emit connected();

    iterateTimer_->start();

    // Discover namespace mapping and CodeSys application root BEFORE the
    // first browse, so the deep walk can seed itself at the real app node
    // and skip the PLCopen / 3S type-system noise. Both calls are no-ops
    // on failure (will fall back to dumping the full Objects tree).
    readNamespaceArray();
    findApplicationRoot();

    reconfigureSubscriptions();

    // Auto-browse the full app tree on every connect. Result is emitted
    // node-by-node via nodeDiscovered(), logged to journalctl, and also
    // dumped as a human-readable tree to /userfs/smb-q6r/plc-browse.txt.
    doBrowse(8, 5000);
#else
    Q_UNUSED(endpointUrl)
    lastError_ = QStringLiteral("Built without OPC UA support");
    setState(State::Error);
    emit connectionFailed(lastError_);
#endif
}

void PlcLink::doDisconnect()
{
#ifdef SMB_Q6R_HAS_OPCUA
    iterateTimer_->stop();
    destroyClient();
#endif
    setState(State::Disconnected);
    emit disconnected();
}

void PlcLink::destroyClient()
{
#ifdef SMB_Q6R_HAS_OPCUA
    if (!client_) return;
    UA_Client_disconnect(client_);
    UA_Client_delete(client_);
    client_ = nullptr;
    subscriptionId_ = 0;
    monitoredItems_.clear();
    namespaceIndexToUri_.clear();
    appRootNodeId_.clear();
    appNamespaceIndex_ = -1;
    // Clear dynamic monId_* properties used to route DataChange callbacks.
    const auto names = dynamicPropertyNames();
    for (const auto& name : names) {
        if (name.startsWith("monId_")) setProperty(name.constData(), {});
    }
#endif
}

void PlcLink::onIterate()
{
#ifdef SMB_Q6R_HAS_OPCUA
    if (!client_) return;
    const UA_StatusCode rc = UA_Client_run_iterate(client_, 0);
    if (rc != UA_STATUSCODE_GOOD && rc != UA_STATUSCODE_GOODNODATA) {
        lastError_ = QString::fromLatin1(UA_StatusCode_name(rc));
        qWarning() << "PlcLink: iterate error" << lastError_;
        // Transition to Error and tear down — caller can decide to retry.
        iterateTimer_->stop();
        destroyClient();
        setState(State::Error);
        emit connectionFailed(lastError_);
    }
#endif
}

void PlcLink::doRead(QString nodeId)
{
#ifdef SMB_Q6R_HAS_OPCUA
    if (!client_) {
        emit readFailed(nodeId, QStringLiteral("not connected"));
        return;
    }
    bool ok = false;
    UA_NodeId nid = parseNodeId(nodeId, &ok);
    if (!ok) {
        emit readFailed(nodeId, QStringLiteral("bad node id format"));
        return;
    }
    UA_Variant value;
    UA_Variant_init(&value);
    const UA_StatusCode rc = UA_Client_readValueAttribute(client_, nid, &value);
    UA_NodeId_clear(&nid);
    if (rc != UA_STATUSCODE_GOOD) {
        UA_Variant_clear(&value);
        emit readFailed(nodeId, QString::fromLatin1(UA_StatusCode_name(rc)));
        return;
    }
    emit valueRead(nodeId, variantFromUa(value));
    UA_Variant_clear(&value);
#else
    emit readFailed(nodeId, QStringLiteral("Built without OPC UA support"));
#endif
}

void PlcLink::doSubscribe(QString nodeId)
{
#ifdef SMB_Q6R_HAS_OPCUA
    if (state_ != State::Connected) {
        // Defer until we're back online; reconfigureSubscriptions() handles
        // the actual creation after each Connected transition.
        if (!pendingSubscriptions_.contains(nodeId))
            pendingSubscriptions_.append(nodeId);
        return;
    }
    if (!pendingSubscriptions_.contains(nodeId))
        pendingSubscriptions_.append(nodeId);
    reconfigureSubscriptions();
#else
    Q_UNUSED(nodeId)
#endif
}

void PlcLink::doWrite(QString nodeId, QVariant value)
{
#ifdef SMB_Q6R_HAS_OPCUA
    if (!client_ || state_ != State::Connected) {
        emit writeFailed(nodeId, QStringLiteral("not connected"));
        return;
    }
    bool ok = false;
    UA_NodeId nid = parseNodeId(nodeId, &ok);
    if (!ok) {
        emit writeFailed(nodeId, QStringLiteral("bad node id format"));
        return;
    }

    // Cover the variant types CodeSys symbol exports actually expose for
    // ReadWrite scalars: bool, int (DINT in PLC), double (LREAL),
    // string. Anything else fails fast — fancier conversions can come
    // when we have a concrete use case.
    UA_Variant uv;
    UA_Variant_init(&uv);

    UA_Boolean ub; UA_Int32 ui32; UA_Double ud; UA_String ustr;
    switch (static_cast<QMetaType::Type>(value.userType())) {
        case QMetaType::Bool:
            ub = value.toBool();
            UA_Variant_setScalar(&uv, &ub, &UA_TYPES[UA_TYPES_BOOLEAN]);
            break;
        case QMetaType::Int:
        case QMetaType::UInt:
        case QMetaType::LongLong:
            ui32 = static_cast<UA_Int32>(value.toInt());
            UA_Variant_setScalar(&uv, &ui32, &UA_TYPES[UA_TYPES_INT32]);
            break;
        case QMetaType::Double:
        case QMetaType::Float:
            ud = value.toDouble();
            UA_Variant_setScalar(&uv, &ud, &UA_TYPES[UA_TYPES_DOUBLE]);
            break;
        case QMetaType::QString: {
            const QByteArray bytes = value.toString().toUtf8();
            ustr.length = static_cast<size_t>(bytes.size());
            ustr.data = const_cast<UA_Byte*>(
                reinterpret_cast<const UA_Byte*>(bytes.constData()));
            UA_Variant_setScalar(&uv, &ustr, &UA_TYPES[UA_TYPES_STRING]);
            const UA_StatusCode rc = UA_Client_writeValueAttribute(client_, nid, &uv);
            UA_NodeId_clear(&nid);
            if (rc == UA_STATUSCODE_GOOD) emit writeSucceeded(nodeId);
            else emit writeFailed(nodeId, QString::fromLatin1(UA_StatusCode_name(rc)));
            return;
        }
        default:
            UA_NodeId_clear(&nid);
            emit writeFailed(nodeId,
                QStringLiteral("unsupported QVariant type: %1").arg(value.typeName()));
            return;
    }

    const UA_StatusCode rc = UA_Client_writeValueAttribute(client_, nid, &uv);
    UA_NodeId_clear(&nid);
    if (rc == UA_STATUSCODE_GOOD) {
        emit writeSucceeded(nodeId);
    } else {
        emit writeFailed(nodeId, QString::fromLatin1(UA_StatusCode_name(rc)));
    }
#else
    Q_UNUSED(nodeId)
    Q_UNUSED(value)
    emit writeFailed(nodeId, QStringLiteral("Built without OPC UA support"));
#endif
}

void PlcLink::readNamespaceArray()
{
#ifdef SMB_Q6R_HAS_OPCUA
    if (!client_) return;
    namespaceIndexToUri_.clear();

    UA_Variant v;
    UA_Variant_init(&v);
    const UA_StatusCode rc = UA_Client_readValueAttribute(
        client_, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_NAMESPACEARRAY), &v);
    if (rc != UA_STATUSCODE_GOOD) {
        qWarning() << "PlcLink: NamespaceArray read failed"
                   << UA_StatusCode_name(rc);
        UA_Variant_clear(&v);
        return;
    }
    if (UA_Variant_isScalar(&v) || v.type != &UA_TYPES[UA_TYPES_STRING]) {
        qWarning() << "PlcLink: NamespaceArray unexpected type";
        UA_Variant_clear(&v);
        return;
    }

    const auto* arr = static_cast<const UA_String*>(v.data);
    for (size_t i = 0; i < v.arrayLength; ++i) {
        const QString uri = QString::fromUtf8(
            reinterpret_cast<const char*>(arr[i].data),
            static_cast<int>(arr[i].length));
        namespaceIndexToUri_.insert(static_cast<int>(i), uri);
        qInfo().noquote() << QString::asprintf(
            "PlcLink ns[%2d]  %s", static_cast<int>(i), qPrintable(uri));
    }
    UA_Variant_clear(&v);
#endif
}

void PlcLink::findApplicationRoot()
{
#ifdef SMB_Q6R_HAS_OPCUA
    if (!client_) return;
    appRootNodeId_.clear();
    appNamespaceIndex_ = -1;

    // Bootstrap BFS from Objects looking for the first string-identifier
    // node that starts with "|var|" and ends with ".Application". This is
    // the CodeSys convention; if the server doesn't expose it (non-CodeSys
    // backend) the deep browse later just falls back to walking Objects.
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

    while (!queue.isEmpty() && visited < maxVisited && appRootNodeId_.isEmpty()) {
        Pending p = queue.takeFirst();

        UA_BrowseRequest req;
        UA_BrowseRequest_init(&req);
        req.requestedMaxReferencesPerNode = 0;
        req.nodesToBrowse = UA_BrowseDescription_new();
        req.nodesToBrowseSize = 1;
        UA_NodeId_copy(&p.id, &req.nodesToBrowse[0].nodeId);
        req.nodesToBrowse[0].browseDirection = UA_BROWSEDIRECTION_FORWARD;
        req.nodesToBrowse[0].includeSubtypes = true;
        req.nodesToBrowse[0].referenceTypeId = UA_NODEID_NUMERIC(0, 0);
        req.nodesToBrowse[0].nodeClassMask   = 0;
        req.nodesToBrowse[0].resultMask      = UA_BROWSERESULTMASK_BROWSENAME;

        UA_BrowseResponse rsp = UA_Client_Service_browse(client_, req);
        if (rsp.responseHeader.serviceResult == UA_STATUSCODE_GOOD
            && rsp.resultsSize > 0) {
            const UA_BrowseResult& r = rsp.results[0];
            for (size_t i = 0; i < r.referencesSize
                 && visited < maxVisited && appRootNodeId_.isEmpty(); ++i) {
                const UA_ReferenceDescription& ref = r.references[i];

                // Skip standard OPC UA server hierarchy (ns=0 numeric:
                // Server, Aliases, NamespaceArray, type definitions...).
                // The CodeSys application always lives in a non-zero
                // namespace under DeviceSet, and excluding these keeps
                // our bootstrap budget focused on the path that matters.
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
                        appNamespaceIndex_ = ref.nodeId.nodeId.namespaceIndex;
                        appRootNodeId_ = QStringLiteral("ns=%1;s=%2")
                            .arg(appNamespaceIndex_).arg(s);
                        qInfo().noquote() << "PlcLink: app root ->"
                                          << appRootNodeId_;
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

    if (appRootNodeId_.isEmpty()) {
        qWarning() << "PlcLink: no |var|...Application node found —"
                   << "falling back to Objects browse";
    }
#endif
}

void PlcLink::doBrowse(int maxDepth, int maxNodes)
{
#ifdef SMB_Q6R_HAS_OPCUA
    if (!client_ || state_ != State::Connected) {
        qWarning() << "PlcLink::browse — not connected";
        return;
    }

    // Collected entries get rendered to /userfs/smb-q6r/plc-browse.txt at
    // the end. childrenByParent indexes into the entries list so the tree
    // renderer can walk the structure without re-traversing the server.
    QList<BrowseEntry> entries;
    QHash<QString, QList<int>> childrenByParent;

    // Choose seed: discovered application root if we have it, otherwise
    // the standard Objects folder. The latter dumps everything (PLCopen
    // types + 3S vendor space) — fine as a debugging fallback, slow.
    UA_NodeId seedId;
    UA_NodeId_init(&seedId);
    QString seedIdStr;
    QString seedName;
    if (!appRootNodeId_.isEmpty()) {
        bool ok = false;
        seedId = parseNodeId(appRootNodeId_, &ok);
        if (!ok) {
            qWarning() << "PlcLink browse: bad appRootNodeId_"
                       << appRootNodeId_;
            return;
        }
        seedIdStr = appRootNodeId_;
        seedName  = QStringLiteral("Application");
    } else {
        UA_NodeId tmp = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
        UA_NodeId_copy(&tmp, &seedId);
        seedIdStr = QStringLiteral("ns=0;i=85");
        seedName  = QStringLiteral("Objects");
    }

    entries.append({QString(), seedIdStr, seedName,
                    QStringLiteral("Object"), 0});

    struct Pending { UA_NodeId id; QString idStr; int depth; };
    QList<Pending> queue;
    {
        UA_NodeId copy; UA_NodeId_init(&copy);
        UA_NodeId_copy(&seedId, &copy);
        queue.append({copy, seedIdStr, 0});
    }

    const auto t0 = QDateTime::currentMSecsSinceEpoch();
    int seen = 1;  // the seed itself

    while (!queue.isEmpty() && seen < maxNodes) {
        Pending p = queue.takeFirst();

        UA_BrowseRequest req;
        UA_BrowseRequest_init(&req);
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

        UA_BrowseResponse rsp = UA_Client_Service_browse(client_, req);
        if (rsp.responseHeader.serviceResult == UA_STATUSCODE_GOOD
            && rsp.resultsSize > 0) {
            const UA_BrowseResult& r = rsp.results[0];
            for (size_t i = 0; i < r.referencesSize && seen < maxNodes; ++i) {
                const UA_ReferenceDescription& ref = r.references[i];
                const int childNs = ref.nodeId.nodeId.namespaceIndex;
                const bool isNumeric =
                    ref.nodeId.nodeId.identifierType == UA_NODEIDTYPE_NUMERIC;

                // Filter strategy:
                //   - We always skip ns=0 numeric references (Server,
                //     Diagnostics, type metadata under the standard space).
                //   - When the app namespace was discovered, skip any
                //     reference outside it — this discards the PLCopen
                //     (ns=2) and 3S (ns=3) type systems that dominated
                //     the old 250-node budget.
                //   - When no app ns was discovered we walk everything
                //     so the dump still produces a useful overview.
                if (childNs == 0 && isNumeric) continue;
                if (appNamespaceIndex_ >= 0 && childNs != appNamespaceIndex_)
                    continue;

                const QString id = nodeIdToString(ref.nodeId.nodeId);
                const QString name = QString::fromUtf8(
                    reinterpret_cast<const char*>(ref.browseName.name.data),
                    static_cast<int>(ref.browseName.name.length));
                const QString cls = nodeClassToString(ref.nodeClass);

                entries.append({p.idStr, id, name, cls, p.depth + 1});
                childrenByParent[p.idStr].append(entries.size() - 1);
                emit nodeDiscovered(id, name, p.depth + 1);
                ++seen;

                if (p.depth + 1 < maxDepth) {
                    UA_NodeId copy; UA_NodeId_init(&copy);
                    UA_NodeId_copy(&ref.nodeId.nodeId, &copy);
                    queue.append({copy, id, p.depth + 1});
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
    UA_NodeId_clear(&seedId);

    const qint64 elapsedMs = QDateTime::currentMSecsSinceEpoch() - t0;
    qInfo() << "PlcLink: browse complete —" << seen << "nodes in"
            << elapsedMs << "ms";

    writeBrowseDump(entries, childrenByParent, elapsedMs);
#else
    Q_UNUSED(maxDepth)
    Q_UNUSED(maxNodes)
#endif
}

void PlcLink::writeBrowseDump(const QList<BrowseEntry>& entries,
                              const QHash<QString, QList<int>>& childrenByParent,
                              qint64 elapsedMs)
{
#ifdef SMB_Q6R_HAS_OPCUA
    // Primary target — persistent flash partition next to the binary.
    // Fall back to /tmp when /userfs is read-only (host dev box).
    const QString primary = QStringLiteral("/userfs/smb-q6r");
    QString dir = primary;
    if (!QDir().mkpath(dir)) {
        qWarning() << "PlcLink: cannot mkpath" << dir
                   << "— falling back to /tmp";
        dir = QStringLiteral("/tmp");
    }
    const QString path = dir + QStringLiteral("/plc-browse.txt");

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        qWarning() << "PlcLink: open dump file" << path << "failed —"
                   << f.errorString();
        return;
    }
    QTextStream out(&f);
    out.setCodec("UTF-8");

    // Wrap every literal that contains non-ASCII glyphs in QStringLiteral.
    // Qt 5.15's operator<<(QTextStream&, const char*) decodes the bytes
    // using the local-8bit codec even when setCodec("UTF-8") is active, so
    // raw "═" literals in a UTF-8 source file end up double-encoded on
    // disk (E2 95 90 → C3 A2 C2 95 C2 90). QStringLiteral is materialised
    // as a UTF-16 QString at compile time and round-trips cleanly through
    // the writer's codec.
    const QString headerBar = QStringLiteral(
        "═══════════════════════════════════════════════════════════════");
    const QString nsRule    = QStringLiteral("───────────────");
    const QString treeRule  = QStringLiteral("────────────────");
    const QString sumRule   = QStringLiteral("───────");
    const QString appMark   = QStringLiteral("  ◄ application");

    // Header
    const QString ts = QDateTime::currentDateTime()
        .toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    out << headerBar << "\n";
    out << "  SMB-Q6R PLC Browse Snapshot\n";
    out << "  Generated: " << ts << "\n";
    out << "  Endpoint:  " << serverUrl_ << "\n";
    if (!appRootNodeId_.isEmpty())
        out << "  App root:  " << appRootNodeId_ << "\n";
    out << headerBar << "\n\n";

    // Namespace table
    out << "NAMESPACE TABLE\n";
    out << nsRule << "\n";
    QList<int> indices = namespaceIndexToUri_.keys();
    std::sort(indices.begin(), indices.end());
    for (int idx : indices) {
        const QString marker = (idx == appNamespaceIndex_) ? appMark : QString();
        out << QStringLiteral("  ns=%1  %2%3\n")
                .arg(idx, -3)
                .arg(namespaceIndexToUri_.value(idx))
                .arg(marker);
    }
    out << "\n";

    // Tree
    out << "APPLICATION TREE\n";
    out << treeRule << "\n";
    if (!entries.isEmpty()) {
        renderTreeNode(out, entries, childrenByParent, 0, QString(), QString());
    }
    out << "\n";

    // Summary
    int varCount = 0, objCount = 0, methodCount = 0, otherCount = 0;
    for (const BrowseEntry& e : entries) {
        if      (e.nodeClass == QStringLiteral("Variable")) ++varCount;
        else if (e.nodeClass == QStringLiteral("Object"))   ++objCount;
        else if (e.nodeClass == QStringLiteral("Method"))   ++methodCount;
        else                                                ++otherCount;
    }
    out << "SUMMARY\n";
    out << sumRule << "\n";
    out << "  Total nodes:  " << entries.size() << "\n";
    out << "  Variables:    " << varCount    << "\n";
    out << "  Objects:      " << objCount    << "\n";
    out << "  Methods:      " << methodCount << "\n";
    if (otherCount > 0)
        out << "  Other:        " << otherCount << "\n";
    out << "  Namespaces:   " << namespaceIndexToUri_.size() << "\n";
    out << "  Browse time:  " << elapsedMs << " ms\n";

    f.close();
    qInfo().noquote() << "PlcLink: dump ->" << path
                      << QStringLiteral("(%1 nodes, %2 ms)")
                            .arg(entries.size()).arg(elapsedMs);

    // Sidecar machine-readable JSON next to the human dump. Consumed by
    // tools/symbol-picker/ to render the tree without re-parsing the
    // glyph-laden text. Same atomic semantics: open WriteOnly|Truncate.
    const QString jsonPath = dir + QStringLiteral("/plc-browse.json");
    QFile jf(jsonPath);
    if (!jf.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "PlcLink: open json sidecar" << jsonPath << "failed —"
                   << jf.errorString();
        return;
    }
    QJsonObject root;
    root.insert(QStringLiteral("generated"),
                QDateTime::currentDateTime().toString(Qt::ISODate));
    root.insert(QStringLiteral("endpoint"),         serverUrl_);
    root.insert(QStringLiteral("appRootNodeId"),    appRootNodeId_);
    root.insert(QStringLiteral("appNamespaceIndex"), appNamespaceIndex_);
    root.insert(QStringLiteral("browseTimeMs"),     elapsedMs);

    QJsonArray nsArr;
    QList<int> nsIndices = namespaceIndexToUri_.keys();
    std::sort(nsIndices.begin(), nsIndices.end());
    for (int idx : nsIndices) {
        QJsonObject ns;
        ns.insert(QStringLiteral("index"), idx);
        ns.insert(QStringLiteral("uri"),   namespaceIndexToUri_.value(idx));
        nsArr.append(ns);
    }
    root.insert(QStringLiteral("namespaces"), nsArr);

    QJsonArray nodesArr;
    for (const BrowseEntry& e : entries) {
        QJsonObject n;
        n.insert(QStringLiteral("id"),       e.nodeId);
        n.insert(QStringLiteral("parentId"), e.parentNodeId);
        n.insert(QStringLiteral("name"),     e.browseName);
        n.insert(QStringLiteral("class"),    e.nodeClass);
        n.insert(QStringLiteral("depth"),    e.depth);
        nodesArr.append(n);
    }
    root.insert(QStringLiteral("nodes"), nodesArr);

    // Compact form keeps the file small (5000-node dump is ~600 KB).
    jf.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    jf.close();
    qInfo().noquote() << "PlcLink: json ->" << jsonPath;
#else
    Q_UNUSED(entries)
    Q_UNUSED(childrenByParent)
    Q_UNUSED(elapsedMs)
#endif
}

void PlcLink::reconfigureSubscriptions()
{
#ifdef SMB_Q6R_HAS_OPCUA
    if (!client_ || state_ != State::Connected) return;

    // Create the single shared subscription if we don't yet have one.
    if (subscriptionId_ == 0) {
        UA_CreateSubscriptionRequest req = UA_CreateSubscriptionRequest_default();
        UA_CreateSubscriptionResponse rsp =
            UA_Client_Subscriptions_create(client_, req, nullptr, nullptr, nullptr);
        if (rsp.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
            qWarning() << "PlcLink: create subscription failed"
                       << UA_StatusCode_name(rsp.responseHeader.serviceResult);
            return;
        }
        subscriptionId_ = rsp.subscriptionId;
    }

    // Create a MonitoredItem for each pending node id, then move it to
    // monitoredItems_. If the server rejects one (bad node) we skip it
    // and continue with the rest.
    while (!pendingSubscriptions_.isEmpty()) {
        const QString id = pendingSubscriptions_.takeFirst();
        bool ok = false;
        UA_NodeId nid = parseNodeId(id, &ok);
        if (!ok) {
            qWarning() << "PlcLink: subscribe bad node id" << id;
            continue;
        }
        UA_MonitoredItemCreateRequest req =
            UA_MonitoredItemCreateRequest_default(nid);
        UA_MonitoredItemCreateResult res =
            UA_Client_MonitoredItems_createDataChange(client_,
                subscriptionId_, UA_TIMESTAMPSTORETURN_BOTH,
                req, this, &dataChangeTrampoline, nullptr);
        UA_NodeId_clear(&nid);
        if (res.statusCode != UA_STATUSCODE_GOOD) {
            qWarning() << "PlcLink: monitor" << id << "failed"
                       << UA_StatusCode_name(res.statusCode);
            continue;
        }
        monitoredItems_.insert(res.monitoredItemId, id);
        // Stash the mapping as a dynamic property so the data-change
        // trampoline can recover the QString without a thread-protected
        // map lookup (trampoline runs from inside run_iterate on the
        // worker thread, but Qt's QHash is not safe to mutate concurrently
        // with reads on the same thread either — we only read here).
        setProperty(("monId_" + QByteArray::number(res.monitoredItemId)).constData(), id);
    }
#endif
}

} // namespace smbq6r
