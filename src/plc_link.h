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

    // One node in the snapshot built by doBrowse() and rendered into the
    // /userfs/smb-q6r/plc-browse.txt tree. Public so the anonymous-namespace
    // tree renderer in plc_link.cpp can take it by reference.
    struct BrowseEntry {
        QString parentNodeId;   // canonical "ns=N;..." of parent, "" for seed
        QString nodeId;         // canonical "ns=N;..." of this node
        QString browseName;
        QString nodeClass;      // "Object" / "Variable" / "Method" / ...
        int     depth = 0;      // 0 = seed, 1 = its children, etc.
    };

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

    // Write a value to a node. The QVariant's metatype decides the OPC UA
    // type sent on the wire: bool → Boolean, int → Int32, double → Double,
    // QString → String. Anything else falls back to a writeFailed signal.
    void writeNode(const QString& nodeId, const QVariant& value);

    // Recursively browse the server namespace and emit nodeDiscovered()
    // for each node found, up to maxNodes total and maxDepth levels
    // deep. The browse seeds itself from the auto-discovered CodeSys
    // application root (|var|<Device>.Application) when possible, falling
    // back to the standard Objects folder otherwise. On every successful
    // browse the full tree is also dumped to /userfs/smb-q6r/plc-browse.txt
    // in a human-readable form (namespace table + indented tree).
    void browseNamespace(int maxDepth = 8, int maxNodes = 5000);

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

    void writeSucceeded(QString nodeId);
    void writeFailed(QString nodeId, QString reason);

private slots:
    // Worker-thread slot: pump the open62541 client event loop.
    void onIterate();
    // Worker-thread slot: kick off an actual connect on the right thread.
    void doConnect(QString endpointUrl);
    void doDisconnect();
    void doRead(QString nodeId);
    void doSubscribe(QString nodeId);
    void doBrowse(int maxDepth, int maxNodes);
    void doWrite(QString nodeId, QVariant value);

private:
    static UA_Client* makeClient();
    void setState(State s);
    void reconfigureSubscriptions();   // recreate after reconnect
    void destroyClient();

    // Post-connect bootstrap: populates namespaceIndexToUri_ from the
    // standard Server_NamespaceArray node (ns=0;i=2255). Required because
    // namespace indices are server-assigned and may shift between CodeSys
    // runtime / library configurations.
    void readNamespaceArray();

    // Short browse from the Objects folder that locates the first node
    // whose string identifier matches "|var|...Application". Sets
    // appRootNodeId_ and appNamespaceIndex_ on success so the deep
    // browse can seed itself there and skip the PLCopen type system.
    void findApplicationRoot();

    // Render the browse snapshot to /userfs/smb-q6r/plc-browse.txt with a
    // header, namespace table, indented unicode-branch tree, and summary
    // counts. Called at the tail of every successful doBrowse().
    void writeBrowseDump(const QList<BrowseEntry>& entries,
                        const QHash<QString, QList<int>>& childrenByParent,
                        qint64 elapsedMs);

    QThread* workerThread_ = nullptr;
    QTimer*  iterateTimer_ = nullptr;

    UA_Client* client_ = nullptr;
    State   state_     = State::Disconnected;
    QString serverUrl_;
    QString lastError_;

    // Server namespace table — populated on connect, cleared on disconnect.
    QHash<int, QString> namespaceIndexToUri_;

    // CodeSys application root discovered post-connect. Empty if we
    // couldn't find a |var|...Application node — browse falls back to
    // the standard Objects folder in that case.
    QString appRootNodeId_;
    int     appNamespaceIndex_ = -1;

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
