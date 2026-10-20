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
* **Solusi Arsitektur:** Menerapkan arsitektur **Dual-Sensor (Entry & Exit Clearance)**. Palang hanya diizinkan turun jika Sensor 2 (setelah palang) telah mendeteksi kendaraan DAN kemudian mendeteksi bahwa kendaraan telah tuntas menjauh (*clearance event*).

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
| **Ambang Batas Deteksi** | Jarak $< 100\text{ cm}$ | Objek berjarak $< 100\text{ cm}$ dianggap mobil; objek $> 100\text{ cm}$ dianggap lajur kosong. |
| **Filter Validasi Masuk** | $2000\text{ ms}$ (2 detik) | Mobil harus berhenti stabil $\ge 2$ detik di depan Sensor 1 sebelum palang membuka. |

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

| Komponen | Pin Modul | Pin Arduino | Konfigurasi Mode | Keterangan Rangkaian |
| :--- | :--- | :--- | :--- | :--- |
| **LCD 16×2 I²C** | SDA<br>SCL<br>VCC<br>GND | A4<br>A5<br>5V<br>GND | I²C Communication (Alamat default: `0x27`) | Penampil kuota slot dan instruksi pengguna. |
| **Sensor 1 (Entry)** | TRIG<br>ECHO<br>VCC<br>GND | D4<br>D5<br>5V<br>GND | `OUTPUT`<br>`INPUT`<br>5V<br>GND | Sensor ultrasonik pendeteksi kedatangan mobil. |
| **Sensor 2 (Exit)** | TRIG<br>ECHO<br>VCC<br>GND | D6<br>D7<br>5V<br>GND | `OUTPUT`<br>`INPUT`<br>5V<br>GND | Sensor ultrasonik verifikasi pelepasan lajur. |
| **Servo Motor** | PWM (Oranye)<br>V+ (Merah)<br>GND (Cokelat) | D9<br>5V<br>GND | `OUTPUT` (Timer PWM) | Di Wokwi berupa modul `wokwi-servo` (tuas putih). |
| **Tombol UP** | Pin 1<br>Pin 2 | D2<br>GND | `INPUT_PULLUP` | Menambah kapasitas slot maksimum (Active-LOW). |
| **Tombol DOWN** | Pin 1<br>Pin 2 | D8<br>GND | `INPUT_PULLUP` | Mengurangi kapasitas slot maksimum (Active-LOW). |
| **Tombol SET** | Pin 1<br>Pin 2 | D10<br>GND | `INPUT_PULLUP` | Tahan 3 dtk: Masuk/Keluar Config; Simpan EEPROM. |
| **LED Hijau** | Anoda (+)<br>Katoda (-) | D11 (via R)<br>GND | `OUTPUT` | Indikator akses terbuka (aman melintas). Resistor $220\,\Omega$. |
| **LED Merah** | Anoda (+)<br>Katoda (-) | D12 (via R)<br>GND | `OUTPUT` | Indikator berhenti/menutup/penuh. Resistor $220\,\Omega$. |
| **Buzzer** | Positif (+)<br>Negatif (-) | D13<br>GND | `OUTPUT` | Umpan balik audio (alarm, bip konfirmasi). |

---

## 5. LOGIKA SISTEM, HMI, & KONTROL AKTUSI

### 5.1 Evaluasi & Kompromi Logika Indikator LED
* **Keputusan:** Sistem menggunakan **Logika Akses Palang** sebagai fungsi utama LED, diperkaya dengan **Pola Kedipan (*Blink Pattern*)** untuk mengindikasikan status ketersediaan ruang parkir.

### 5.2 Matriks Status HMI & Aktuator

