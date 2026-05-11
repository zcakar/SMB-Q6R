# Fanuc 6R Robot Kinematik Notları

> Faz 4+'da derinlemesine ele alınacaktır. Bu, başlangıç çerçevesidir.

## Konfigürasyon

**Fanuc 6R**, 6 dönel eklemli (RRRRRR), kollaboratif sınıf robotuna
verilen jenerik addır; tipik mimari:

```
       ┌─ J5 (wrist pitch)
       │   ┌─ J6 (wrist roll, flange)
       │   │
   ┌── J4 forearm roll
   │
J3 elbow
   │
J2 shoulder
   │
J1 base (yaw)
```

Eklem isimleri (sektörde yaygın):

| Eklem | Sembol | Yönü              | Tipik Aralık     |
|-------|--------|-------------------|------------------|
| 1     | J1     | taban dönüşü       | ± 170°           |
| 2     | J2     | omuz eğimi         | -90°…+135°       |
| 3     | J3     | dirsek             | -180°…+180°      |
| 4     | J4     | ön kol dönüşü      | ± 180°           |
| 5     | J5     | bilek eğimi         | ± 125°           |
| 6     | J6     | bilek dönüşü        | ± 360°           |

Gerçek değerler robotun mekanik dokümanından alınmalı; üreticisi/modeli
henüz netleşmemiş — kullanıcı "Fanuc 6R" demiş, ancak fiziksel olarak
gerçek bir Fanuc mı yoksa Fanuc benzeri jenerik 6R mi olduğu Faz 0'da
açıklanacak.

## Sorumluluk: PLC vs Pendant

| Görev                                       | Konumu  |
|---------------------------------------------|---------|
| DH parametreleri / offset tablosu           | PLC     |
| Forward kinematics (kesin)                  | PLC     |
| Inverse kinematics                          | PLC     |
| Singular noktada anti-singularity damping   | PLC     |
| Yörünge planlayıcı (PTP, LIN, CIRC)         | PLC     |
| Joint hız/ivme limitleyici                  | PLC     |
| Pendant'ta önizleme FK (canvas çizim için)  | Pendant |

**Pendant tarafındaki FK** yalnızca görsel önizleme içindir; kontrol
döngüsünde kullanılmaz. Bu, Faz 4'te küçük bir 6R kinematik kütüphanesi
(KDL veya basit DH-tabanlı çözücü) ile çözülecektir.

## Koordinat Sistemleri (Frame'ler)

Endüstri standartı 4 frame:

- **World (Base):** Yerdeki sabit referans
- **Robot Base:** Robotun J1 ekseninin tabanı (genelde World ile çakışık)
- **Tool (TCP):** Flange + tool offset
- **User Frame:** Operatörün tanımladığı parça koordinat sistemi

Frame editörü Faz 7'de yapılır. PLC'ye yazılırken kabul:
- 6 değişkenlik vektör: X, Y, Z (mm), Rx, Ry, Rz (derece, Euler XYZ)
  veya quaternion (W, X, Y, Z). Karar Faz 7'de PLC ile birlikte verilir.

## Jog Modları

| Mod              | Hareket                                                 |
|-------------------|---------------------------------------------------------|
| Joint            | Tek eklemi seçilen yönde hareket ettir                  |
| World Cartesian  | TCP'yi World eksenlerinde hareket ettir                 |
| Tool Cartesian   | TCP'yi Tool ekseni boyunca hareket ettir                |
| User Cartesian   | TCP'yi User Frame'inde hareket ettir                    |

Pendant tarafından gönderilecek minimum komut seti:
- `jog_start(mode, axis_index, direction, speed_percent)`
- `jog_stop()`

Handwheel modunda her tıklama bir küçük artımsal hareket gönderir:
- `jog_increment(mode, axis_index, delta_mm_or_deg)`

## TCP (Tool Center Point) Tanımlama

Yöntemler:
- **4-point method:** TCP'yi 4 farklı yönelimde aynı sabit noktaya değdir
- **3-point method:** Hızlı yöntem, daha az kesin
- Manuel girilebilir: X, Y, Z, Rx, Ry, Rz

UI sihirbazı Faz 7'de.

## Program Modeli

Teach Pendant programları, çoğu robotta bir komut listesi olarak
saklanır:

```
1: J P[1] 100% FINE        ; Joint move to P[1] at 100% speed
2: L P[2] 500mm/s CNT50    ; Linear move at 500mm/s with 50% blending
3: WAIT DI[1] = ON         ; Wait for digital input
4: CALL SUBROUTINE_A       ; Subroutine call
5: J P[3] 50%
```

Faz 6'da editörümüz bu modelle uyumlu yapılacak (TP'ın Fanuc syntaks'ına
yakın bir alt küme).

PLC'de **yorumcu** olur; pendant sadece **editör + uploader**dir.

## Açık Sorular (Faz 4 öncesi)

1. Robotun gerçek DH parametreleri / link uzunlukları nedir?
2. Hangi tool flange standardı? (ISO 9409-1-40 muhtemelen)
3. Robot tabanı World ile mi çakışık, yoksa offset var mı?
4. Jog hızı limit kaç °/s ve mm/s?
5. Singularity yakınında davranış: hata mı, yavaşlatma mı, mesh redirect mi?
