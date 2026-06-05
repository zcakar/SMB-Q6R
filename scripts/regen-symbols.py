#!/usr/bin/env python3
"""
Generate plc_symbols.h (C++) and PlcSymbols.qml (QML singleton) from
the curated symbols/symbols.json manifest the picker writes.

Strict filter applied on the way in:
  * only ``class == "Variable"`` entries — Objects/ReferenceType/etc.
    aren't subscribable on the wire
  * only entries whose nodeId carries the ``|var|`` prefix; the picker
    occasionally captures ``|type|`` (CodeSys type definitions) and
    ``|vprop|`` (read-only Variable property meta-data) which we want
    to skip for the runtime symbol table
  * only entries whose logical name maps cleanly to a C++ identifier
    after dot-splitting (no ``#``, no leading digit, etc.)

Outputs are overwritten in-place; both targets are gitignored.
"""
from __future__ import annotations

import argparse
import datetime as _dt
import json
import pathlib
import re
import sys
from collections import OrderedDict
from typing import Any

# C++ keyword set so we never emit a constexpr that shadows one.
_CPP_RESERVED = {
    "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand",
    "bitor", "bool", "break", "case", "catch", "char", "char8_t",
    "char16_t", "char32_t", "class", "compl", "concept", "const",
    "consteval", "constexpr", "constinit", "const_cast", "continue",
    "co_await", "co_return", "co_yield", "decltype", "default", "delete",
    "do", "double", "dynamic_cast", "else", "enum", "explicit", "export",
    "extern", "false", "float", "for", "friend", "goto", "if", "inline",
    "int", "long", "module", "mutable", "namespace", "new", "noexcept",
    "not", "not_eq", "nullptr", "operator", "or", "or_eq", "private",
    "protected", "public", "register", "reinterpret_cast", "requires",
    "return", "short", "signed", "sizeof", "static", "static_assert",
    "static_cast", "struct", "switch", "template", "this", "thread_local",
    "throw", "true", "try", "typedef", "typeid", "typename", "union",
    "unsigned", "using", "virtual", "void", "volatile", "wchar_t",
    "while", "xor", "xor_eq",
}

# QML/JavaScript reserved words for the QML side.
_QML_RESERVED = {
    "abstract", "boolean", "break", "byte", "case", "catch", "char",
    "class", "const", "continue", "debugger", "default", "delete", "do",
    "double", "else", "enum", "export", "extends", "false", "final",
    "finally", "float", "for", "function", "goto", "if", "implements",
    "import", "in", "instanceof", "int", "interface", "let", "long",
    "native", "new", "null", "package", "private", "protected", "public",
    "return", "short", "static", "super", "switch", "synchronized",
    "this", "throw", "throws", "transient", "true", "try", "typeof",
    "var", "void", "volatile", "while", "with", "yield",
}

_IDENT_RE = re.compile(r"^[a-zA-Z_][a-zA-Z0-9_]*$")


def to_camel(seg: str) -> str:
    """Underscore_or_kebab → camelCase. Leading digit gets ``_`` prefix
    so the result is always a valid identifier."""
    parts = re.split(r"[_\-]+", seg)
    head = parts[0]
    tail = "".join(p[:1].upper() + p[1:] for p in parts[1:] if p)
    name = head + tail
    if not name:
        return name
    if name[0].isdigit():
        name = "_" + name
    return name


def safe_ident(seg: str, reserved: set[str]) -> str:
    """Make a single path segment safe for use as a C++ / QML identifier."""
    n = to_camel(seg)
    if n.lower() in reserved:
        n += "_"
    return n


def to_qml_root_ident(seg: str) -> str:
    """QML property declarations (the names following ``readonly
    property var``) must start with a lowercase letter — QML enforces
    this even though JavaScript itself doesn't. We apply two rules to
    the leading letter:
      * pure-acronym tokens (GVL, IO, EMG01) drop to all-lower (gvl, io,
        emg01) — the parser is fine with the all-caps form for nested
        keys but readers expect ``plc.gvl.enable``, not ``plc.GVL.enable``
      * regular PascalCase camel-folds to lowerCamel (GlobalVars →
        globalVars)

    Nested keys (inside the JS object literals we hand the engine) are
    NOT subject to this rule and round-trip the original CodeSys name
    via ``qml_nested_key()`` below, so users see ``GVL.Enable`` and
    ``EMG_01`` exactly as they appear in the symbol configuration.
    """
    n = safe_ident(seg, _QML_RESERVED)
    if not n:
        return n
    if re.fullmatch(r"[A-Z][A-Z0-9]*", n):
        return n.lower()
    return n[0].lower() + n[1:]


