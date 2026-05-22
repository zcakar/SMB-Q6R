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

void PlcLink::doConnect(QString endpointUrl)
{
#ifdef SMB_Q6R_HAS_OPCUA
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
