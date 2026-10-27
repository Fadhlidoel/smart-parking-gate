# SMART PARKING GATE: SISTEM GERBANG PARKIR OTOMATIS BERBASIS MIKROKONTROLER
**Dokumen Spesifikasi Teknis, Desain Arsitektur, Logika FSM, dan Panduan Implementasi Tim**

---

## 1. DESKRIPSI & IDENTITAS PROYEK

* **Nama Proyek:** Smart Parking Gate: Sistem Gerbang Parkir Otomatis Berbasis Mikrokontroler
* **Platform Eksekusi:** Wokwi Simulator (Arduino Uno)
* **Paradigma Sistem:** Murni *Embedded System* (Non-IoT, Standalone Firmware, Non-blocking Architecture)
* **Target Penyelesaian:** Pertengahan November
* **Jumlah Anggota Tim:** 6 Orang

### 1.1 Skenario Operasional
Sistem diterapkan pada area akses masuk gedung perkantoran atau kampus. Kendaraan yang tiba di depan gerbang akan dideteksi oleh sensor ultrasonik. Untuk membedakan kendaraan yang benar-benar berniat masuk dengan objek yang hanya melintas sekilas (seperti pejalan kaki atau hewan), sistem menerapkan penundaan validasi (*presence filter*) selama 2 detik. 

Setelah tervalidasi dan kuota parkir masih tersedia, mikrokontroler memerintahkan motor servo membuka palang secara bertahap (halus). Setelah seluruh bodi kendaraan melewati palang (diverifikasi oleh sensor kedua), palang akan turun kembali secara otomatis. Seluruh status sistem diinformasikan secara visual melalui LCD 16×2 I²C dan indikator LED dua warna, serta notifikasi audio melalui buzzer. Sistem juga dilengkapi fitur pengaturan kapasitas slot maksimum berbasis tombol fisik yang tersimpan secara permanen pada memori internal EEPROM.

---

## 2. EVALUASI KRITIS & DASAR KEPUTUSAN DESAIN

### 2.1 Masalah Sensor Tunggal (*Single-Sensor Blind Spot*)
* **Celah Logika Asal:** Jika hanya menggunakan satu sensor ultrasonik di depan gerbang, saat bodi tengah atau belakang kendaraan berada tepat di bawah palang, moncong mobil telah keluar dari kerucut pancaran sensor (*beam angle*). Sensor akan membaca status kosong (*out of range*) dan mikrokontroler akan langsung menurunkan palang, menyebabkan benturan fisik pada atap kendaraan (*safety hazard*).
* **Solusi Arsitektur:** Menerapkan arsitektur **Dual-Sensor (Entry & Exit Clearance)**. Palang hanya diizinkan turun jika Sensor 2 (setelah palang) telah mendeteksi kendaraan DAN kemudian mendeteksi bahwa kendaraan telah tuntas menjauh (*clearance event*). Kondisi "tuntas menjauh" baru diakui setelah Sensor 2 kosong **stabil selama 1 detik** (`DURASI_CLEARANCE`), sehingga satu pembacaan gema yang hilang tidak langsung menurunkan palang.

### 2.2 Sifat Akustik Sensor HC-SR04 & Orientasi Sudut
* **Permasalahan Pantulan Miring:** Sensor ultrasonik mengukur jarak berdasarkan pantulan gelombang suara 40 kHz. Bumper depan mobil yang aerodinamis atau miring memantulkan gelombang ke sudut lain (*specular reflection*), memicu pembacaan jarak acak (*false negative*).
* **Permasalahan Saling Ganggu (*Acoustic Crosstalk*):** Jika dua sensor diletakkan saling berhadapan membujur di jalur lintasan, pancaran dari Sensor 1 dapat diterima langsung oleh receiver Sensor 2.
* **Solusi Geometri:** Kedua sensor dipasang di sisi trotoar dengan **arah tembak menyamping tegak lurus (90°)** ke bodi pintu mobil. Bidang pintu mobil yang luas dan vertikal menjamin pantulan gema akustik kembali secara presisi ke receiver.

### 2.3 Keputusan Platform Simulasi & Batasan Non-IoT
* **Wokwi vs Proteus/LabVIEW:** Wokwi dipilih karena memiliki integrasi modul I²C, servo, dan sensor ultrasonik yang stabil berbasis browser, tidak memerlukan lisensi berbayar, bebas instalasi library pihak ketiga, dan format rangkaian berbasis JSON memudahkan kolaborasi tim.
* **Penolakan Integrasi Python/Blender:** Penggunaan Python (PC) atau 3D Blender untuk mengontrol logika gerbang ditolak agar tidak mengaburkan penilaian dosen. Sistem dirancang 100% *embedded firmware* yang berjalan mandiri di dalam mikrokontroler ATmega328P.

