# Referans Kaynaklar

## Donanım Belgeleri

| Belge                                              | Yol                                                              |
|----------------------------------------------------|------------------------------------------------------------------|
| HN00-09Q6 Datasheet (English)                      | `/home/embed/Dev/QT6/TeachPendant/HN00-09Q6 en-US.pdf`           |
| HN00-09Q6 Datasheet (Chinese)                      | `/home/embed/Dev/QT6/TeachPendant/HN00-09Q6.pdf`                 |
| HN00-09Q6 3D STEP CAD modeli                       | `/home/embed/Dev/QT6/TeachPendant/HN00-09Q6.stp`                 |
| Qt Teach Pendant Development Manual                | `/home/embed/Dev/QT6/TeachPendant/QT Teach pendant devlepoment manual.pdf` |
| EtherCAT Master Station Application Development    | `/home/embed/Dev/QT6/TeachPendant/EtherCAT Master Station Application Development .docx` |
| Lavichip TP Customer Customized Demand Table V1.0  | `/home/embed/Dev/QT6/TeachPendant/LAVICHIP  teach pendant customer customized demand tableV1.0.xlsx` |

## SDK ve Demo Yazılım

```
/home/embed/Dev/QT6/TeachPendant/Lavichip QT teach pendant development SDK/
├── 1. Application development documentation/
│   └── QT Teach pendant devlepoment manual.pdf
├── 2. Interface development demo/
│   ├── HWInterfaceDemo.tar.gz                — Qt demo kaynakları
│   └── HWInterfaceDemo/                       — açılmış arşiv
│       ├── backlight.tar.gz
│       ├── button.tar.gz
│       ├── led.tar.gz
│       ├── matrixKeys.tar.gz
│       ├── pwm.tar.gz
│       ├── rotaryEncoder.tar.gz
│       └── wheel.tar.gz
├── 3. Developmental environment/
│   ├── arago-tisdk-qt5.12.9-eglfs-v1.0.0-lavich-20220110(1).sh   — eski SDK (Qt 5.12, kernel 4.9)
│   └── Ubuntu64-kernel4.9-Qt5.12.9-20220110.rar                  — eski VM imajı
└── 4. Key-value custom software/
    ├── HT0803KeyValueDefine.tar.gz
    └── HT0804KeyValueDefine.tar.gz
```

> **Not:** SDK'daki Qt 5.12.9 ve kernel 4.9 versiyonları **eski model**
> HT0803/HT0804 için. Yeni HN00-09Q6 Qt 5.15.10 + kernel 5.10.209 ile
> gelir. Demo kodlarındaki cihaz API'leri (LED, buzzer vb.) hâlâ
> geçerli ama Qt-spesifik kısımlar farklı.

## Diğer Demolar

- **`demoV1.2.0/`** — Windows üzerinde çalışan eski HMI demo (Qt5 + DLL'ler)
- **`CNC DT550_C30_TE400_31.1.20.rar`** — CNC için CodeSys + Qt referansı (incelenmedi)

## Referans Projeler

| Proje    | Yol                          | Faydası                                |
|----------|------------------------------|-----------------------------------------|
| CADNC    | `/home/embed/Dev/CADNC/`     | `.ai/` deseni, modern Qt6 mimari        |
| SODOO    | `/home/embed/Dev/SODOO/`     | Kurumsal doküman seti şablonu           |
| MilCAD   | `/home/embed/Dev/MilCAD/`    | CAM modülü, ikonlar — UI ilham kaynağı  |

## Cihazda

```
192.168.1.245 (Tronlong, boş parola)
├── /userfs/app/                — uygulama dağıtım yolu
│   └── lyx_appDemo             — fabrika demo
├── /etc/profile.d/qt5.15.10.sh — Qt LD/PATH/QML
├── /etc/profile.d/qt_env.sh    — XCB + input device map
├── /etc/netplan/01-netcfg.yaml — network static IP
├── /sys/class/backlight/backlight/brightness  — 0..100
└── /dev/{leds,pwm,buttons,buttonstop,ttyS2}   — özel donanım
```

## Üçüncü Taraf

| Kaynak                          | Lisans   | Notlar                                    |
|---------------------------------|----------|-------------------------------------------|
| Qt 5.15.10                      | LGPL-3.0 | Cihazda hazır, ek kurulum gerekmez        |
| libmodbus                       | LGPL-2.1 | Modbus TCP istemcisi için aday            |
| open62541                       | MPL-2.0  | OPC UA istemcisi için aday                |
| KDL (Orocos Kinematics)         | LGPL-2.1 | Önizleme FK için aday                     |
| QtSerialPort                    | LGPL-3.0 | Qt 5.15.10 ile gelir                      |

## Hızlı Web Referanslar (gerektiğinde)

- Rockchip RK3568 datasheet — Rockchip resmi
- CodeSys SoftMotion robotic library docs — CodeSys helper
- Fanuc Karel programming reference (kamu) — programlama syntax ilhamı
- ISO 9409-1 (tool flange) — TCP tanım standartı

Bu yolları sözleşme/lisans dahilinde elle araştırırız — URL'leri kayıt
altına almıyoruz çünkü erişim ve sürüm değişebilir.