| Kondisi Sistem | Sudut Palang | LED Merah | LED Hijau | Pola Buzzer | Tampilan Layar LCD 16×2 |
| :--- | :---: | :---: | :---: | :---: | :--- |
| **IDLE (Ada Slot)** | 0° (Mendatar) | **ON Statis** | OFF | Senyap | `Baris 1: SLOT: XX/YY OPEN`<br>`Baris 2: SILAKAN MENDEKAT` |
| **WAIT_VALIDATION** | 0° (Mendatar) | **ON Statis** | OFF | Senyap | `Baris 1: MENDETEKSI...   `<br>`Baris 2: MOHON TUNGGU 2S ` |
| **OPENING** | 0° $\to$ 90° | Kedip Cepat | OFF | Bip Ritmik Cepat | `Baris 1: MEMBUKA GERBANG `<br>`Baris 2: HARAP BERHENTI  ` |
| **OPEN_WAIT** | 90° (Tegak) | OFF | **ON Statis** | Bip Pendek 2x | `Baris 1: SILAKAN MASUK   `<br>`Baris 2: HATI-HATI JALAN ` |
| **CLOSING** | 90° $\to$ 0° | Kedip Cepat | OFF | Bip Ritmik Kontinu| `Baris 1: MENUTUP GERBANG `<br>`Baris 2: AWAS PALANG TURUN` |
| **PARKING_FULL** | 0° (Mendatar) | Kedip Lambat | OFF | Bip Panjang 1x | `Baris 1: PARKIR PENUH!   `<br>`Baris 2: AKSES DITOLAK    ` |
| **CONFIG_MODE** | 0° (Terkunci) | Kedip Bareng | Kedip Bareng| Bip Klik Pendek | `Baris 1: ATUR MAX SLOT   `<br>`Baris 2: KAPASITAS: < XX >` |

### 5.3 Karakteristik Gerak Halus Servo (*Smooth Movement*)
Untuk menghindari hentakan mekanis yang merusak roda gigi servo SG90 dan menjaga representasi visual realistis di Wokwi:
* Palang tidak digerakkan instan dengan `servo.write(90)`.
* Sudut diperbarui bertahap: $1^\circ$ setiap $15\text{ ms}$ menggunakan interpolasi `millis()` non-blocking, menghasilkan durasi buka/tutup total sekitar $1.35\text{ detik}$.

---

## 6. SISTEM MANAJEMEN SLOT & MEMORI EEPROM

### 6.1 Persistensi Data EEPROM
* Nilai kapasitas maksimum disimpan pada alamat memori internal `EEPROM[0]`.
* Nilai default awal pabrik (*factory default*) adalah **10 slot**.
* Penulisan menggunakan fungsi `EEPROM.update(0, val)` yang hanya menulis sel memori jika nilai berubah, mencegah ausnya sel memori silikon mikrokontroler (batas toleransi $100.000$ siklus tulis).

### 6.2 Alur Transisi Mode Konfigurasi (*Safety Interlock*)
1. Mode Konfigurasi **hanya dapat diakses** saat palang berada di posisi `IDLE` (0°).
2. Tekan dan tahan tombol **SET (Pin 10)** selama minimal 3 detik untuk masuk ke menu pengaturan.
3. Saat masuk mode konfigurasi, servo terkunci mati di posisi 0°, dan pembacaan Sensor 1 & 2 diabaikan sementara.
4. Tekan tombol **UP (Pin 2)** untuk menambah kuota maksimum (maks. 99).
5. Tekan tombol **DOWN (Pin 8)** untuk mengurangi kuota maksimum (min. 1).
6. Tekan tombol **SET** sekali lagi untuk menyimpan nilai baru ke EEPROM dan sistem kembali ke mode operasional normal.

---

## 7. ALGORITMA SISTEM

1. **Inisialisasi Sistem:**
   * Konfigurasi I/O pin (Pin tombol sebagai `INPUT_PULLUP`).
   * Inisialisasi LCD 16×2, tampilkan splash screen selamat datang.
   * Baca nilai kapasitas maksimum dari `EEPROM[0]`. Jika data kosong/rusak ($>99$ atau $\le 0$), atur nilai default ke 10.
   * Gerakkan servo ke posisi awal $0^\circ$ (horizontal). Nyalakan LED Merah.
2. **Evaluasi Tombol Konfigurasi:**
   * Cek durasi penekanan tombol SET. Jika $\ge 3000\text{ ms}$ dan status gerbang `IDLE`, beralih ke `STATE_CONFIG`.
3. **Pemeriksaan Kapasitas Parkir:**
   * Hitung sisa slot aktif. Jika `slot <= 0`, ubah status ke `STATE_FULL`. LED Merah berkedip lambat. Palang dilarang membuka.
4. **Akuisisi Sensor 1 (Pendeteksian Kedatangan):**
   * Kirim pulsa ultrasonik pada Sensor 1. Baca waktu pantul echo.
   * Jika jarak terukur $< 100\text{ cm}$, catat timestamp awal deteksi `t_start`.