---

## 3. ARSITEKTUR FISIK & VISUALISASI TATA LETAK

### 3.1 Diagram Tata Letak Jalur (*Physical Layout*)

```text
       [ Trotoar Kiri ]
   -------------------------------------------------
     [Sensor 1]             [PALANG]            [Sensor 2]
         │ (Arah tembak 90°)   │                    │ (Arah tembak 90°)
         ▼                     ▼                    ▼
   ═════════════════════════════════════════════════  --> Arah Gerak Mobil
   -------------------------------------------------
       [ Trotoar Kanan ]
```

### 3.2 Spesifikasi Geometri & Parameter Deteksi

| Parameter | Dimensi / Nilai | Penjelasan Teknis |
| :--- | :--- | :--- |
| **Lebar Lajur Gerbang** | $\pm 2.5 - 3.0\text{ meter}$ | Standar lajur satu mobil. |
| **Posisi Sensor 1 (Entry)** | $1.5 - 2.0\text{ meter}$ sebelum palang | Memberikan ruang henti aman (*safe braking distance*) saat palang bergerak naik. |
| **Posisi Sensor 2 (Exit)** | $1.5 - 2.0\text{ meter}$ setelah palang | Memastikan bumper belakang kendaraan (panjang mobil $3.5 - 4.5\text{ m}$) telah bebas sepenuhnya dari ayunan palang. |
| **Arah Hadap Sensor** | $90^\circ$ Melintang | Menembak sisi pintu samping kendaraan. |
| **Ambang Batas Deteksi** | Jarak $\le 100\text{ cm}$ | Objek berjarak $\le 100\text{ cm}$ dianggap mobil; objek $> 100\text{ cm}$ atau di luar jangkauan baca dianggap lajur kosong. Berlaku sama untuk Sensor 1 dan Sensor 2. |
| **Filter Validasi Masuk** | $2000\text{ ms}$ (2 detik) | Mobil harus berhenti stabil $\ge 2$ detik di depan Sensor 1 sebelum palang membuka. |
| **Filter Clearance Keluar** | $1000\text{ ms}$ (1 detik) | Setelah Sensor 2 terhalang, sensor harus kosong stabil $\ge 1$ detik sebelum palang menutup. Jika terhalang lagi, hitungan diulang. |
| **Batas Tunggu Lewat** | $30000\text{ ms}$ (30 detik) | Jika Sensor 2 tidak pernah terhalang dalam 30 detik sejak palang terbuka penuh, palang menutup otomatis tanpa menambah slot terisi. |

---

## 4. ANGGARAN & PEMETAAN PIN ARDUINO UNO (HARDWARE BUDGETING)

Semua pin I/O mikrokontroler ATmega328P dialokasikan secara efisien tanpa memerlukan IC expander:

```text
                  ┌──────────────────────┐
                  │     ARDUINO UNO      │
                  ├──────────────────────┤
                  │                      │
       [TX] D0 ───┤                      ├─── A5 [SCL] ─── LCD 16x2 I2C
       [RX] D1 ───┤                      ├─── A4 [SDA] ─── LCD 16x2 I2C
  Btn UP (D2) ───┤                      ├─── A3 (Cadangan)
      (D3 NC) ───┤                      ├─── A2 (Cadangan)
Sensor 1 TRIG ───┤ D4                   ├─── A1 (Cadangan)
Sensor 1 ECHO ───┤ D5                   ├─── A0 (Cadangan)
Sensor 2 TRIG ───┤ D6                   │
Sensor 2 ECHO ───┤ D7                   ├─── VIN
Btn DOWN (D8) ───┤ D8                   ├─── 5V  ─── Jalur Daya VCC
   Servo (D9) ───┤ D9 (PWM)             ├─── 3V3
  Btn SET (D10)──┤ D10                  ├─── GND ─── Ground Bersama
LED Hijau (D11)──┤ D11                  └─── RST
LED Merah (D12)──┤ D12
   Buzzer (D13)──┤ D13
                  └──────────────────────┘
```

### 4.1 Tabel Detail Pengkabelan

