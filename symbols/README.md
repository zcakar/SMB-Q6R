# symbols/ — PLC Symbol Manifest

Bu dizin, projeyle birlikte git'te tutulan **kurate edilmiş** PLC sembol
listesini barındırır. Uygulama kodu burada tanımlı **logical name**'leri
kullanır; runtime'da PlcLink bunları OPC UA NodeId string'lerine çözer.

## Akış

```
PLC (canlı)
   │
   │ doBrowse / plc-browse.json   (PlcLink, full dump, 5000 düğüm)
   ▼
tools/symbol-picker (host-side Qt Widgets tool)
   │
   │ kullanıcı tri-state checkbox ile seçim yapar
   ▼
symbols/symbols.json              (curated, git-tracked)
   │
   │ codegen (Phase 2)
   ▼
src/generated/plc_symbols.h       (C++ constexpr literals)
qml/generated/PlcSymbols.qml      (QML singleton)
   │
   ▼
uygulama kodu: PlcSymbols.gvl.enable   /   plc::sym::kGvlEnable
```

## Dosyalar

- `symbols.json` — picker'ın çıktısı, kurate edilmiş sembol listesi.
  PR diff'inde görünür; PLC yapısı değişirse dev manuel olarak resync eder.

## Picker'ı çalıştırmak

```bash
# Offline mode (en son cihazdan çekilmiş dump'tan)
bash scripts/pick-symbols.sh

# Veya açıkça bir dump dosyası belirt
bash scripts/pick-symbols.sh .ai/plc-browse-2026-06-04.json
```

## symbols.json şeması

```json
{
  "generated":    "ISO timestamp — picker tarafından yazıldı",
  "source":       "canlı endpoint URL veya offline dump path",
  "namespaceUri": "CodeSys app URI",
  "symbols": [
    {
      "name":      "gvl.enable",
      "path":      "Application.GVL.Enable",
      "nodeId":    "ns=4;s=|var|MAT LC-C07.Application.GVL.Enable",
      "class":     "Variable"
    }
  ]
}
```

`name` alanı camelCase-dotted; codegen bunu C++ ve QML identifier'larına
mekanik dönüştürür (`gvl.enable` → `kGvlEnable` / `PlcSymbols.gvl.enable`).