def qml_nested_key(seg: str) -> str:
    """A JS object key in the generated QML. CodeSys variable names are
    almost always valid JS identifiers, so we round-trip them as-is —
    that gives users ``PlcSymbols.globalVars.GVL.Enable`` instead of
    ``...gvl.enable``. Falls back to the sanitised PascalCase form when
    the original has characters that wouldn't survive dot access."""
    if _IDENT_RE.fullmatch(seg):
        return seg
    return safe_ident(seg, _QML_RESERVED)


def cpp_ident(seg: str) -> str:
    """A C++ identifier for a namespace or constexpr name. C++ has no
    "must start lowercase" rule, so we preserve the CodeSys name when
    it's already a valid identifier — this keeps the path readable
    (``plc_sym::IoConfig_Globals_Mapping::EMG_01`` instead of
    ``plc_sym::IoConfigGlobalsMapping::EMG01``)."""
    if _IDENT_RE.fullmatch(seg):
        if seg.lower() in _CPP_RESERVED:
            return seg + "_"
        return seg
    return safe_ident(seg, _CPP_RESERVED)


def load_symbols(path: pathlib.Path) -> dict[str, Any]:
    with path.open(encoding="utf-8") as f:
        return json.load(f)


def filter_symbols(raw: list[dict]) -> list[dict]:
    """Drop entries we can't expose as a runtime constant."""
    kept = []
    for s in raw:
        if s.get("class") != "Variable":
            continue
        node = s.get("nodeId", "")
        if "|var|" not in node:
            continue
        # Path with '#' (e.g. SM3_Basic#MC_GearIn.Acceleration) — comes
        # from CodeSys type references baked into FB instances. Not a
        # user symbol; not a valid identifier either.
        if "#" in s.get("path", ""):
            continue
        kept.append(s)
    return kept


def build_tree(symbols: list[dict]) -> "OrderedDict[str, Any]":
    """Group symbols by their dotted path into a nested OrderedDict so
    we can emit nested namespaces / object literals."""
    root: OrderedDict[str, Any] = OrderedDict()
    for s in symbols:
        path = s["path"]
        node_id = s["nodeId"]
        segs = path.split(".")
        cursor: OrderedDict[str, Any] = root
        for seg in segs[:-1]:
            cursor = cursor.setdefault(seg, OrderedDict())
            if isinstance(cursor, str):
                # Collision: a leaf and a branch share the same name.
                # Pick the branch and let codegen keep going; the leaf
                # was likely a property that we don't surface anyway.
                cursor = OrderedDict()
        leaf = segs[-1]
        cursor[leaf] = node_id
    return root


# ────────────────────────────────────────────────────────────── C++ ──

_CPP_HEADER = '''\
// AUTO-GENERATED from symbols/symbols.json — DO NOT EDIT
// Run scripts/regen-symbols.py to refresh.
//
// Source:    {source}
// Symbols:   {count}
// Generated: {ts}
#pragma once

#include <string_view>

namespace smbq6r::plc_sym {{
'''

_CPP_FOOTER = '''\
} // namespace smbq6r::plc_sym
'''


def emit_cpp(tree: "OrderedDict[str, Any]", out: list[str], indent: int = 0) -> None:
    pad = "    " * indent
    for k, v in tree.items():
        if isinstance(v, dict):
            ident = cpp_ident(k)
            out.append(f"{pad}namespace {ident} {{")
            emit_cpp(v, out, indent + 1)
            out.append(f"{pad}}} // namespace {ident}")
        else:
            ident = cpp_ident(k)
            esc = v.replace("\\", "\\\\").replace('"', '\\"')
            out.append(f'{pad}inline constexpr std::string_view '
                       f'{ident}{{"{esc}"}};')


def write_cpp(tree: "OrderedDict[str, Any]", meta: dict, dst: pathlib.Path,
              count: int) -> None:
    body: list[str] = [_CPP_HEADER.format(
        source=meta.get("source", "?"),
        count=count,
        ts=meta.get("generated", _dt.datetime.now().isoformat(timespec="seconds")),
    )]
    emit_cpp(tree, body, indent=0)
    body.append(_CPP_FOOTER)
    dst.parent.mkdir(parents=True, exist_ok=True)
    dst.write_text("\n".join(body) + "\n", encoding="utf-8")