| Komponen | Pin Modul | Pin Arduino | Konstanta Pin | Konfigurasi Mode | Keterangan Rangkaian |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **LCD 16×2 I²C** | SDA<br>SCL<br>VCC<br>GND | A4<br>A5<br>5V<br>GND | `ALAMAT_LCD` (`0x27`) | I²C Communication | Penampil kuota slot dan instruksi pengguna. |
| **Sensor 1 (Masuk)** | TRIG<br>ECHO<br>VCC<br>GND | D4<br>D5<br>5V<br>GND | `PIN_TRIG_MASUK`<br>`PIN_ECHO_MASUK` | `OUTPUT`<br>`INPUT`<br>5V<br>GND | Sensor ultrasonik pendeteksi kedatangan mobil. |
| **Sensor 2 (Keluar)** | TRIG<br>ECHO<br>VCC<br>GND | D6<br>D7<br>5V<br>GND | `PIN_TRIG_KELUAR`<br>`PIN_ECHO_KELUAR` | `OUTPUT`<br>`INPUT`<br>5V<br>GND | Sensor ultrasonik verifikasi pelepasan lajur. |
| **Servo Motor** | PWM (Oranye)<br>V+ (Merah)<br>GND (Cokelat) | D9<br>5V<br>GND | `PIN_SERVO` | `OUTPUT` (PWM) | Di Wokwi berupa modul `wokwi-servo` (tuas putih). |
| **Tombol UP** | Pin 1<br>Pin 2 | D2<br>GND | `PIN_TOMBOL_UP` | `INPUT_PULLUP` | Menambah kapasitas slot maksimal (Active-LOW). |
| **Tombol DOWN** | Pin 1<br>Pin 2 | D8<br>GND | `PIN_TOMBOL_DOWN` | `INPUT_PULLUP` | Mengurangi kapasitas slot maksimal (Active-LOW). |
| **Tombol SET** | Pin 1<br>Pin 2 | D10<br>GND | `PIN_TOMBOL_SET` | `INPUT_PULLUP` | Tahan 3 dtk: Masuk Config. Tekan singkat saat Config: Simpan EEPROM & Keluar. |
| **LED Hijau** | Anoda (+)<br>Katoda (-) | D11 (via R)<br>GND | `PIN_LED_HIJAU` | `OUTPUT` | Indikator akses terbuka (aman melintas). Resistor $220\,\Omega$. |
| **LED Merah** | Anoda (+)<br>Katoda (-) | D12 (via R)<br>GND | `PIN_LED_MERAH` | `OUTPUT` | Indikator berhenti/menutup/penuh. Resistor $220\,\Omega$. |
| **Buzzer** | Positif (+)<br>Negatif (-) | D13<br>GND | `PIN_BUZZER` | `OUTPUT` | Umpan balik audio (alarm, bip konfirmasi). |

---

## 5. LOGIKA SISTEM, HMI, & KONTROL AKTUSI

### 5.1 Evaluasi & Kompromi Logika Indikator LED
* **Keputusan:** Sistem menggunakan **Logika Akses Palang** sebagai fungsi utama LED, diperkaya dengan **Pola Kedipan (*Blink Pattern*)** untuk mengindikasikan status ketersediaan ruang parkir.

### 5.2 Matriks Status HMI & Aktuator

| Status FSM (`StatusSistem`) | Sudut Palang | LED Merah | LED Hijau | Pola Buzzer | Tampilan Layar LCD 16×2 |
| :--- | :---: | :---: | :---: | :---: | :--- |
| **STATUS_SIAGA** | 0° (Mendatar) | **ON Statis** | OFF | Senyap | `Baris 1: SLOT: XX/YY OPEN`<br>`Baris 2: SILAKAN MENDEKAT` |
| **STATUS_VALIDASI** | 0° (Mendatar) | **ON Statis** | OFF | Senyap | `Baris 1: MENDETEKSI...   `<br>`Baris 2: MOHON TUNGGU 2S ` |
| **STATUS_BUKA** | 0° $\to$ 90° | Kedip Cepat | OFF | Bip Ritmik Cepat | `Baris 1: MEMBUKA GERBANG `<br>`Baris 2: HARAP BERHENTI  ` |
| **STATUS_LEWAT** | 90° (Tegak) | OFF | **ON Statis** | Bip Pendek 2x | `Baris 1: SILAKAN MASUK   `<br>`Baris 2: HATI-HATI JALAN ` |
| **STATUS_TUTUP** | 90° $\to$ 0° | Kedip Cepat | OFF | Bip Ritmik Kontinu| `Baris 1: MENUTUP GERBANG `<br>`Baris 2: PALANG TURUN!   ` |
| **STATUS_PENUH** | 0° (Mendatar) | Kedip Lambat | OFF | Bip Panjang 1x | `Baris 1: PARKIR PENUH!   `<br>`Baris 2: AKSES DITOLAK    ` |
| **STATUS_KONFIG** | 0° (Terkunci) | Kedip Bareng | Kedip Bareng| Bip Klik Pendek | `Baris 1: ATUR MAX SLOT   `<br>`Baris 2: KAPASITAS: <XX> ` |