5. **Penyaringan Waktu (Presence Debouncing):**
   * Jika jarak terus bertahan $< 100\text{ cm}$ selama minimal $2000\text{ ms}$ terus-menerus:
     * Bunyikan buzzer 2 kali.
     * Ubah status ke `STATE_OPENING`.
   * Jika objek menghilang sebelum 2 detik tercapai, reset timer dan kembali ke standby.
6. **Mekanisme Pembukaan Gerbang:**
   * Naikkan sudut servo secara berkala dari $0^\circ$ ke $90^\circ$ ($1^\circ$ per $15\text{ ms}$).
   * LED Merah berkedip cepat saat bergerak.
   * Saat mencapai $90^\circ$, matikan LED Merah, nyalakan LED Hijau, alihkan ke `STATE_OPEN_WAIT`.
7. **Verifikasi Lintasan Mobil (Sensor 2):**
   * Di `STATE_OPEN_WAIT`, sistem menunggu mobil menutupi Sensor 2 (jarak $< 100\text{ cm}$).
   * Setelah Sensor 2 mendeteksi bodi mobil, sistem terus menunggu hingga Sensor 2 kembali bersih/bebas (jarak $> 100\text{ cm}$).
8. **Pengurangan Kuota & Penutupan Gerbang:**
   * Begitu Sensor 2 bersih:
     * Kurangi slot tersedia sebanyak 1 (`slot = slot - 1`).
     * Ubah status ke `STATE_CLOSING`.
     * Turunkan servo bertahap dari $90^\circ$ ke $0^\circ$.
     * Buzzer berbunyi ritmik berkala dan LED Merah berkedip cepat.
   * Saat servo mencapai $0^\circ$, nyalakan LED Merah statis dan kembali ke `STATE_IDLE`.

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
        <  Tombol SET Ditahan >───── Ya ───► [Masuk STATE_CONFIG]
        <       >= 3 Detik?   >                 │
         \───────────────────/                  ├─► Atur Slot (UP/DOWN)
                   │ Tidak                      ├─► Tekan SET Simpan EEPROM
                   ▼                            └─► Kembali ke Run Mode
         /───────────────────\                          │
        <    Sisa Slot <= 0?  >───── Ya ───► [STATE_FULL: Palang Terkunci 0°]
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
       │   Buzzer Beep 2x       │                          │
       │   STATE_OPENING:       │                          │
       │   Servo Naik 0° -> 90° │                          │
       └───────────┬────────────┘                          │
                   │                                       │
                   ▼                                       │
       ┌────────────────────────┐                          │
       │   STATE_OPEN_WAIT:     │                          │
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
       │   Slot = Slot - 1      │                          │
       │   STATE_CLOSING:       │                          │
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
  * Mengintegrasikan semua file *header* (`.h`) ke dalam skrip utama.
  * Mengonstruksi loop utama menggunakan Finite State Machine (`enum State {IDLE, WAIT_VALIDATION, OPENING, OPEN_WAIT, CLOSING, FULL, CONFIG}`).
  * Memprogram aktuasi servo SG90 secara bertahap (interpolasi non-blocking) di Pin D9.
  * Memimpin proses penggabungan kode dan validasi akhir sistem.

### Anggota 2: Sensor & Signal Conditioning Engineer
* **Fokus File:** `module_sensor.h`
* **Tanggung Jawab:**
  * Menulis algoritma pembacaan sensor ultrasonik `readDistance(trigPin, echoPin)`.
  * Membangun fungsi logika *presence debounce* 2 detik pada Sensor 1 untuk validasi kendaraan masuk.
  * Membangun fungsi pelacak kebebasan jalur pada Sensor 2 (*exit-clearance logic*).
  * Menghasilkan fungsi status yang bersih: `bool isVehicleWaiting()` dan `bool isVehicleCleared()`.

### Anggota 3: HMI & Visual Interface Engineer
* **Fokus File:** `module_display.h`
* **Tanggung Jawab:**
  * Menginisialisasi modul LCD 16×2 I²C (pustaka `LiquidCrystal_I2C`).
  * Membuat fungsi pembaruan layar bebas kedip (*flicker-free update*) yang hanya menyegarkan karakter teks jika ada perubahan data.
  * Memprogram keluaran LED Merah (D12) dan LED Hijau (D11) untuk status statis maupun berbagai mode kedipan (*fast blink* dan *slow blink*) menggunakan `millis()`.

