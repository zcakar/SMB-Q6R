#pragma once

#include <QString>

#include "BrowseJsonLoader.h"   // BrowseRoot / BrowseNode

// Offline source that parses a CodeSys Symbol Configuration XML
// (the file produced by Build → Symbol Configuration in the CodeSys IDE).
// Element shape:
//
//   <Node name="Application" nodeflags="NodeTypeBranchNode BranchNodeApplicationName">
//     <Node name="GVL" nodeflags="NodeTypeBranchNode SigTypeGvl">
//       <Node name="Enable" nodeflags="SigTypeGvl ExportedVariable"
//             type="T_BOOL" access="ReadWrite" />
//
// The XML doesn't carry NodeIds — they are constructed at runtime as
//   ns=?;s=|var|<DeviceName>.Application.<Path>
// We can't recover the namespace index from the XML; the picker writes
// "ns=?" placeholder strings for now and codegen rewrites them when the
// live PLC tells us which index Application sits at.
//
// |DeviceName| comes from the XML's <ProjectInfo devicename="..."/> but
// CodeSys IDE rename may have shifted it (the XML stays at the original
// name). The picker lets the user override this to match the live PLC.
class CodesysXmlSource
{
public:
    CodesysXmlSource(QString xmlPath, QString deviceNameOverride,
                     int assumedNamespaceIndex);

    BrowseRoot* load();
    QString lastError() const { return lastError_; }
    QString sourceLabel() const { return xmlPath_; }

private:
    QString xmlPath_;
    QString deviceNameOverride_;
    int     assumedNamespaceIndex_;
    QString lastError_;
};