### 5.3 Karakteristik Gerak Halus Servo (*Smooth Movement*)
Untuk menghindari hentakan mekanis yang merusak roda gigi servo SG90 dan menjaga representasi visual realistis di Wokwi:
* Palang tidak digerakkan instan dengan `servo.write(90)`.
* Sudut diperbarui bertahap: $1^\circ$ setiap $15\text{ ms}$ menggunakan interpolasi `millis()` non-blocking, menghasilkan durasi buka/tutup total sekitar $1.35\text{ detik}$.

### 5.4 Batasan Arsitektur Non-blocking
* Seluruh logika di dalam `loop()` (FSM, servo, LED, LCD, tombol) berbasis `millis()` tanpa `delay()`.
* Pengecualian yang disengaja:
  * `tampilkanSplash()` memakai `delay()` sekitar 3,3 detik, **hanya sekali** di `setup()` sebelum `loop()` berjalan.
  * `bacaJarak()` memakai `pulseIn()` yang menunggu pantulan maksimal `TIMEOUT_ECHO` (15 ms) per pembacaan. Jeda ini pendek dan terbatas, sehingga siklus loop tetap responsif. Akibat sampingnya, jangkauan baca maksimal sekitar 257 cm; di atas itu dianggap lajur kosong.

---

## 6. SISTEM MANAJEMEN SLOT & MEMORI EEPROM

### 6.1 Persistensi Data EEPROM
* Nilai kapasitas maksimum disimpan pada alamat memori internal `ALAMAT_EEPROM` (indeks 0).
* Nilai bawaan awal (*default*) adalah `SLOT_DEFAULT` (10 slot), dengan batas `SLOT_MIN` (1) hingga `SLOT_MAX` (99).
* Penulisan menggunakan fungsi `simpanEeprom(nilaiBaru)` (berbasis `EEPROM.update`) yang hanya menulis sel memori jika nilai berubah untuk mencegah keausan memori silikon.

### 6.2 Alur Transisi Mode Konfigurasi (*Safety Interlock*)
1. Mode Konfigurasi **hanya dapat diakses** saat palang diam tertutup di 0°, yaitu pada `STATUS_SIAGA` atau `STATUS_PENUH`. Akses dari `STATUS_PENUH` diperlukan karena itu satu-satunya cara menambah kuota saat parkir penuh.
2. Tekan dan tahan tombol **SET (`PIN_TOMBOL_SET` / Pin 10)** selama minimal `TAHAN_CONFIG` (3 detik) untuk masuk ke menu pengaturan (`STATUS_KONFIG`).
3. Saat masuk mode konfigurasi, servo terkunci di posisi 0° (`SUDUT_TUTUP`), dan pembacaan Sensor 1 & 2 diabaikan sementara.
4. Tekan tombol **UP (`PIN_TOMBOL_UP` / Pin 2)** untuk menambah kuota maksimum (maks. `SLOT_MAX` = 99).
5. Tekan tombol **DOWN (`PIN_TOMBOL_DOWN` / Pin 8)** untuk mengurangi kuota maksimum (min. `SLOT_MIN` = 1).
6. Tekan tombol **SET** sekali lagi (`cekTombolSet()`) untuk menyimpan nilai baru ke EEPROM dan sistem kembali ke mode `STATUS_SIAGA`.

### 6.3 Batasan Penghitungan Slot
* Versi ini hanya memiliki gerbang masuk. Belum ada sensor atau pemicu untuk mobil keluar area parkir, sehingga `kurangiSlot()` disiapkan tetapi belum dipanggil.
* `slotTerisi` tidak disimpan di EEPROM dan kembali ke 0 setiap sistem dinyalakan ulang.
* Keluar dari `STATUS_PENUH` hanya bisa dengan menaikkan kapasitas melalui menu konfigurasi, atau dengan restart sistem.

---

## 7. ALGORITMA SISTEM

1. **Inisialisasi Sistem:**
   * Konfigurasi I/O pin (Pin tombol sebagai `INPUT_PULLUP`).
   * Inisialisasi LCD 16×2, panggil `tampilkanSplash()`.
   * Panggil `bacaEeprom()`. Jika data kosong/rusak ($>99$ atau $\le 0$), atur nilai ke `SLOT_DEFAULT` (10).
   * Gerakkan servo ke posisi awal `SUDUT_TUTUP` ($0^\circ$). Nyalakan LED Merah. Status diset ke `STATUS_SIAGA`.
