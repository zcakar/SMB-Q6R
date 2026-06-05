#include "SymbolsJsonWriter.h"

#include "BrowseJsonLoader.h"

#include <QDateTime>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>

namespace {

// Build the human-readable path from a node up to (but not including)
// the Application root. Resulting form: "GVL.Enable" — the codegen
// then dot-lowercases the leading segment for the logical name.
QString pathFromAppRoot(const BrowseNode* leaf)
{
    QStringList rev;
    const BrowseNode* n = leaf;
    while (n && n->parent) {
        rev.prepend(n->name);
        // Stop at the Application root itself — its parent is empty.
        // We don't want "Application." in every path.
        if (!n->parent->parent) break;
        n = n->parent;
    }
    return rev.join(QLatin1Char('.'));
}

// "GVL.Enable" → "gvl.enable" — lower-case the segments so the codegen
// can mechanically map to camelCase ids without ambiguity.
QString logicalName(const QString& path)
{
    QStringList segs = path.split(QLatin1Char('.'));
    for (QString& s : segs) s = s.toLower();
    return segs.join(QLatin1Char('.'));
}

} // namespace

bool SymbolsJsonWriter::write(const QString& path,
                              const BrowseRoot* root,
                              const QString& sourceLabel,
                              const QList<const BrowseNode*>& leaves)
{
    lastError_.clear();
    if (!root) {
        lastError_ = QStringLiteral("null browse root");
        return false;
    }

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        lastError_ = QStringLiteral("open %1 failed — %2")
                         .arg(path, f.errorString());
        return false;
    }

    QJsonObject doc;
    doc.insert(QStringLiteral("generated"),
               QDateTime::currentDateTime().toString(Qt::ISODate));
    doc.insert(QStringLiteral("source"),       sourceLabel);
    doc.insert(QStringLiteral("namespaceUri"), root->namespaceUri);
    doc.insert(QStringLiteral("appRootNodeId"),root->appRootNodeId);

    QJsonArray syms;
    for (const BrowseNode* n : leaves) {
        const QString p = pathFromAppRoot(n);
        QJsonObject s;
        s.insert(QStringLiteral("name"),   logicalName(p));
        s.insert(QStringLiteral("path"),   p);
        s.insert(QStringLiteral("nodeId"), n->id);
        s.insert(QStringLiteral("class"),  n->nodeClass);
        syms.append(s);
    }
    doc.insert(QStringLiteral("symbols"), syms);

    // Pretty-printed: symbols.json is small (typically <100 entries)
    // and lives in git — readability outweighs file-size.
    f.write(QJsonDocument(doc).toJson(QJsonDocument::Indented));
    return true;
}
