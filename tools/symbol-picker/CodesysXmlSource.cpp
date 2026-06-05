#include "CodesysXmlSource.h"

#include <QFile>
#include <QXmlStreamReader>

namespace {

// Map CodeSys IEC type tokens (T_BOOL, T_INT, ...) to short DataType
// labels we surface in the picker's class column. Anything we don't
// recognise stays as-is so the user still sees something useful.
QString shortTypeName(const QString& t)
{
    if (t.isEmpty()) return {};
    QString s = t;
    if (s.startsWith(QStringLiteral("T_"))) s.remove(0, 2);
    return s;
}

} // namespace

CodesysXmlSource::CodesysXmlSource(QString xmlPath,
                                   QString deviceNameOverride,
                                   int assumedNamespaceIndex)
    : xmlPath_(std::move(xmlPath)),
      deviceNameOverride_(std::move(deviceNameOverride)),
      assumedNamespaceIndex_(assumedNamespaceIndex) {}

BrowseRoot* CodesysXmlSource::load()
{
    lastError_.clear();

    QFile f(xmlPath_);
    if (!f.open(QIODevice::ReadOnly)) {
        lastError_ = QStringLiteral("cannot open %1 — %2")
                         .arg(xmlPath_, f.errorString());
        return nullptr;
    }

    QString deviceName = deviceNameOverride_;  // takes precedence

    QXmlStreamReader xml(&f);
    // We need two passes-worth of state in one streaming pass:
    // 1. find <ProjectInfo devicename=.../> for the path prefix
    // 2. recursively materialise the <Node> tree
    // We do both in the same loop using a stack of in-progress nodes.

    auto* out = new BrowseRoot;
    out->endpoint           = xmlPath_;
    out->appNamespaceIndex  = assumedNamespaceIndex_;
    out->namespaceUri       = QStringLiteral("(from CodeSys XML — verify against live PLC)");

    QList<BrowseNode*> stack;   // current ancestor chain inside <Node> elements

    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.hasError()) {
            lastError_ = QStringLiteral("xml parse error at line %1: %2")
                             .arg(xml.lineNumber()).arg(xml.errorString());
            delete out;
            return nullptr;
        }

        if (xml.isStartElement()) {
            const QStringRef tag = xml.name();

            if (tag == QStringLiteral("ProjectInfo")) {
                if (deviceName.isEmpty()) {
                    deviceName = xml.attributes()
                        .value(QStringLiteral("devicename")).toString();
                }
                continue;
            }

            if (tag == QStringLiteral("Node")) {
                const QXmlStreamAttributes attrs = xml.attributes();
                const QString name = attrs.value(QStringLiteral("name")).toString();
                const QString type = attrs.value(QStringLiteral("type")).toString();
                const QString flags = attrs.value(QStringLiteral("nodeflags")).toString();

                // "ExportedVariable" flag = leaf, "NodeTypeBranchNode" = container.
                // We use children-presence at the XML level to decide which
                // node class to label them with.
                const bool isBranch = flags.contains(QStringLiteral("NodeTypeBranchNode"));

                auto* node = new BrowseNode;
                node->name      = name;
                node->depth     = stack.size();
                node->nodeClass = isBranch ? QStringLiteral("Object")
                    : (type.isEmpty() ? QStringLiteral("Variable")
                                      : shortTypeName(type));

                // Build the path-derived NodeId.
                // The Application node itself is the seed; for it we emit
                // the canonical "|var|<Device>.Application" form. For its
                // descendants we accumulate the path under it.
                QStringList pathParts;
                for (BrowseNode* p : stack) {
                    if (p->name == QStringLiteral("Application")) {
                        pathParts.clear();   // Application is the path origin
                        continue;
                    }
                    if (!pathParts.isEmpty() || !p->name.isEmpty())
                        pathParts.append(p->name);
                }
                pathParts.append(name);

                // Strip "Application" from the head — it goes into the
                // |var|<Device>.Application prefix instead.
                QString tailPath = pathParts.join(QLatin1Char('.'));
                QString prefix = isBranch ? QStringLiteral("|appo|") : QStringLiteral("|var|");
                if (name == QStringLiteral("Application")) {
                    prefix = QStringLiteral("|var|");
                    tailPath.clear();
                }
                QString idTail = QStringLiteral("%1%2.Application")
                                     .arg(prefix, deviceName);
                if (!tailPath.isEmpty()) idTail += QLatin1Char('.') + tailPath;

                node->id = QStringLiteral("ns=%1;s=%2")
                               .arg(assumedNamespaceIndex_).arg(idTail);

                // Wire to parent or to roots[].
                if (stack.isEmpty()) {
                    out->roots.append(node);
                } else {
                    node->parent     = stack.last();
                    node->parentId   = stack.last()->id;
                    stack.last()->children.append(node);
                }
                out->allNodes.append(node);

                // Application is what we want PickerWindow to surface as
                // the seed; record its node id for SymbolsJsonWriter.
                if (name == QStringLiteral("Application"))
                    out->appRootNodeId = node->id;

                stack.append(node);
                continue;
            }
        }

        if (xml.isEndElement() && xml.name() == QStringLiteral("Node")) {
            if (!stack.isEmpty()) stack.removeLast();
        }
    }

    if (out->roots.isEmpty()) {
        lastError_ = QStringLiteral("no <Node> elements found in XML");
        delete out;
        return nullptr;
    }
    if (out->appRootNodeId.isEmpty()) {
        // Fall back to the first root we created.
        out->appRootNodeId = out->roots.first()->id;
    }
    return out;
}