2. **Evaluasi Tombol Konfigurasi:**
   * Cek penekanan tombol SET via `cekTombolSetTahan()`. Jika terpenuhi ($\ge 3000\text{ ms}$) dan status gerbang `STATUS_SIAGA` atau `STATUS_PENUH` (palang di 0°), beralih ke `STATUS_KONFIG`.
3. **Pemeriksaan Kapasitas Parkir:**
   * Hitung sisa slot aktif via `hitungSisaSlot()`. Jika sisa slot $\le 0$, ubah status ke `STATUS_PENUH`. LED Merah berkedip lambat (`KEDIP_LAMBAT`). Palang dilarang membuka.
4. **Akuisisi Sensor Masuk (Sensor 1):**
   * Baca jarak melalui `bacaJarak(PIN_TRIG_MASUK, PIN_ECHO_MASUK)`.
   * Jika jarak terukur $\le \text{JARAK\_DETEKSI}$ ($100\text{ cm}$), reset sisa status validasi lama via `resetValidasiMasuk()`, lalu mulai hitung waktu stabil.
5. **Penyaringan Waktu (Presence Debouncing):**
   * Jika objek stabil bertahan $\le 100\text{ cm}$ selama minimal $\text{DURASI\_VALIDASI}$ ($2000\text{ ms}$) via `cekMobilMasuk()`:
     * Bunyikan buzzer melalui `bunyiMasuk()`.
     * Ubah status ke `STATUS_BUKA`.
   * Jika objek menghilang sebelum 2 detik, panggil `resetValidasiMasuk()` dan kembali siaga.
6. **Mekanisme Pembukaan Gerbang:**
   * Naikkan sudut servo bertahap dari `SUDUT_TUTUP` ($0^\circ$) ke `SUDUT_BUKA` ($90^\circ$) dengan jeda `JEDA_SERVO` ($15\text{ ms}$).
   * LED Merah berkedip cepat (`KEDIP_CEPAT`) saat palang bergerak.
   * Saat mencapai $90^\circ$, matikan LED Merah, nyalakan LED Hijau, reset status Sensor 2 via `resetSensorKeluar()`, catat waktu mulai, lalu alihkan ke `STATUS_LEWAT`.
7. **Verifikasi Lintasan Mobil (Sensor Keluar / Sensor 2):**
   * Di `STATUS_LEWAT`, sistem menunggu mobil menutupi Sensor 2 lalu kembali tuntas lewat (`cekMobilLewat()`).
   * "Tuntas lewat" berarti Sensor 2 kosong **stabil selama 1 detik** (`DURASI_CLEARANCE`). Jika Sensor 2 terhalang lagi sebelum 1 detik, hitungan diulang dari nol.
   * **Batas tunggu (`TIMEOUT_LEWAT`, 30 detik):** jika sejak palang terbuka penuh Sensor 2 **tidak pernah** terhalang selama 30 detik, palang menutup otomatis (`STATUS_TUTUP`) **tanpa** `tambahSlot()`. Begitu Sensor 2 pernah terhalang, batas tunggu ini tidak berlaku lagi sehingga palang tidak akan turun saat mobil masih di area Sensor 2.
8. **Pengurangan Kuota & Penutupan Gerbang:**
   * Begitu Sensor 2 tuntas terlewati (kosong stabil 1 detik):
     * Tambah hitungan mobil masuk via `tambahSlot()`.
     * Ubah status ke `STATUS_TUTUP`.
     * Turunkan servo bertahap dari $90^\circ$ ke $0^\circ$.
     * Alarm audio aktif (`bunyiGerak()`) dan LED Merah berkedip cepat.
   * Saat servo mencapai $0^\circ$, nyalakan LED Merah statis dan kembali ke `STATUS_SIAGA`.

---

## 8. FLOWCHART SISTEM

