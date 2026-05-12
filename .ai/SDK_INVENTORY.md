# SDK Envanteri ve Değer Analizi

> Lavichip SDK'sındaki tüm kaynakların kapsamlı katalog ve bizim için kullanım değeri.
> Bu dosya **kararları desteklemek için** referanstır — neyi kopyalayalım, neyi sadece
> ilham aldık, neyi görmezden gelelim.

## 0. En Kritik Bulgular

**Vendor referans HMI'sı (`demoV1.2.0`) 6 eksen kol robotu için DEĞİL.** İçindeki
action kodları (`actionCode_*.ini`) **enjeksiyon kalıp pick-and-place gantry
robotu** için tasarlanmış: "X2 Fore/Back", "Y2 Rise/Fall", "Mold Close",
"Ejector Forward", "CorePuller" gibi komutlar — kartesyen 3-4 eksen
manipulator. SMB-Q6R hedefimiz olan **Fanuc 6R kol robotu** ile mekanik
olarak farklı; **action kataloğu doğrudan kullanılamaz**.

Yine de **UI mimarisi, çoklu-dil çerçevesi, hata sözlüğü formatı, tema
sistemi, program dosya formatı** %95 yeniden kullanılabilir / şablon
alınabilir.

## 1. Kategorize Envanter

### 1.1 Doğrudan Çalışan Demolar

```
Lavichip QT teach pendant development SDK/2. Interface development demo/HWInterfaceDemo/
├── backlight.tar.gz    — Qt5 Widgets, /sys/class/backlight slider
├── button.tar.gz       — Qt5 Widgets, /dev/buttonstop (Enable S1/S2 okuma)
├── led.tar.gz          — Qt5 Widgets, /dev/leds ioctl, 5 LED toggle
├── matrixKeys.tar.gz   — Qt5 Widgets, /dev/input/event0 evdev dinleyici
├── pwm.tar.gz          — Qt5 Widgets, /dev/pwm ioctl (buzzer)
├── rotaryEncoder.tar.gz — Qt5 Widgets, /dev/buttons (Mode 3-pos switch)
└── wheel.tar.gz        — Qt5 Widgets, Qt mouse wheel event handler
```

**Değer:** ★★★★★
- Her donanım için **çalışan referans uygulama** = bizim implementasyonumuza karşı
  **oracle** (karşılaştırma noktası).
- HN00-09Q6 için aynı `/dev/*` yollarını kullanır (HT0803/HT0804 zamanı yazıldı
  ama API uyumlu).