### Anggota 4: Audio & Safety Alert Engineer
* **Fokus File:** `module_audio.h`
* **Tanggung Jawab:**
  * Mengendalikan modul buzzer pasif/aktif di Pin D13.
  * Menulis fungsi nada non-blocking tanpa fungsi `delay()`:
    * `playWelcomeTone()`: Dua nada frekuensi tinggi pendek tanda pembukaan gerbang.
    * `playClosingAlarm()`: Pola peringatan ritmik berulang saat palang sedang turun.
    * `playDeniedAlarm()`: Satu nada panjang berfrekuensi rendah tanda penolakan akses (slot penuh).
    * `playButtonBeep()`: Nada klik pendek untuk umpan balik penekanan tombol.

### Anggota 5: Memory Storage & Parameter Config Engineer
* **Fokus File:** `module_storage.h`
* **Tanggung Jawab:**
  * Memprogram pembacaan 3 Push Button di Pin 2 (UP), Pin 8 (DOWN), dan Pin 10 (SET) dengan filter *debounce*.
  * Mengelola status deteksi penekanan tombol SET selama 3 detik untuk aktivasi mode konfigurasi.
  * Menangani operasi baca/tulis memori non-volatile menggunakan pustaka `EEPROM.h` (`EEPROM.read` dan `EEPROM.update`).
  * Menyediakan variabel global `maxSlots` dan `occupiedSlots`.

### Anggota 6: Hardware Schematic, Layout & QA Test Engineer
* **Fokus File:** `diagram.json` & Dokumen Pengujian
* **Tanggung Jawab:**
  * Mengonfigurasi seluruh sirkuit pada file `diagram.json` Wokwi dengan peletakan komponen yang ergonomis dan warna kabel standar (Merah = 5V, Hitam = GND, Biru/Kuning = Sinyal Data).
  * Menyusun matriks pengujian kasus batas (*edge-case test suite*).
  * Melakukan pengujian sistematis di Wokwi untuk memverifikasi tidak adanya kondisi macet (*deadlock*) pada perangkat lunak.

---

## 11. MATRIKS PENGUJIAN SKENARIO BATAS (QA & EDGE CASES)

| ID Uji | Skenario Kondisi | Perilaku Sistem yang Diharapkan | Kriteria Lolos (Pass) |
| :---: | :--- | :--- | :--- |
| **TC-01** | Mobil terdeteksi di Sensor 1 hanya selama $1.2\text{ detik}$ lalu mundur. | Palang tetap tertutup (0°). Timer deteksi di-reset. LCD kembali ke menu IDLE. | Servo tidak bergeming; sistem kembali standby. |
| **TC-02** | Mobil terdeteksi di Sensor 1 stabil selama $2.1\text{ detik}$. | Buzzer berbunyi bip 2x, LED Hijau menyala, servo naik perlahan ke posisi 90°. | Palang terangkat penuh; LCD menampilkan "SILAKAN MASUK". |
| **TC-03** | Kuota parkir penuh (`slot == 0`). Mobil berdiri di depan Sensor 1. | Palang menolak membuka. LED Merah berkedip lambat. Buzzer berbunyi nada tolak. | Palang tetap 0°; LCD menampilkan "PARKIR PENUH!". |
| **TC-04** | Mobil berhenti tepat di tengah gerbang (menghalangi Sensor 2). | Palang tetap tertahan di atas (90°). Sistem menolak menutup sampai Sensor 2 bebas. | Palang tidak turun sebelum mobil tuntas lewat. |
| **TC-05** | Tombol SET ditekan saat palang sedang terangkat atau mobil sedang melintas. | Perintah diabaikan (*ignored*). Mode konfigurasi dilarang aktif saat jalur sibuk. | Mode konfigurasi hanya bisa diakses saat gerbang `IDLE`. |
| **TC-06** | Simulasi di-reset / daya mati mendadak saat slot disetel ke 25. | Saat menyala kembali, kapasitas slot tetap 25 (dibaca dari EEPROM). | Nilai konfigurasi tidak hilang setelah reset. |

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
   * Anggota 6 mengeksekusi skenario uji TC-01 hingga TC-06 pada proyek master untuk menjamin keandalan sistem secara terpadu.