```text
       ┌────────────────────────┐
       │         START          │
       └───────────┬────────────┘
                   │
                   ▼
       ┌────────────────────────┐
       │ Inisialisasi Hardware  │
       │ Baca EEPROM & Servo 0° │
       └───────────┬────────────┘
                   │
                   ▼
         /───────────────────\
        <  Tombol SET Ditahan >───── Ya ───► [STATUS_KONFIG]
        <   >= 3 Detik?       >                 │
         \───────────────────/                  ├─► Atur Slot (UP/DOWN)
                   │ Tidak                      ├─► Tekan SET Simpan EEPROM
                   ▼                            └─► Kembali ke Run Mode
         /───────────────────\                          │
        <    Sisa Slot <= 0?  >───── Ya ───► [STATUS_PENUH: Palang Terkunci 0°]
         \───────────────────/                  │ (LED Merah Kedip Lambat)
                   │ Tidak                      └──────────┐
                   ▼                                       │
         /───────────────────\                             │
        <   Sensor 1 < 100cm? >───── Tidak ────────────────┤
         \───────────────────/                             │
                   │ Ya                                    │
                   ▼                                       │
         /───────────────────\                             │
        <   Stabil >= 2 Detik?>───── Tidak ─► [Reset Timer]│
         \───────────────────/                             │
                   │ Ya                                    │
                   ▼                                       │
       ┌────────────────────────┐                          │
       │   Bunyi Bip 2x         │                          │
       │   STATUS_BUKA:         │                          │
       │   Servo Naik 0° -> 90° │                          │
       └───────────┬────────────┘                          │
                   │                                       │
                   ▼                                       │
       ┌────────────────────────┐                          │
       │   STATUS_LEWAT:        │                          │
       │   LED Hijau ON,        │                          │
       │   Palang Tertahan 90°  │                          │
       └───────────┬────────────┘                          │
                   │                                       │
                   ▼                                       │
         /───────────────────\                             │
        <  Sensor 2 < 100cm?  >───── Tidak ─► (Tunggu Mobil Masuk)
         \───────────────────/                             │
                   │ Ya (Mobil Sedang Lewat)               │
                   ▼                                       │
         /───────────────────\                             │
        <  Sensor 2 > 100cm?  >───── Tidak ─► (Tunggu Mobil Bebas)
         \───────────────────/                             │
                   │ Ya (Bodi Mobil Bebas Palang)          │
                   ▼                                       │
       ┌────────────────────────┐                          │
       │   Tambah Slot Terisi   │                          │
       │   STATUS_TUTUP:        │                          │
       │   Servo Turun 90° -> 0°│                          │
       │   Alarm Audio Aktif    │                          │
       └───────────┬────────────┘                          │
                   │                                       │
                   ▼                                       │
       ┌────────────────────────┐                          │
       │   Palang Tertutup (0°) │                          │
       │   LED Merah ON Statis  │                          │
       └───────────┬────────────┘                          │
                   │                                       │
                   └───────────────────────────────────────┘
                                   │
                                   ▼
                         [Ulangi Siklus Loop]
```

**Catatan revisi flowchart:**
* Cabang *"Sensor 2 > 100cm?"* baru dianggap **Ya** setelah Sensor 2 kosong **stabil selama 1 detik** (`DURASI_CLEARANCE`). Jika terhalang lagi sebelum 1 detik, kembali ke *"Tunggu Mobil Bebas"*.
* Pada cabang *"Tunggu Mobil Masuk"* (Sensor 2 belum pernah terhalang), jika sudah 30 detik sejak palang terbuka penuh (`TIMEOUT_LEWAT`), sistem langsung ke `STATUS_TUTUP` **tanpa** menambah slot terisi.
* Dari `STATUS_PENUH`, tombol SET yang ditahan 3 detik juga membuka `STATUS_KONFIG` (palang tetap 0°).

---

## 9. STRUKTUR FILE MODULAR WOKWI

Untuk memastikan keenam anggota tim dapat bekerja secara independen tanpa benturan revisi, sistem dibagi ke dalam struktur file modular di Wokwi:

```text
smart-parking-gate/
├── diagram.json          (Konfigurasi sirkuit & visual koordinat kanvas Wokwi)
├── sketch.ino            (File program utama: Setup, Loop, & State Machine FSM)
├── config.h              (Definisi Pin, konstanta jarak, struktur data status)
├── module_sensor.h       (Logika pembacaan HC-SR04 & filter debouncing 2 detik)
├── module_display.h      (Logika teks LCD 16x2 I2C & kendali indikator LED)
├── module_audio.h        (Logika pola nada buzzer frekuensi non-blocking)
└── module_storage.h      (Logika debouncing 3 tombol & manajemen memori EEPROM)
```

---

## 10. PEMBAGIAN KERJA & TANGGUNG JAWAB TIM (6 ANGGOTA)