# ────────────────────────────────────────────────────────────── QML ──

_QML_HEADER = '''\
// AUTO-GENERATED from symbols/symbols.json — DO NOT EDIT
// Run scripts/regen-symbols.py to refresh.
//
// Source:    {source}
// Symbols:   {count}
// Generated: {ts}
pragma Singleton
import QtQml 2.15

QtObject {{
'''

_QML_FOOTER = '''\
}
'''


def emit_qml(tree: "OrderedDict[str, Any]", out: list[str], indent: int = 1,
             is_root: bool = True) -> None:
    """Emit nested object literals.

    QML quirk: ``property var x: { ... }`` is parsed as a code block,
    not an object literal — so to bind an object we must wrap it in
    parentheses: ``property var x: ({ ... })``. Without the parens
    the page fails to load with a syntax error and the surface area
    that imports it (PlcPage.qml here) renders as a blank pane.
    """
    pad = "    " * indent
    items = list(tree.items())
    for i, (k, v) in enumerate(items):
        last = i == len(items) - 1
        # Root level → must lowerCamel (QML rule).
        # Deeper levels → preserve the original CodeSys name so reads
        # like ``PlcSymbols.globalVars.GVL.Enable`` match what the dev
        # sees in CodeSys IDE and on the wire.
        ident = to_qml_root_ident(k) if is_root else qml_nested_key(k)
        if isinstance(v, dict):
            if is_root:
                # Parenthesise: ``property var x: { ... }`` would parse
                # as a code block; ``({...})`` is the object literal.
                out.append(f"{pad}readonly property var {ident}: ({{")
                emit_qml(v, out, indent + 1, is_root=False)
                out.append(f"{pad}}})")
            else:
                comma = "" if last else ","
                out.append(f'{pad}"{ident}": {{')
                emit_qml(v, out, indent + 1, is_root=False)
                out.append(f"{pad}}}{comma}")
        else:
            esc = v.replace("\\", "\\\\").replace('"', '\\"')
            if is_root:
                out.append(f'{pad}readonly property string {ident}: '
                           f'"{esc}"')
            else:
                comma = "" if last else ","
                out.append(f'{pad}"{ident}": "{esc}"{comma}')


def write_qml(tree: "OrderedDict[str, Any]", meta: dict, dst: pathlib.Path,
              count: int) -> None:
    body: list[str] = [_QML_HEADER.format(
        source=meta.get("source", "?"),
        count=count,
        ts=meta.get("generated", _dt.datetime.now().isoformat(timespec="seconds")),
    )]
    emit_qml(tree, body, indent=1, is_root=True)
    body.append(_QML_FOOTER)
    dst.parent.mkdir(parents=True, exist_ok=True)
    dst.write_text("\n".join(body) + "\n", encoding="utf-8")


# qmldir — minimal, registers the singleton.
def write_qmldir(dst: pathlib.Path) -> None:
    dst.write_text("module Generated\nsingleton PlcSymbols 1.0 PlcSymbols.qml\n",
                   encoding="utf-8")


# ────────────────────────────────────────────────────────────── main ──

def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--symbols", default="symbols/symbols.json",
                    type=pathlib.Path,
                    help="curated picker output to read")
    ap.add_argument("--cpp-out", default="src/generated/plc_symbols.h",
                    type=pathlib.Path,
                    help="C++ header destination")
    ap.add_argument("--qml-out", default="qml/generated/PlcSymbols.qml",
                    type=pathlib.Path,
                    help="QML singleton destination")
    args = ap.parse_args()

    if not args.symbols.is_file():
        print(f"error: symbols file not found: {args.symbols}", file=sys.stderr)
        return 1

    data = load_symbols(args.symbols)
    syms = filter_symbols(data.get("symbols", []))
    if not syms:
        print(f"warning: no usable symbols after filtering "
              f"(input had {len(data.get('symbols', []))})", file=sys.stderr)
    tree = build_tree(syms)

    write_cpp(tree, data, args.cpp_out, count=len(syms))
    write_qml(tree, data, args.qml_out, count=len(syms))
    write_qmldir(args.qml_out.parent / "qmldir")

    print(f"[regen] kept {len(syms)}/{len(data.get('symbols', []))} symbols")
    print(f"[regen] wrote {args.cpp_out}")
    print(f"[regen] wrote {args.qml_out}")
    print(f"[regen] wrote {args.qml_out.parent / 'qmldir'}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
