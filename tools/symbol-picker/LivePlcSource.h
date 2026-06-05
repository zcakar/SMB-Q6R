#pragma once

#include <QString>

#include "BrowseJsonLoader.h"   // for BrowseRoot / BrowseNode definitions

// Direct OPC UA browse against a live PLC. Runs synchronously: connects,
// reads NamespaceArray, locates the |var|...Application root, BFS-browses
// the application subtree, then disconnects. Returns the same BrowseRoot
// shape JsonDumpSource does so PickerWindow can swap them transparently.
//
// Use only from a worker thread — even though synchronous, a busy PLC
// makes the network round-trip take several seconds and we don't want to
// freeze the Qt event loop.
class LivePlcSource
{
public:
    explicit LivePlcSource(QString endpointUrl);

    BrowseRoot* load();
    QString lastError() const { return lastError_; }
    QString sourceLabel() const { return endpointUrl_; }

    // Cap mirrors PlcLink::doBrowse: deep enough to reach every leaf
    // under GVL/GVL_Control_Var/IoConfig_Globals_Mapping; bounded so a
    // misconfigured server can't run away.
    static constexpr int kMaxDepth = 8;
    static constexpr int kMaxNodes = 5000;

private:
    QString endpointUrl_;
    QString lastError_;
};