### Anggota 1: Ketua Tim / System Architect
* **Fokus File:** `sketch.ino`
* **Tanggung Jawab:**
  * Mengintegrasikan semua file header (`.h`) ke dalam skrip utama.
  * Mengonstruksi loop utama menggunakan Finite State Machine (`enum StatusSistem { STATUS_SIAGA, STATUS_VALIDASI, STATUS_BUKA, STATUS_LEWAT, STATUS_TUTUP, STATUS_PENUH, STATUS_KONFIG }`).
  * Memprogram aktuasi servo pada pin `PIN_SERVO` (D9) secara bertahap dari `SUDUT_TUTUP` (0°) hingga `SUDUT_BUKA` (90°) dengan interpolasi `JEDA_SERVO` non-blocking.
  * Mengelola variabel global sistem: `statusSaatIni` dan `sudutPalang`.
  * Memimpin proses penggabungan kode dan validasi akhir sistem.

### Anggota 2: Sensor & Signal Conditioning Engineer
* **Fokus File:** `module_sensor.h`
* **Tanggung Jawab:**
  * Menulis fungsi inisialisasi pin `inisialisasiSensor()`.
  * Menulis algoritma pembacaan sensor ultrasonik `bacaJarak(pinTrig, pinEcho)`.
  * Membangun logika validasi mobil masuk `cekMobilMasuk()` dengan filter `DURASI_VALIDASI` (2 detik) pada `PIN_TRIG_MASUK` & `PIN_ECHO_MASUK`.
  * Membangun logika pelacak penyelesaian lajur `cekMobilLewat()` pada `PIN_TRIG_KELUAR` & `PIN_ECHO_KELUAR`, dengan filter kosong stabil `DURASI_CLEARANCE` (1 detik).
  * Menyediakan fungsi reset timer validasi `resetValidasiMasuk()`.
  * Menyediakan `resetSensorKeluar()` dan `sensorKeluarPernahTerdeteksi()` sebagai dasar timeout `TIMEOUT_LEWAT` (30 detik) di `STATUS_LEWAT`.

### Anggota 3: HMI & Visual Interface Engineer
* **Fokus File:** `module_display.h`
* **Tanggung Jawab:**
  * Menginisialisasi LCD 16×2 I²C melalui fungsi `inisialisasiDisplay()`.
  * Membuat fungsi pembaruan layar bebas kedip `perbaruiLcd(status, sisaSlot, totalSlot)` berbasis `REFRESH_LCD`.
  * Memprogram kendali LED Merah (`PIN_LED_MERAH`) dan LED Hijau (`PIN_LED_HIJAU`) melalui `perbaruiLed(status)` untuk mode statis maupun kedip (`KEDIP_CEPAT` dan `KEDIP_LAMBAT`) menggunakan `millis()`.
  * Menampilkan layar pembuka `tampilkanSplash()` dan menu teknisi `tampilkanMenuConfig(nilaiSementara)`.

### Anggota 4: Audio & Safety Alert Engineer
* **Fokus File:** `module_audio.h`
* **Tanggung Jawab:**
  * Mengendalikan modul buzzer pada pin `PIN_BUZZER` (D13) melalui `inisialisasiAudio()`.
  * Menulis fungsi umpan balik audio non-blocking tanpa `delay()`:
    * `bunyiMasuk()`: Bip 2x pendek konfirmasi validasi mobil masuk.
    * `bunyiGerak()`: Alarm ritmik berkala saat palang sedang bergerak turun.
    * `bunyiPenuh()`: Satu nada panjang tanda penolakan akses (slot penuh).
    * `bunyiKlik()`: Nada klik pendek untuk umpan balik penekanan tombol.
    * `perbaruiAudio()`: Dipanggil di loop utama untuk pemrosesan ritme audio.
    * `matikanAudio()`: Mematikan sinyal buzzer seketika.

### Anggota 5: Memory Storage & Parameter Config Engineer
* **Fokus File:** `module_storage.h`
* **Tanggung Jawab:**
  * Menyiapkan fungsi inisialisasi tombol dan memori `inisialisasiStorage()`.
  * Memprogram pembacaan 3 tombol dengan filter `DEBOUNCE_TOMBOL`: `cekTombolUp()`, `cekTombolDown()`, dan `cekTombolSet()`.
  * Mengelola deteksi penekanan tombol SET selama `TAHAN_CONFIG` (3 detik) melalui `cekTombolSetTahan()`.
  * Menangani operasi EEPROM: `bacaEeprom()` dan `simpanEeprom(nilaiBaru)` pada alamat `ALAMAT_EEPROM`.
  * Mengelola variabel kuota parkir: `kapasitasMaks`, `slotTerisi`, `hitungSisaSlot()`, `tambahSlot()`, dan `kurangiSlot()`.