- **Plan:** Hepsini Docker pipeline'ımızla cross-compile edip cihaza deploy ederiz
  (Phase 1.5'te). Bizim app yanına koyarız, fiziksel donanım test sırasında
  her ikisinin sonucunu kaydederiz.

### 1.2 Vendor Tam HMI Demo (`demoV1.2.0/`)

Windows x86_64 Qt5 Widgets uygulaması. Çalışan binary değil ama **kaynak ağacı
zenginliği** çok yüksek:

```
demoV1.2.0/L...V1.2.0/
├── HMI/program/                     — Vendor robot program dosyaları (örnek)
│   ├── 00.V2x_Lpro                  — Ana program (12-kolon int format)
│   ├── 00.V2x_Lprosq1..8            — Alt programlar (1..8)
│   ├── 00.V2x_Arr, 00.V2x_Ir_Arr    — Diziler
│   ├── 00.V2x_Refer                 — Referans noktaları
│   ├── 00.V2x_Extend, 00.V2x_Label_ini — Ek konfig
│   └── c.V2x_Lpro, c.V2x_Lprosq*    — "Current" program varyantları
├── data/                            — TÜM kullanıcı verisi + çoklu dil
│   ├── actionCode_*.ini             — Robot komut sözlüğü (12 dil)
│   ├── faultmeaning_*.ini           — Hata kodu sözlüğü
│   ├── warnmeaning_*.ini            — Uyarı sözlüğü
│   ├── handNoticeMeaning_*.ini      — Kullanıcı bildirimleri
│   ├── originMeaning_*.ini          — Origin/home pozisyon mesajları
│   ├── servoparaname_*.ini          — Servo parametre adları
│   ├── lognumber_*.ini              — Log entry adları
│   ├── helprecord_*.ini             — Help kayıtları
│   ├── IM_modifyName_*.ini          — Variable rename mesajları
│   ├── *.qm                         — Qt çeviri binary'leri (12 dil)
│   ├── *.qss                        — 4 renk teması (black/green/orange/purple/yellow)
│   ├── *.ini *.bin *.dat            — Sistem/yapılandırma verisi
│   ├── instructions_en/Part_*.htm   — 62 HTML instruction sayfası (İngilizce)
│   ├── instructions_cn/Part_*.htm   — 62 HTML instruction sayfası (Çince)
│   └── assistant.htm + assistant.files/ — Online yardım modülü
├── translations/                    — Qt'nin kendi .qm dosyaları (qt_*.qm)
├── HMI dosyaları + DLL'ler         — Windows runtime
└── *.exe                            — PE32+ x86_64 GUI uygulaması
```

#### Çoklu Dil — Türkçe Dahil ★★★★★

`actionCode_turkey.ini`, `faultmeaning_turkey.ini`, `warnmeaning_turkey.ini`,
`handNoticeMeaning_turkey.ini`, `originMeaning_turkey.ini`,
`lognumber_turkey.ini`, `helprecord_turkey.ini`, `IM_modifyName_turkey.ini`,
`turkey.qm` — **vendor zaten Türkçe yapmış**.

Çevirilerden bir kesit (kalıp manipulator için):
```ini
[actionCode]
0=Hareket yok
1=AltPrg Geri
2=AltPrg İleri
3=AltPrg Yükseltme
...
31=Kalıp Kapalı Aktif
33=İtici İleri Aktif
```

**Plan:** Kendi 6R action sözlüğümüz için bu dosya **formatını** kullan
([actionCode] ini section'ı, integer key=Turkish text). İçeriği değiştir.
qm dosyaları için: standart `lupdate/lrelease` ile kendimiz üretebiliriz —
ama Qt'nin temel widget'larının Türkçe çevirisi (`qt_tr.qm`) henüz hiçbir
yerde gelmedi (vendor da yok). Onu ayrı bulmamız gerekecek.

#### Tema Sistemi (QSS)

Vendor 4 renk teması sunuyor:
- `setupWidget_black_color.qss` (default dark)
- `setupWidget_green_color.qss`
- `setupWidget_orange_color.qss`
- `setupWidget_purple_color.qss`
- `setupWidget_yellow_color.qss`

QSS = Qt Style Sheets, Widgets için CSS-benzeri syntax. **QML için kullanılamaz** —
QML'in kendi `import QtQuick.Controls.Material` gibi tema motoru var ama bizim
elimizdeki QML 2.12 modülünde Material/Universal yok. **Plan:** QML için
elle bir `Theme.qml` singleton yazacağız (Phase 2'de yapılır), QSS dosyaları
sadece **renk paleti referansı** olarak okunur.

#### Robot Program Dosya Formatı (.V2x_*)

Vendor formatı tersine mühendislik:

**`00.V2x_Lpro`** (ana program) — düz metin, satır başına 12 boşluk-ayrılmış integer:
```
0 151 0 0 0 0 0 50 0 0 0 0
1 153 0 0 0 0 0 50 0 0 0 0
2 6 0 0 0 0 0 0 0 0 0 0
3 152 0 0 0 0 0 50 0 0 0 0
```

Kolonlar muhtemelen: `[satir_no] [action_code] [param1..10]`. action_code
(151/153/152/6/82/111) `actionCode_*.ini`'deki kodlarla eşleşir. Param'lar
muhtemelen koordinat, hız, gecikme.

**`00.V2x_Refer`** (referans noktaları) — INI formatında, her bölüm bir nokta:
```ini
[0]
number=1
name=模上待机   ; "Mold-on standby"
used=0
data0=0 151 0 0 0 0 0 50 0 0 0 0
data1=0 152 0 0 0 0 0 50 0 0 0 0
...
data9=34 160 0 0 0 112 86 146 104 0 0 0
```

Bu format **Faz 6 program editörü** için ilham verici ama doğrudan
benimsenmemeli — bizim 6R için XYZRPY + joint angles içeren bir format
gerekecek (vendor'ın 4-eksen pick-and-place için tasarlanmış).

#### Hata Sözlüğü

`faultmeaning_english.ini`:
```
[faultMeaning_cn]
1=[Y2 Rise] timeout
2=[Y2 Fall] timeout
...
13=[Horiz1 Limit] and [Verti1 Limit] signal coexist
```

Format: `kod=mesaj`. **Plan:** Aynı format → kendi 6R için (örn. `J1_servo_off`,
`E_stop_triggered`, `enable_switch_panic`, `plc_link_lost`, ...).

#### 62 Instruction HTML Sayfası

`instructions_en/Part_0.htm`..`Part_62.htm` — her biri bir instruction'ın
kullanıcı kılavuzu HTML formatında, içinde gömülü PNG/JPG screenshot'lar var
(`Part_X.files/image*.png`). Yardım sistemi olarak vendor bunu QTextBrowser ile
gösteriyor olmalı.

**Değer:** ★★ — içerik manipulator'a özel, ama format Phase 6+'da kendi
yardım sistemimiz için **şablon** alınabilir.

### 1.3 Anahtar Değer Tanımlama Araçları

```
Lavichip QT teach pendant development SDK/4. Key-value custom software/
├── HT0803KeyValueDefine.tar.gz
└── HT0804KeyValueDefine.tar.gz
```

Ubuntu Qt uygulaması. 14 matrix tuşunun her birine farklı Linux KEY_*
kodu atamak için GUI editör. `default.keySet` dosyası `/etc/key_config/`
altına gider.

**Değer:** ★★★ — HN00-09Q6 için key map ayarlamak gerekirse kullanılabilir
ama bizim Diagnostics ekranımız zaten key code'ları gösterip log'layacak,
böylece manuel map keşfedilebilir.

### 1.4 Eski SDK Ortamı (`3. Developmental environment/`)

```
arago-tisdk-qt5.12.9-eglfs-v1.0.0-lavich-20220110.sh   (368 MB)
Ubuntu64-kernel4.9-Qt5.12.9-20220110.rar              (3.5 GB)
```

**TI Sitara AM335x için eski SDK + eski VM imajı.** HN00-09Q6 RK3568'le
**uyumsuz**. Sıfır değer — diske yer kaplamasından başka şey değil.
**Önerim:** Yedeklendikten sonra silinebilir (~3.9 GB kazanım).

### 1.5 CNC DT550 RAR (33 MB)

`CNC DT550_C30_TE400_31.1.20.rar` — henüz açılmadı (host'ta `unrar`/`7z` yok).
İçerikten `DT550` model adı CNC kontrolcüsü, `C30_TE400 31.1.20` versiyon
ipucu veriyor. Muhtemelen CodeSys + Qt + EtherCAT entegrasyonu referansı.

**Plan:** Phase 2 başlangıcında host'a `apt install p7zip-full` ile
extract edilir. Phase 1 için gerekli değil.

### 1.6 EtherCAT Master Station Application Development.docx

1.2 MB Word belgesi. Phase 2 PLC entegrasyonu öncesi okunmalı.

### 1.7 LAVICHIP Customer Customized Demand Table V1.0.xlsx

7.3 MB Excel. Müşteri özelleştirme seçenekleri tablosu — pendant'ın hangi
varyantlarda (renk, buton sayısı, etiket) yapılabildiğini listeleyen vendor
matrisi. Bizim için doğrudan değer yok ama AKIM METAL OEM ilişkisini
şekillendiren belge olabilir.

## 2. Yeniden Kullanılabilirlik Matrisi

| Kaynak                              | Yeniden Kullan? | Nasıl?                              | Faz   |
|-------------------------------------|-----------------|--------------------------------------|-------|
| HWInterfaceDemo source'ları         | KOPYALA pattern | C++ pattern referansı (zaten kullandık) | 1   |
| HWInterfaceDemo cross-compiled .bin | KULLAN          | Bizim test sırasında "oracle" olarak  | 1.5 |
| actionCode_turkey.ini formatı       | FORMAT ALL      | İçeriği 6R için yeniden yaz           | 6   |
| faultmeaning_turkey.ini             | FORMAT + İÇERİK | İçerik %30 ortak (E-Stop, watchdog...) | 3, 5 |
| handNoticeMeaning_turkey.ini        | FORMAT + İÇERİK | %50 ortak (Connection lost, vs)       | 2, 5 |
| servoparaname_turkey.ini            | FORMAT + İÇERİK | Servo parametreleri benzer            | 7   |
| .V2x_Lpro / .V2x_Refer formatı      | İLHAM           | Kendi 6R formatımızı tasarla (XYZRPY) | 6   |
| QSS tema dosyaları                  | RENK PALETİ     | QML için Theme.qml singleton'a aktar  | 2+  |
| qt_*.qm Qt çevirileri               | DOĞRUDAN        | translations/ dizinine kopyala         | 8   |
| instructions HTML formatı           | ŞABLON          | Phase 6+ help system için             | 6+  |
| Eski TI SDK (arago-tisdk)           | KULLANMA        | RK3568 uyumsuz                        | —   |
| CNC DT550 RAR                       | ARAŞTIR         | CodeSys + EtherCAT pattern             | 2   |
| EtherCAT.docx                       | OKU             | Phase 2 öncesi referans                | 2   |

## 3. Aksiyon Listesi (önceliklendirilmiş)

### Bu hafta (Phase 1 kapsamı)
1. ⏳ **Vendor HWInterfaceDemo'ları cross-compile et** — Docker pipeline ile,
   Focal arm64 hedefinde. Tarif: scripts/build-vendor-demos.sh.
2. ⏳ Hepsini cihazda `/home/Tronlong/vendor-demos/` altına deploy et.
3. ⏳ Karşılaştırma testi yap (PHASE1_TEST_PLAN.md'ye göre).
4. ⏳ Karşılaştırma sonuçlarını `ENGINEERING_LOG.md`'ye yaz: LED port haritası,
   key code haritası, switch bit haritası.

### Önümüzdeki hafta
5. ⏳ CNC DT550 RAR'ı açıp incele (`sudo apt install p7zip-full` sonrası).
6. ⏳ EtherCAT.docx oku (Phase 2 ön çalışma).
7. ⏳ Turkish .qm dosyalarını projemize bağla (translations/).

### Phase 2+
8. ⏳ actionCode dosyasının kendi 6R sürümünü tasarla.
9. ⏳ .V2x_* formatına alternatif olarak kendi `*.smbpro` formatımızı tanımla.
10. ⏳ QSS tema palet renklerini Theme.qml'e taşı.

## 4. Riskler

- **Format reverse-engineering eksik:** .V2x_Lpro'da 12 kolonun **anlamı**
  belge edinilmedi. Tersine mühendislik tahminle yapılır; vendor'dan
  belge istemek (AKIM METAL OEM kanalıyla) daha güvenli olur.
- **Demo binary'leri eski model donanım için:** HT0803/HT0804 LED port
  haritası ≠ HN00-09Q6. Yine de **API uyumlu** olduğu için demolar bizim
  HN00-09Q6'da çalışır — sadece hangi fiziksel LED'in hangi port'ta olduğu
  farklı görünür. Bu zaten testimizle ölçeceğimiz şey.
- **Türkçe .qm vendor tarafından üretilmiş:** Lavichip içindeki yazılım
  görüntülemesi için. Bizim metinlerimiz farklı (6R için), o yüzden
  `turkey.qm` dosyası **doğrudan kullanılamaz** — sadece string'leri çevirmek
  için referans (vendor'ın hangi kelimeyi Türkçe nasıl çevirdiğini görmek).
