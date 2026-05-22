#include "plc_link.h"

#include <QThread>
#include <QTimer>
#include <QDebug>
#include <QMetaObject>

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
    reconfigureSubscriptions();

    // Auto-browse: dump the discovered tree to journalctl so the dev side
    // can copy/paste node IDs into the symbol bindings. Cheap to do once
    // per connect; the recursion is depth-capped.
    doBrowse(5, 250);
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

void PlcLink::doBrowse(int maxDepth, int maxNodes)
{
#ifdef SMB_Q6R_HAS_OPCUA
    if (!client_ || state_ != State::Connected) {
        qWarning() << "PlcLink::browse — not connected";
        return;
    }

    // Iterative BFS so we can cap total nodes without recursion blow-up
    // on a malformed server. Each queue entry pairs a node we'll browse
    // with its depth (0 = direct child of Objects).
    struct Pending { UA_NodeId id; int depth; };
    QList<Pending> queue;

    // Seed: the standard Objects folder, ns=0;i=85.
    queue.append({UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), -1});

    int seen = 0;
    while (!queue.isEmpty() && seen < maxNodes) {
        const Pending p = queue.takeFirst();

        UA_BrowseRequest req;
        UA_BrowseRequest_init(&req);
        req.requestedMaxReferencesPerNode = 0;
        req.nodesToBrowse = UA_BrowseDescription_new();
        req.nodesToBrowseSize = 1;
        // Deep-copy p.id INTO the request so the request owns its own
        // memory. Shallow-assigning shared string identifiers between
        // p.id and req.nodesToBrowse[0].nodeId caused a double-free when
        // both got UA_NodeId_clear()'d at the end of the loop.
        UA_NodeId_copy(&p.id, &req.nodesToBrowse[0].nodeId);
        req.nodesToBrowse[0].browseDirection = UA_BROWSEDIRECTION_FORWARD;
        req.nodesToBrowse[0].includeSubtypes = true;
        // 0 = "any reference type" — give us organises/hasComponent/hasProperty
        req.nodesToBrowse[0].referenceTypeId = UA_NODEID_NUMERIC(0, 0);
        req.nodesToBrowse[0].nodeClassMask  = 0;   // any class
        req.nodesToBrowse[0].resultMask =
            UA_BROWSERESULTMASK_NODECLASS | UA_BROWSERESULTMASK_BROWSENAME;

        UA_BrowseResponse rsp = UA_Client_Service_browse(client_, req);

        if (rsp.responseHeader.serviceResult == UA_STATUSCODE_GOOD
            && rsp.resultsSize > 0) {
            const UA_BrowseResult& r = rsp.results[0];
            for (size_t i = 0; i < r.referencesSize && seen < maxNodes; ++i) {
                const UA_ReferenceDescription& ref = r.references[i];
                // Skip references back to the standard type hierarchy —
                // CodeSys exposes those under Objects, but we only care
                // about variables, GVL nodes, and the application tree.
                if (ref.nodeId.nodeId.namespaceIndex == 0
                    && ref.nodeId.nodeId.identifierType == UA_NODEIDTYPE_NUMERIC
                    && ref.nodeId.nodeId.identifier.numeric < 100) {
                    continue;
                }
                const QString id = nodeIdToString(ref.nodeId.nodeId);
                const QString name = QString::fromUtf8(
                    reinterpret_cast<const char*>(ref.browseName.name.data),
                    static_cast<int>(ref.browseName.name.length));
                qInfo().noquote() << QString::asprintf(
                    "PlcLink browse %2d  %-40s  %s",
                    p.depth + 1, qPrintable(name), qPrintable(id));
                emit nodeDiscovered(id, name, p.depth + 1);
                ++seen;

                if (p.depth + 1 < maxDepth) {
                    UA_NodeId copy;
                    UA_NodeId_init(&copy);
                    UA_NodeId_copy(&ref.nodeId.nodeId, &copy);
                    queue.append({copy, p.depth + 1});
                }
            }
        }

        // p.id was either the seeded Objects literal (no heap) or a copy
        // we made above. Clearing both shapes is safe.
        UA_NodeId_clear(const_cast<UA_NodeId*>(&p.id));
        UA_BrowseResponse_clear(&rsp);
        UA_BrowseRequest_clear(&req);
    }

    qInfo() << "PlcLink: browse complete —" << seen << "nodes emitted";
    // Drain any leftover pending entries' heap (depth-limited tree may
    // still have unvisited children when seen >= maxNodes).
    while (!queue.isEmpty()) {
        Pending p = queue.takeFirst();
        UA_NodeId_clear(&p.id);
    }
#else
    Q_UNUSED(maxDepth)
    Q_UNUSED(maxNodes)
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
