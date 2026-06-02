# PLC Entegrasyonu — CodeSys SMB-MAT-LC-C07

> **Status (2026-06-02):** Phase 1.5 OPC UA bootstrap **canlı** — pendant
> CodeSys'e bağlanıyor, namespace'i browse ediyor, ReadWrite scalar'lara
> subscribe + write yapıyor. Bu dosya tasarım kararlarını + concrete
> implementasyonu + Phase 2 yol haritasını tutar.

## 1. Genel Mimari

```
┌──────────────┐  Ethernet  ┌─────────────────┐  EtherCAT  ┌──────────┐
│  HN09-Q6     │ ────────── │ SMB-MAT-LC-C07  │ ────────── │ 6R Robot │
│ (Teach Pend.)│  OPC UA    │ (CodeSys 3.5.20)│  / Servo   │ Servos   │
│ 192.168.0.245│            │   192.168.0.2   │            │          │
└──────────────┘            └─────────────────┘            └──────────┘
       ↑                            ↑
   yüksek seviye               düşük seviye
   (UI, jog komutu,            (trajectory, kinematics,
    program editör)             EtherCAT master, safety)
```

## 2. Sorumluluk Dağılımı

| Görev                          | Pendant | PLC |
|--------------------------------|:-------:|:---:|
| Trajectory planning (CP, PTP)  |         | ✓   |
| Forward Kinematics             | preview | ✓   |
| Inverse Kinematics             |         | ✓   |
| Servo komutu (her ms)          |         | ✓   |
| EtherCAT master                |         | ✓   |
| Güvenlik mantığı (sertifikalı) |         | ✓   |
| Jog komut komutlamak (m/s, °/s)| ✓       |     |
| Program editörü                | ✓       |     |
| Frame editörü (TCP, User, Base)| ✓       | ✓   |
| Alarm gösterimi                | ✓       |     |
| IO görünümü                    | ✓       | ✓   |

Komut dizisi yorumlayıcı **PLC tarafında** — pendant düzenleyici +
görüntüleyici. Off-line pendant durumunda bile program akışı çalışır.

## 3. Seçilen Protokol — OPC UA

| Karar | Detay |
|-------|------|
| Stack | **open62541 v1.3.10 LTS** (Open Source, BSD/MIT) |
| Linkage | Statik, CMake `FetchContent` (`SMB_Q6R_OPCUA` ON) |
| Sebep | Type-safe, browseable, modern endüstri standardı; ekstra device-side .deb yok |
| Reddedilen | Modbus TCP (16-bit register limiti, tip yok), CodeSys ADS (lisans) |
| Binary etki | +1.7 MB (host 5.8 MB, arm64 stripped 1.8 MB) |

### 3.1 Bağlantı Parametreleri

| | Değer |
|---|---|
| Endpoint URL | `opc.tcp://192.168.0.2:4840` |
| Published EndpointUrl (server) | `opc.tcp://SMB:4840` (CodeSys hostname publish ediyor; mismatch warn — zararsız) |
| Security Mode | None |
| User Token | Anonymous |
| Session Timeout | open62541 default (uint32 max) |
| Iterate Poll | 50 ms QTimer |

### 3.2 Network Gereksinimleri

- PLC ve pendant aynı L2 switch'te olmalı.
- Pendant `eth1` üzerinde iki IP taşır:
  - `192.168.1.245/24` — SSH/deploy/development
  - `192.168.0.245/24` — PLC ile haberleşme (alias)
- `scripts/configure-plc-network.sh` netplan'a kalıcı yazar.

## 4. CodeSys Sembol Yapısı

### 4.1 Symbol Configuration (XML)

`vendor-demos/CodeSysSP20_3Axis_CNC_SMB_LAZER_17052025_1620.Device.Application.xml`
(715 KB, 193 node) CodeSys IDE'de **Symbol Configuration** objesinden
"Build" ile üretiliyor.

- CodeSys version: 3.5 SP20 Patch 2+
- Runtime ID: 3.5.18.50
- Setting flags: `SupportOPCUA, XmlIncludeNodeFlags, XmlIncludeComments`

### 4.2 GVL Grupları

5 üst-seviye Global Variable List:

| GVL | İçerik |
|-----|--------|
| `Constants` | Versiyon string'leri (CompilerVersion, RuntimeVersion) |
| `GVL` | Genel: Enable, _xLaserOff, g_CNCMachine, X_Array, Y_Array, pGeoInfo |
| `GVL_Control_Var` | Jog ve durum: jog_Negative_X/Y/Z, Jog_Mode_Active, home_start, all_axis_zero_ok, abort_limit_switch, ... |
| `IoConfig_Globals` | Donanım obj. (T_CAADiagDeviceDefault, T_IoDrvEthercat_Diag struct'ları) |
| `IoConfig_Globals_Mapping` | I/O bitleri direkt adresli: EMG_01, Door_Left_10, Limit_*, emg_stop_Q00, laser_ctrl_Q03, pano_ayd_Q04 |

### 4.3 Node ID Formatı (KEŞFEDİLDİ)

```
Application root: ns=4;s=|var|MAT LC-C07.Application
PLC device:       ns=4;s=|plc|MAT LC-C07
Variable:         ns=4;s=|var|MAT LC-C07.Application.<GVL>.<VarName>
```

**Önemli:** Symbol XML'deki proje adı (`CodeSysSP20_3Axis_CNC_SMB_LAZER_...`)
node ID'lerde GEÇMİYOR. Sadece **PLC device name** (`MAT LC-C07`) +
sabit `Application` segment'i.

Örnekler:
- `ns=4;s=|var|MAT LC-C07.Application.GVL.Enable`
- `ns=4;s=|var|MAT LC-C07.Application.GVL_Control_Var.jog_Negative_X`
- `ns=4;s=|var|MAT LC-C07.Application.IoConfig_Globals_Mapping.EMG_01`

### 4.4 PlcPage Default Watch Listesi

`qml/PlcPage.qml` içinde 12 node hardcoded — Connect anında auto-subscribe:

| Label | Node | RW | Tip |
|-------|------|:--:|-----|
| GVL.Enable | `...Application.GVL.Enable` | ✓ | BOOL |
| EMG_01 | `...IoConfig_Globals_Mapping.EMG_01` | R | BIT |
| Door_Left | `...IoConfig_Globals_Mapping.Door_Left_10` | R | BIT |
| Door_Right | `...IoConfig_Globals_Mapping.Door_Right_11` | R | BIT |
| Limit_Xpos | `...IoConfig_Globals_Mapping.Limit_Xpos_06` | R | BIT |
| Limit_Xneg | `...IoConfig_Globals_Mapping.Limit_xneg_07` | R | BIT |
| Limit_Ypos | `...IoConfig_Globals_Mapping.Limit_Ypos_05` | R | BIT |
| Limit_Yneg | `...IoConfig_Globals_Mapping.Limit_Yneg_04` | R | BIT |
| Limit_Zneg | `...IoConfig_Globals_Mapping.Limit_Zneg_03` | R | BIT |
| emg_stop | `...IoConfig_Globals_Mapping.emg_stop_Q00` | ✓ | BIT |
| laser_ctrl | `...IoConfig_Globals_Mapping.laser_ctrl_Q03` | ✓ | BIT |
| pano_ayd | `...IoConfig_Globals_Mapping.pano_ayd_Q04` | ✓ | BIT |

## 5. PlcLink Implementasyonu (özet)

`src/plc_link.{h,cpp}` — open62541 UA_Client'ı bir QThread içinde sarmalar.

```
PlcLink (QObject, moveToThread(workerThread_))
  ├ slots (queued)
  │   ├ connectToServer(QString url)
  │   ├ disconnectFromServer()
  │   ├ readNode(QString id)         → emit valueRead / readFailed
  │   ├ subscribeNode(QString id)    → DataChange → emit valueChanged
  │   ├ writeNode(QString id, QVariant)  → emit writeSucceeded / writeFailed
  │   └ browseNamespace(maxDepth, maxNodes) → emit nodeDiscovered, qInfo log
  ├ signals
  │   ├ stateChanged(State)          (qRegisterMetaType ile queue'ya safe)
  │   ├ connected / disconnected / connectionFailed
  │   └ value* / write* / nodeDiscovered
  └ iterate timer (50 ms, worker thread) — UA_Client_run_iterate(0)
```

### Önemli Implementasyon Notları

1. **Worker thread**: ana UI thread'i hiçbir zaman OPC UA blocking
   network call'ı görmez. QML'den çağrılan slot'lar
   `Qt::QueuedConnection` ile worker'a teslim edilir.

2. **Idempotent connect**: aynı URL'e tekrar Connect basıldığında
   doConnect() erken döner; mevcut session'ı bozmaz.

3. **Auto-browse on connect**: bağlantı kurulur kurulmaz
   `browseNamespace(5, 250)` çağrılır; her node `qInfo`'ya yazılır →
   journalctl'dan node ID keşfi mümkün olur.

4. **Subscription survival**: PlcLink subscription'ları kendi içinde
   tutar; QML tarafında PlcPage `Connections.onPlcStateChanged` ile
   her Connected geçişinde `subscribeAll()` çağırır.

5. **Variant conversion**: `variantFromUa()` ve `doWrite()` şu
   scalar tipleri destekler: Boolean, SByte/Byte, Int16/UInt16,
   Int32/UInt32, Int64/UInt64, Float, Double, String. Struct
   (ExtensionObject) için "<TypeName>" placeholder döner.

6. **BadAttributeIdInvalid (struct nodes)**: CodeSys
   `T_CAADiagDeviceDefault` gibi function-block tipindeki node'ları
   doğrudan monitor edemeyiz — child scalar field'larına subscribe
   etmek gerekir. PlcPage default listesinden çıkarıldı.

## 6. Watchdog / Heartbeat — YAPILACAK (Phase 2)

**Şu an yok.** Phase 2'de eklenmesi zorunlu:

```
Pendant → PLC:  her 100 ms'de bir "alive" pulse + son komut sayacı
PLC → Pendant:  her 100 ms'de durum paketi + alarm bayrakları
```

- Pendant 500 ms heartbeat göndermezse → PLC otomatik **Hold** moduna
  geçer (hareketi durdurur, motorlar enerjili kalır).
- Pendant 1000 ms PLC cevabı almazsa → UI "Connection Lost" ekranı,
  jog/program kontrolleri kilitlenir.

Önerilen implementasyon:
- PLC tarafında `GVL.PendantHeartbeat` (UDINT) — pendant'ın yazacağı
  monoton counter
- PLC tarafında `GVL.PlcHeartbeat` (UDINT) — pendant'ın subscribe
  olacağı, PLC'nin her 100 ms artırdığı counter
- Pendant tarafında `PlcLink::startHeartbeat()` 100 ms QTimer

## 7. Güvenlik Notu

E-Stop ve Enable Switch sinyalleri **Pendant donanımında** zaten
ST1/ST2 ve COM/NO/NC pinleri üzerinden çıkıyor. Bu sinyaller doğrudan
**PLC'nin güvenlik girişine** kablolanmalı, hiçbir yazılım katmanından
geçmemeli. Pendant yazılımı yalnızca bu sinyalleri kullanıcıya **görsel
olarak** gösterir; gerçek güvenlik fonksiyonu kablolu/sertifikalı PLC
tarafıdır.

## 8. Phase 2 Yol Haritası (Faz 2 Detay)

| Adım | İş |
|------|----|
| 2.1 | Heartbeat (yukarı) — pendant ↔ PLC |
| 2.2 | Reconnect-on-disconnect (auto retry exponential backoff) |
| 2.3 | Jog komutları: 14 fiziksel buton → PLC `GVL_Control_Var.jog_Negative_X` vb. yazma |
| 2.4 | Mode switch state → PLC `GVL.Sys_Mode` writeback |
| 2.5 | LED durumu PLC'den subscribe (Enable/Servo/Stop sinyalleri) |
| 2.6 | Joint position display (PLC'den live X/Y/Z + Cartesian) |
| 2.7 | Alarm view (PLC alarm GVL listesi) |
| 2.8 | Program list + load/run/stop (PLC `Sys_Cmd` rising edge) |

## 9. Açık Sorular

1. PLC sürücü hangi vendor? EtherCAT slave id'ler (vendor-id, product-code)
   subscription ile alınabilir mi?
2. Robot kinematic parametreleri (DH veya offset tabanlı) PLC'de hazır mı?
   (CodeSys SoftMotion'ı kullanıyorsa configure edilmiştir)
3. Fanuc 6R'nın yorumcusu (TP komutları, KAREL benzeri) PLC tarafında
   hangi seviyede? Sadece point-to-point mi, full G-code mi?
4. Sym XML 3-axis CNC LAZER projesinden, 6-axis robot projesi farklı
   bir XML mi yoksa aynısı mı genişletilecek?

## 10. Referans

- open62541 docs: <https://www.open62541.org/doc/v1.3/>
- CodeSys OPC UA Server (V3 SP14+):
  <https://help.codesys.com/api-content/2/codesys/3.5.17.0/en/_cds_runtime_opc_ua_server/>
- Symbol Configuration export format:
  <https://help.codesys.com/api-content/2/codesys/3.5.17.0/en/_cds_obj_symbol_configuration/>
