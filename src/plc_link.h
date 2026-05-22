#pragma once

#include <QObject>
#include <QString>
#include <QVariant>
#include <QHash>

QT_BEGIN_NAMESPACE
class QThread;
class QTimer;
QT_END_NAMESPACE

// Forward declare so callers don't need to pull in open62541.h.
struct UA_Client;

namespace smbq6r {

// OPC UA client wrapper around open62541. The PlcLink object owns the
// open62541 UA_Client* and runs its single-threaded iteration loop in a
// dedicated worker QThread, so PLC traffic never blocks the UI.
//
// Threading model
// ---------------
// PlcLink is constructed on the main thread and immediately moveToThread'd
// onto its own QThread. ALL slots are invoked through queued connections;
// callers must use signals or QMetaObject::invokeMethod(..., Qt::QueuedConnection)
// — never call the open62541 functions directly. Signals are emitted from
// the worker thread and Qt's auto-connect logic re-queues them onto the
// recipient's thread (UI), so QML bindings stay coherent.
//
// Node IDs
// --------
// CodeSys exposes its symbols in namespace index 4 under a string
// identifier (e.g. "|var|CODESYS.Application.GVL.MyVar"). We accept any
// OPC UA node id string in the form "ns=N;s=STRING" or "ns=N;i=NUMERIC".
class PlcLink : public QObject
{
    Q_OBJECT
public:
    enum class State {
        Disconnected,
        Connecting,
        Connected,
        Reconnecting,
        Error
    };
    Q_ENUM(State)

    explicit PlcLink(QObject* parent = nullptr);
    ~PlcLink() override;

    PlcLink(const PlcLink&) = delete;
    PlcLink& operator=(const PlcLink&) = delete;

    State   state()      const { return state_; }
    QString serverUrl()  const { return serverUrl_; }
    QString lastError()  const { return lastError_; }

public slots:
    // Start an async connect attempt. Safe to call from any thread.
    void connectToServer(const QString& endpointUrl);

    // Tear the session down cleanly. No-op when already disconnected.
    void disconnectFromServer();

    // One-shot read. valueRead() is emitted with the result; readFailed()
    // with the OPC UA status name on error.
    void readNode(const QString& nodeId);

    // Subscribe to value changes. Each change emits valueChanged(nodeId,
    // newValue). Subscriptions persist across server reconnects (the
    // worker re-creates them after a Connected transition).
    void subscribeNode(const QString& nodeId);

    // Recursively browse the server namespace from the Objects folder
    // and emit nodeDiscovered() for each node found, up to maxNodes
    // total and maxDepth levels deep. Used to discover the exact node
    // ID strings CodeSys exposes for the symbol configuration we're
    // pointed at (varies slightly between CODESYS versions).
    void browseNamespace(int maxDepth = 5, int maxNodes = 250);

signals:
    // Lifecycle.
    void stateChanged(smbq6r::PlcLink::State newState);
    void connected();
    void disconnected();
    void connectionFailed(QString reason);

    // Data flow.
    void valueRead(QString nodeId, QVariant value);
    void valueChanged(QString nodeId, QVariant value);
    void readFailed(QString nodeId, QString reason);

    // Emitted once per node visited by browseNamespace(). depth is 0 for
    // the immediate children of Objects, 1 for their children, etc.
    void nodeDiscovered(QString nodeId, QString browseName, int depth);

private slots:
    // Worker-thread slot: pump the open62541 client event loop.
    void onIterate();
    // Worker-thread slot: kick off an actual connect on the right thread.
    void doConnect(QString endpointUrl);
    void doDisconnect();
    void doRead(QString nodeId);
    void doSubscribe(QString nodeId);
    void doBrowse(int maxDepth, int maxNodes);

private:
    static UA_Client* makeClient();
    void setState(State s);
    void reconfigureSubscriptions();   // recreate after reconnect
    void destroyClient();

    QThread* workerThread_ = nullptr;
    QTimer*  iterateTimer_ = nullptr;

    UA_Client* client_ = nullptr;
    State   state_     = State::Disconnected;
    QString serverUrl_;
    QString lastError_;

    // SubscriptionId -> nodeId map, so we can route DataChange callbacks
    // back to a meaningful string for the UI. The integer key is what
    // open62541 hands us when a monitored item is created.
    QHash<quint32, QString> monitoredItems_;

    // Pending subscriptions requested while disconnected — recreated on
    // each successful connect.
    QStringList pendingSubscriptions_;

    // Subscription id we own on the server side (only ever one).
    quint32 subscriptionId_ = 0;
};

} // namespace smbq6r
