#pragma once

#include <QList>
#include <QString>

struct BrowseNode;
struct BrowseRoot;

// Emits the curated symbols.json consumed by Phase-2 codegen. Logical
// "name" field is derived from each selected leaf's path under the app
// root: "Application.GVL.Enable" → "gvl.enable". Codegen then turns
// that into C++ kGvlEnable / QML PlcSymbols.gvl.enable.
class SymbolsJsonWriter
{
public:
    // Write the manifest. Returns true on success; populates lastError()
    // otherwise. Overwrites |path|.
    bool write(const QString& path,
               const BrowseRoot* browseRoot,
               const QString& sourceLabel,
               const QList<const BrowseNode*>& selectedLeaves);

    QString lastError() const { return lastError_; }

private:
    QString lastError_;
};
