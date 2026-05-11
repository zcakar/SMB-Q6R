# PLC Entegrasyonu — CodeSys SMB-MAT-LC-C07

> Bu dosya **Faz 2** geldiğinde derinlemesine geliştirilecek. Şimdi
> başlangıç notları ve kararsızlık alanları listelenir.

## Genel Mimari

```
┌──────────────┐  Ethernet  ┌─────────────────┐  EtherCAT / Servo  ┌──────────┐
│  HN09-Q6     │ ────────── │ SMB-MAT-LC-C07  │ ────────────────── │  6R Robot │
│ (Teach Pend.)│            │ (CodeSys PLC)   │                    │  Servos   │
└──────────────┘            └─────────────────┘                    └──────────┘
       ↑                            ↑
   yüksek seviye               düşük seviye
   komutlar, durum             hareket kontrolü
```

## Sorumluluk Dağılımı (Öneri)

| Görev                          | Pendant | PLC |
|--------------------------------|:-------:|:---:|
| Trajectory planning (CP, PTP)  |         | ✓   |
| Forward Kinematics             | (preview) | ✓ |
| Inverse Kinematics             |         | ✓   |
| Servo komutu (her ms)          |         | ✓   |
| EtherCAT master                |         | ✓   |
| Güvenlik mantığı (sertifikalı) |         | ✓   |
| Jog komut komutlamak (m/s, °/s)| ✓       |     |
| Program editörü                | ✓       |     |
| Komut dizisi yorumlayıcı       | (?)     | (?) |
| Frame editörü (TCP, User, Base)| ✓       | ✓   |
| Alarm gösterimi                | ✓       |     |
| IO görünümü                    | ✓       | ✓   |

Pendant'ın komut dizisi yorumlamasını **PLC'de** tutmak, off-line
çalışırken bile program akışının çalışması için en iyisidir. Pendant
yalnızca düzenleyici ve görüntüleyicidir.

## Olası Protokol Seçenekleri

### A. Modbus TCP
- **Avantaj:** En basit, CodeSys'in dahili sunucusu var, libmodbus ile
  C++ tarafı 200 satırda çalışır.
- **Dezavantaj:** Yalnızca 16-bit register ve coil; structured veri
  manuel paketlenir. Tip güvenliği yok.
- **Uygunluk:** Faz 2 hızlı başlama için ideal; Faz 4+'da yapay limit
  oluşturabilir.

### B. OPC UA
- **Avantaj:** Structured data, browseable, modern endüstri standardı,
  subscription tabanlı (push notification), `open62541` C++ binding'i var.
- **Dezavantaj:** Kurulum daha karmaşık; sertifika/security ayarı,
  bağlantı çağrıları daha verbose.
- **Uygunluk:** Uzun vade için en sağlam seçenek.

### C. CodeSys Sembolik
- **Avantaj:** PLC değişkenlerine doğrudan isimle erişim.
- **Dezavantaj:** Genelde lisanslı runtime gerektirir; portable değildir.

### Önerilen Yaklaşım

**Faz 2:** Modbus TCP ile başla (heartbeat + temel jog komutları).
**Faz 4 sonu:** Trafik gereksinimi netleşince OPC UA'ya geçişi değerlendir.

PLC tarafında her iki sunucu da paralel çalışabilir; bu, geçişi yumuşatır.

## Heartbeat / Watchdog

Pendant'tan PLC'ye **heartbeat** zorunludur:

```
Pendant → PLC:  her 100 ms'de bir "alive" pulse + son komut sayacı
PLC → Pendant:  her 100 ms'de durum paketi + alarm bayrakları
```

Pendant'ın 500 ms'den uzun süre heartbeat göndermemesi → PLC otomatik
**Hold** moduna geçer (hareketi durdurur, motorlar enerjili kalır).

Pendant'ın 1000 ms'den uzun süre PLC cevabı almaması → UI "Connection
Lost" ekranı göster, jog/program kontrollerini kilitle.

## Sembol / Register Haritası Şablonu (Modbus)

Bu Faz 2'de doldurulacaktır; iskelet:

| Modbus Adres    | Yön      | Sembol                     | Tip      | Açıklama                |
|-----------------|----------|-----------------------------|----------|--------------------------|
| HoldingReg 0    | Read     | `Sys.Mode`                  | u16      | 0=Stop 1=Man 2=Auto      |
| HoldingReg 1    | Read     | `Sys.Status`                | u16 bit  | bit0=Ready, bit1=Run...   |
| HoldingReg 2-7  | Read     | `Joint[1..6].Position`      | float32  | Eklem açıları (derece)   |
| HoldingReg 20-25| Read     | `Cartesian.{X,Y,Z,Rx,Ry,Rz}`| float32  | TCP pozisyonu             |
| HoldingReg 40   | Write    | `Cmd.JogAxis`               | i16      | 1..6 = eklem, 7..12=Cart |
| HoldingReg 41   | Write    | `Cmd.JogSpeed`              | u16      | 0..100 yüzde              |
| HoldingReg 42   | Write    | `Cmd.JogDirection`          | i16      | -1, 0, +1                 |
| HoldingReg 50   | Write    | `Cmd.Heartbeat`             | u16      | counter, monoton          |
| Coil 0          | Write    | `Cmd.ServoOn`               | bool     | Servo enable              |
| Coil 1          | Write    | `Cmd.ProgramStart`          | bool     | rising edge başlat       |
| Coil 2          | Write    | `Cmd.ProgramStop`           | bool     | rising edge durdur       |

## CodeSys Tarafı

CodeSys SMB-MAT-LC-C07 hakkında bilgi henüz toplanmadı. Faz 2'de
araştırılacak:

- Hangi runtime sürümü (CodeSys 3.5 SP21 olası)
- Yerleşik Modbus TCP Server / OPC UA Server eklentileri
- EtherCAT master eklentisi sürümü
- Robot kinematic library (CodeSys SoftMotion?) lisansı
- Cihazda PLC firmware'i ile beraber gelen demo proje var mı?

**Aksiyon (Faz 2 başında):** PLC dokümanını edin, IDE üzerinden bağlan,
mevcut programı çıkar.

## Güvenlik Notu

E-Stop ve Enable Switch sinyalleri **Pendant donanımında** zaten ST1/ST2
ve COM/NO/NC pinleri üzerinden çıkıyor. Bu sinyaller doğrudan **PLC'nin
güvenlik girişine** kablolanmalı, hiçbir yazılım katmanından geçmemeli.
Pendant yazılımı yalnızca bu sinyalleri kullanıcıya **görsel olarak**
gösterir; gerçek güvenlik fonksiyonu kablolu/sertifikalı PLC tarafıdır.

## Açık Sorular

1. PLC'nin IP'si ne olacak? (Önerilen: 192.168.1.10 — pendant 192.168.1.245)
2. EtherCAT bus ile servo sürücüler hangi vendor? (Faz 0 fotoğraflarındaki
   "UC-x07" yazılı küçük modül muhtemelen Servo veya I/O genişletme;
   doğrulamak gerek)
3. Robot kinematic parametreleri (DH veya offset tabanlı) PLC'de hazır
   mı, yoksa biz mi tanımlayacağız?
4. Fanuc 6R'nın yorumcusu (TP komutları, KAREL benzeri) tarafımızdan mı
   yapılacak, yoksa standart bir alt küme mi yetecek?