### Anggota 6: Hardware Schematic, Layout & QA Test Engineer
* **Fokus File:** `diagram.json` & Dokumen Pengujian
* **Tanggung Jawab:**
  * Mengonfigurasi seluruh sirkuit pada file `diagram.json` Wokwi dengan peletakan komponen yang ergonomis dan warna kabel standar (Merah = 5V, Hitam = GND, Biru = SDA, Kuning = SCL, Hijau/Oranye = Sinyal, Ungu = PWM/Audio).
  * Menyusun matriks pengujian kasus batas (*edge-case test suite*).
  * Melakukan pengujian sistematis di Wokwi untuk memverifikasi keandalan sistem tanpa kondisi macet (*deadlock*).

---

## 11. MATRIKS PENGUJIAN SKENARIO BATAS (QA & EDGE CASES)

| ID Uji | Skenario Kondisi | Perilaku Sistem yang Diharapkan | Kriteria Lolos (Pass) |
| :---: | :--- | :--- | :--- |
| **TC-01** | Mobil terdeteksi di Sensor Masuk < 2 detik lalu mundur. | Palang tetap tertutup (0°). Timer deteksi di-reset via `resetValidasiMasuk()`. LCD kembali ke `STATUS_SIAGA`. | Servo tidak bergeming; sistem kembali siaga. |
| **TC-02** | Mobil terdeteksi di Sensor Masuk stabil $\ge 2\text{ detik}$. | Buzzer membunyikan `bunyiMasuk()`, LED Hijau ON, servo naik perlahan ke posisi 90° (`STATUS_BUKA`). | Palang terangkat penuh; LCD menampilkan "SILAKAN MASUK". |
| **TC-03** | Kuota parkir penuh (`hitungSisaSlot() <= 0`). Mobil berdiri di depan Sensor Masuk. | Palang menolak membuka (`STATUS_PENUH`). LED Merah kedip lambat. Buzzer membunyikan `bunyiPenuh()`. | Palang tetap 0°; LCD menampilkan "PARKIR PENUH!". |
| **TC-04** | Mobil berhenti tepat di tengah gerbang (menghalangi Sensor Keluar). | Palang tetap tertahan di atas (90° / `STATUS_LEWAT`). Sistem menolak menutup sampai sensor bebas. | Palang tidak turun sebelum mobil tuntas lewat (`cekMobilLewat()`). |
| **TC-05** | Tombol SET ditekan saat palang sedang terangkat atau mobil sedang melintas. | Perintah diabaikan (*Safety Interlock*). Menu konfigurasi dilarang aktif saat jalur sibuk. | Mode konfigurasi (`STATUS_KONFIG`) hanya bisa diakses saat `STATUS_SIAGA` atau `STATUS_PENUH` dengan palang di 0°. |
| **TC-06** | Simulasi di-reset / daya mati mendadak saat slot disetel ke 25. | Saat menyala kembali, kapasitas slot tetap 25 (dibaca dari `ALAMAT_EEPROM` via `bacaEeprom()`). | Nilai konfigurasi tidak hilang setelah reset. |
| **TC-07** | Saat `STATUS_LEWAT`, Sensor 2 terhalang lalu kosong kurang dari 1 detik, kemudian terhalang lagi. | Palang tetap 90°. Hitungan clearance diulang setiap kali Sensor 2 terhalang lagi. | Palang baru turun setelah Sensor 2 kosong stabil $\ge 1$ detik. |
| **TC-08** | Palang terbuka penuh, tetapi tidak ada objek di Sensor 2 selama 30 detik. | Sistem menutup palang otomatis (`STATUS_TUTUP`) dan kembali ke `STATUS_SIAGA`. | Palang turun setelah 30 detik; slot terisi **tidak** bertambah. |

---

## 12. PANDUAN KOLABORASI WOKWI (WORKFLOW)

1. **Pembuatan Master Repository:**
   * Anggota 1 membuat satu proyek awal di situs Wokwi, menyalin kode `diagram.json` yang telah dirancang Anggota 6, lalu membagikan URL proyek ke seluruh anggota.
2. **Pengerjaan Independen:**
   * Anggota 2 sampai 5 membuka proyek master, mengklik **"Save a copy"** ke akun masing-masing, dan menguji modulnya secara terisolasi.
3. **Integrasi Kode:**
   * Pada proyek master, Anggota 1 membuat tab file baru (*New File*) sesuai struktur modular: `config.h`, `module_sensor.h`, `module_display.h`, `module_audio.h`, dan `module_storage.h`.
   * Salin kode yang telah divalidasi oleh tiap anggota ke tab masing-masing.
4. **Verifikasi Akhir:**
   * Anggota 6 mengeksekusi skenario uji TC-01 hingga TC-08 pada proyek master untuk menjamin keandalan sistem secara terpadu.