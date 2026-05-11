# Mühendislik Günlüğü

> Kararlar, sürpriz bulgular, çözülen sorunlar. Kronolojik (yeni → eski).

---

## 2026-05-11 — Faz 0 Başlangıç

### Bulgular

- **SoC tespit edildi:** Lavichip HN00-09Q6 üzerindeki `rk805 pwrkey` ve
  Mali GPU'su, SoC'un **Rockchip RK3568** olduğunu işaret ediyor. SDK
  manualindeki TI Sitara AM335x bilgileri **eski model** içindi; yeni
  HN00-09Q6 farklı.

- **Kernel:** `5.10.209-rt89 PREEMPT_RT`. Bu real-time kernel teach pendant
  için tasarlanmış — jog/safety thread'leri SCHED_FIFO priority alabilir.

- **Qt platformu:** `xcb` (X11). Manual'ın bahsettiği `eglfs` ve `linuxfb`
  yolları artık geçerli değil — modern XCB ile geliştireceğiz.

- **OS:** Ubuntu 20.04.6 LTS, standart bir Linux dağıtımı. `apt`, `systemd`,
  `netplan` mevcut. Bu, geliştirme akışını gemiştirir (özel BSP gerek yok).

- **SSH kullanıcı:** Manual `root` / `1234` der; gerçekte cihazda
  `Tronlong` / `<boş>`. Yeni imaj farklı kullanıcı standartı kullanıyor.
  Tronlong ismi SoM/board üreticisi Shenzhen Tronlong'a referans
  veriyor olabilir.

- **Network interface:** Aviation konektör → adapter box → RJ45 yolu
  `eth1` olarak gelir. `eth0/2/3` da tanımlı ama down. Yapılandırma
  netplan kontrolünde; `/etc/systemd/network/eth1.network` ile çelişki
  var (netplan kazanır).

- **Serial port:** Manual `/dev/ttyS1` der; HN00-09Q6'da `/dev/ttyS2`.

- **Mevcut uygulama:** `/userfs/app/lyx_appDemo` — fabrika demosu (41 KB
  Qt Widgets uygulaması).

### Kararlar

- **`.ai/` şablonu olarak CADNC örneği alındı** — modern, sade,
  doğrudan teknik. SODOO daha kurumsal; bizim daha yakın.
- **Dil çifti:** Kod İngilizce, kullanıcı iletişimi Türkçe (CADNC ile aynı).
- **Native build (Yol A)** Faz 1 için seçildi; cross-compile Faz 2+'da
  değerlendirilir.

### Sorular (kullanıcıdan beklenenler)

- Robotun fiziksel modeli/serisi (gerçek Fanuc mı, klon mu?)
- PLC'nin IP'si ne olacak?
- Pendant ↔ PLC arasındaki anahtarın internet'e bağlanması planı var mı?
- Robotun kinematik parametre dokümanı mevcut mu?
- E-Stop ve Enable Switch kabloları PLC'nin güvenlik girişine bağlandı mı?

---

(Bu dosya her önemli karar ve sürpriz bulguda güncellenir.)
