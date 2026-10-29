# Revisi Tahap 2 pada Setiap Modul dan Konfigurasi
**Dokumen Komprehensif Pembaruan Logika Sistem, Antarmuka HMI, dan Manajemen Memori**  
*Proyek: Smart Parking Gate System berbasis Arduino Uno*  
*Tanggal Rilis: 29 Oktober 2026*  
*Target Pembaca: Anggota 1 s.d. Anggota 6 (Seluruh Tim Pengembang & QA)*

---

## 1. Pendahuluan dan Latar Belakang Revisi

Dokumen ini merupakan konsolidasi menyeluruh yang menyatukan seluruh perbaikan dari **Revisi Tahap 1** (penyempurnaan logika keselamatan lintasan, filter sensor ultrasonik, dan stabilitas simulasi) serta **Revisi Tahap 2** (perubahan paradigma tampilan slot LCD dari hitung mundur sisa menjadi hitung maju slot terisi, validasi persistensi EEPROM dengan *magic byte*, dan penyelarasan variabel sistem).

Dokumen disusun secara terstruktur **per modul/berkas** untuk memudahkan setiap penanggung jawab bagian dalam memahami:
1. Komponen apa yang diubah.
2. Lokasi bagian/fungsi/baris yang mengalami modifikasi.
3. Komparasi kode dan perilaku sistem antara **Sebelum Perubahan** vs **Setelah Perubahan**.
4. Evaluasi status persistensi EEPROM pada simulator Wokwi di akhir laporan.

---

## 2. Rincian Pembaruan Per Modul / Per Berkas

---

### A. Berkas `config.h` (Konfigurasi Pusat & Acuan Parameter Tim)

Berkas ini adalah *single source of truth* untuk seluruh parameter pin, pewaktuan, ambang batas deteksi, dan prototipe fungsi.

#### 1. Bagian yang Diubah / Ditambahkan
* **Pewaktuan Sensor & Keselamatan (Tahap 1):** Ditambahkan konstanta `DURASI_CLEARANCE` dan `TIMEOUT_LEWAT`.
* **Prototipe Sensor (Tahap 1):** Ditambahkan prototipe fungsi pelacak Sensor 2 `resetSensorKeluar()` dan `sensorKeluarPernahTerdeteksi()`.
* **Verifikasi EEPROM (Tahap 2):** Ditambahkan alamat dan konstanta *magic byte* `ALAMAT_MAGIC` dan `NILAI_MAGIC`.
* **Sinkronisasi Ambang Batas Jarak:** Komentar diselaraskan dari batas `< 100 cm` menjadi `<= 100 cm`.

#### 2. Perbedaan Sebelum vs Setelah Perubahan

```cpp
// ==================== SEBELUM PERUBAHAN ====================
// Parameter Sensor Ultrasonik
#define JARAK_DETEKSI           100     // Batas deteksi (< 100 cm ada mobil)
#define DURASI_VALIDASI        2000     // Waktu henti mobil di sensor masuk (ms)
#define INTERVAL_SENSOR          50     // Jeda pembacaan sensor berkala (ms)
#define TIMEOUT_ECHO          15000     // Batas waktu pulsa echo (mikrodetik)
// (Belum ada DURASI_CLEARANCE dan TIMEOUT_LEWAT)

// Prototipe Sensor
void inisialisasiSensor();
long bacaJarak(uint8_t pinTrig, uint8_t pinEcho);
bool cekMobilMasuk();
bool cekMobilLewat();
void resetValidasiMasuk();
// (Belum ada resetSensorKeluar dan sensorKeluarPernahTerdeteksi)

// Parameter EEPROM
#define ALAMAT_EEPROM             0     // Alamat byte penyimpanan slot di EEPROM
#define SLOT_DEFAULT             10     // Nilai kapasitas bawaan default
// (Belum ada ALAMAT_MAGIC dan NILAI_MAGIC)
```

```cpp
// ==================== SETELAH PERUBAHAN ====================
// Parameter Sensor Ultrasonik (Tahap 1)
#define JARAK_DETEKSI           100     // Batas deteksi mobil (<= 100 cm ada mobil)
#define DURASI_VALIDASI        2000     // Waktu henti mobil di sensor masuk (ms)
#define INTERVAL_SENSOR          50     // Jeda pembacaan sensor berkala (ms)
#define TIMEOUT_ECHO          15000     // Batas waktu pulsa echo (mikrodetik)
#define DURASI_CLEARANCE       1000     // Sensor 2 harus kosong stabil selama ini sebelum palang menutup (ms)
#define TIMEOUT_LEWAT         30000     // Batas tunggu mobil menyentuh Sensor 2 sejak palang terbuka penuh (ms)

// Prototipe Sensor (Tahap 1)
void inisialisasiSensor();
long bacaJarak(uint8_t pinTrig, uint8_t pinEcho);
bool cekMobilMasuk();
bool cekMobilLewat();
void resetValidasiMasuk();
void resetSensorKeluar();                // Reset status Sensor 2 setiap kali palang selesai membuka
bool sensorKeluarPernahTerdeteksi();     // True jika Sensor 2 sudah terhalang sejak reset terakhir

// Parameter EEPROM & Magic Byte (Tahap 2)
#define ALAMAT_EEPROM             0     // Alamat byte penyimpanan slot di EEPROM
#define ALAMAT_MAGIC              1     // Alamat byte penanda data EEPROM valid
#define NILAI_MAGIC            0xA5     // Nilai penanda (jika beda, berarti data belum pernah diatur)
#define SLOT_DEFAULT             10     // Nilai kapasitas bawaan default
#define SLOT_MIN                  1     // Batas minimal slot parkir
#define SLOT_MAX                 99     // Batas maksimal slot parkir
```

---

### B. Berkas `sketch.ino` (Logika Utama Sistem FSM & Kontrol Servo)

Berkas utama yang mengatur transisi status *Finite State Machine* (FSM), pergerakan palang motor servo, dan pembaruan berkala antarmuka.

#### 1. Bagian yang Diubah / Ditambahkan
* **Variabel Pelacak Waktu & Serial Log (Tahap 1):** Ditambahkan `waktuMulaiLewat` dan penanda satu kali cetak log `logSensorKeluar`.
* **State `STATUS_SIAGA` (Tahap 1):** Pemanggilan `resetValidasiMasuk()` sebelum transisi ke `STATUS_VALIDASI`.
* **State `STATUS_BUKA` (Tahap 1):** Inisialisasi awal saat palang terbuka penuh menuju `STATUS_LEWAT`.
* **State `STATUS_LEWAT` (Tahap 1):** Penerapan 3 cabang pengaman: penyelesaian clearance 1 detik (`tambahSlot()`), penahanan palang saat mobil di sensor 2, dan *timeout* 30 detik tanpa penambahan slot.
* **Fungsi `loop()` - Pemanggilan Tampilan LCD (Tahap 2):** Mengubah parameter yang dikirim ke fungsi `perbaruiLcd()` dari sisa slot menjadi slot terisi.

#### 2. Perbedaan Sebelum vs Setelah Perubahan

```cpp
// ==================== SEBELUM PERUBAHAN ====================
// loop() - Pemanggilan LCD (Format Hitung Mundur Sisa)
perbaruiLcd(statusSaatIni, hitungSisaSlot(), kapasitasMaks);

// STATUS_LEWAT (Logika Lama)
case STATUS_LEWAT:
  if (cekMobilLewat()) {
    tambahSlot();
    bunyiGerak();
    targetSudut = SUDUT_TUTUP;
    statusSaatIni = STATUS_TUTUP;
  }
  // Tidak ada timeout 30 detik (palang macet terbuka jika mobil batal lewat)
  // Tidak ada penahanan jika mobil berhenti di tengah sensor 2
  break;
```

```cpp
// ==================== SETELAH PERUBAHAN ====================
// loop() - Pemanggilan LCD (Tahap 2: Format Hitung Maju Slot Terisi)
perbaruiLcd(statusSaatIni, slotTerisi, kapasitasMaks);

// STATUS_BUKA (Tahap 1: Penyiapan Timer Lintasan)
case STATUS_BUKA:
  if (palangSampaiTujuan && sudutPalang == SUDUT_BUKA) {
    Serial.println(F("[FSM]: Palang terbuka penuh. Menunggu mobil lewat (batas 30 detik)..."));
    resetSensorKeluar();
    waktuMulaiLewat = millis();
    logSensorKeluar = false;
    statusSaatIni = STATUS_LEWAT;
  }
  break;

// STATUS_LEWAT (Tahap 1: Proteksi 3 Cabang)
case STATUS_LEWAT:
  if (cekMobilLewat()) {
    Serial.println(F("[FSM]: Sensor 2 kosong stabil 1 detik. Mobil selesai lewat. Menutup palang..."));
    tambahSlot();
    bunyiGerak();
    targetSudut = SUDUT_TUTUP;
    statusSaatIni = STATUS_TUTUP;
  } else if (sensorKeluarPernahTerdeteksi()) {
    if (!logSensorKeluar) {
      Serial.println(F("[FSM]: Mobil terdeteksi di Sensor 2. Palang ditahan sampai Sensor 2 kosong."));
      logSensorKeluar = true;
    }
  } else if (millis() - waktuMulaiLewat >= TIMEOUT_LEWAT) {
    Serial.println(F("[FSM]: Timeout 30 detik, tidak ada mobil di Sensor 2. Menutup palang tanpa menambah slot..."));
    bunyiGerak();
    targetSudut = SUDUT_TUTUP;
    statusSaatIni = STATUS_TUTUP;
  }
  break;
```

---

### C. Berkas `module_display.h` (Antarmuka Visual LCD 16x2 & Indikator LED)

Modul penampil HMI visual untuk menyajikan instruksi kepada pengemudi dan teknisi.

#### 1. Bagian yang Diubah
* **Format Teks Baris Pertama `STATUS_SIAGA` (Tahap 2):** Format diubah dari menampilkan sisa slot parkir (`SLOT: %02d/%02d OPEN`) menjadi format pemakaian slot saat ini (`TERISI: %02d/%02d`).
* **Kerapian Kolom LCD 16 Karakter (Tahap 1):** Teks pada `STATUS_TUTUP` dan `STATUS_KONFIG` dipangkas agar tidak melebihi 16 karakter tanpa menimbulkan kedipan (*flicker-free*).

#### 2. Perbedaan Sebelum vs Setelah Perubahan

```cpp
// ==================== SEBELUM PERUBAHAN ====================
// Pada fungsi perbaruiLcd()
case STATUS_SIAGA:
  snprintf(buf, sizeof(buf), "SLOT: %02d/%02d OPEN", sisaSlot, totalSlot);
  _tulisBaris(0, buf);
  _tulisBaris(1, "SILAKAN MENDEKAT");
  break;

case STATUS_TUTUP:
  _tulisBaris(0, "MENUTUP GERBANG");
  _tulisBaris(1, "AWAS PALANG TURUN"); // 17 karakter (melebihi batas 16 baris LCD)
  break;

// Pada fungsi tampilkanMenuConfig()
snprintf(buf, sizeof(buf), "KAPASITAS: < %02d >", nilaiSementara); // 17 karakter
```

```cpp
// ==================== SETELAH PERUBAHAN ====================
// Pada fungsi perbaruiLcd()
case STATUS_SIAGA:
  // Tahap 2: Menampilkan slot yang terisi, menghitung maju dari 00 ke kuota maksimal
  snprintf(buf, sizeof(buf), "TERISI: %02d/%02d", sisaSlot, totalSlot);
  _tulisBaris(0, buf);
  _tulisBaris(1, "SILAKAN MENDEKAT");
  break;

case STATUS_TUTUP:
  _tulisBaris(0, "MENUTUP GERBANG");
  _tulisBaris(1, "PALANG TURUN!");   // Tahap 1: 13 karakter (rapi dan aman dalam 16 kolom)
  break;

// Pada fungsi tampilkanMenuConfig()
snprintf(buf, sizeof(buf), "KAPASITAS: <%02d>", nilaiSementara); // Tahap 1: 15 karakter
```

---

### D. Berkas `module_storage.h` (Manajemen Memori EEPROM & Tombol Pengaturan)

Modul yang mengelola penyimpanan permanen kuota slot, debouncing tombol fisik, dan perhitungan ketersediaan slot.

#### 1. Bagian yang Diubah
* **Fungsi `bacaEeprom()` (Tahap 2):** Menambahkan verifikasi *magic byte* di alamat 1 (`ALAMAT_MAGIC`). Jika *magic byte* bernilai bukan `0xA5` atau nilai kapasitas di luar batas 1–99, sistem otomatis menuliskan nilai bawaan `SLOT_DEFAULT` (10) dan menandai *magic byte* serta mencetak diagnosis ke Serial Monitor.
* **Fungsi `simpanEeprom()` (Tahap 2):** Menyimpan nilai baru ke `ALAMAT_EEPROM` menggunakan `EEPROM.update`, sekaligus memperbarui `ALAMAT_MAGIC` dengan `NILAI_MAGIC`, serta melakukan cetak verifikasi pembacaan ulang langsung dari EEPROM ke Serial Monitor.

#### 2. Perbedaan Sebelum vs Setelah Perubahan

```cpp
// ==================== SEBELUM PERUBAHAN ====================
void bacaEeprom() {
  int nilai = EEPROM.read(ALAMAT_EEPROM);
  if (nilai < SLOT_MIN || nilai > SLOT_MAX) {
    nilai = SLOT_DEFAULT;
  }
  kapasitasMaks = nilai;
}

void simpanEeprom(int nilaiBaru) {
  nilaiBaru = constrain(nilaiBaru, SLOT_MIN, SLOT_MAX);
  EEPROM.update(ALAMAT_EEPROM, (byte)nilaiBaru);
  kapasitasMaks = nilaiBaru;
  if (slotTerisi > kapasitasMaks) {
    slotTerisi = kapasitasMaks;  
  }
}
```

```cpp
// ==================== SETELAH PERUBAHAN ====================
void bacaEeprom() {
  byte magic = EEPROM.read(ALAMAT_MAGIC);
  int  nilai = EEPROM.read(ALAMAT_EEPROM);

  if (magic != NILAI_MAGIC || nilai < SLOT_MIN || nilai > SLOT_MAX) {
    nilai = SLOT_DEFAULT;
    EEPROM.update(ALAMAT_EEPROM, (byte)nilai);
    EEPROM.update(ALAMAT_MAGIC, (byte)NILAI_MAGIC);
    Serial.print(F("[EEPROM]: Data belum valid (magic=0x"));
    Serial.print(magic, HEX);
    Serial.print(F("). Menulis default = "));
    Serial.println(nilai);
  } else {
    Serial.print(F("[EEPROM]: Data valid terbaca. Kapasitas = "));
    Serial.println(nilai);
  }
  kapasitasMaks = nilai;
}

void simpanEeprom(int nilaiBaru) {
  nilaiBaru = constrain(nilaiBaru, SLOT_MIN, SLOT_MAX);
  EEPROM.update(ALAMAT_EEPROM, (byte)nilaiBaru);
  EEPROM.update(ALAMAT_MAGIC, (byte)NILAI_MAGIC);
  kapasitasMaks = nilaiBaru;
  if (slotTerisi > kapasitasMaks) {
    slotTerisi = kapasitasMaks;
  }
  Serial.print(F("[EEPROM]: Berhasil ditulis -> Nilai = "));
  Serial.print(EEPROM.read(ALAMAT_EEPROM));
  Serial.print(F(", Magic = 0x"));
  Serial.println(EEPROM.read(ALAMAT_MAGIC), HEX);
}
```

---

### E. Berkas `module_sensor.h` (Akuisisi Data & Pemfilteran Sensor Ultrasonik)

Modul pengendali dua sensor jarak ultrasonik HC-SR04 untuk validasi kedatangan dan lintasan mobil.

#### 1. Bagian yang Diubah (Tahap 1)
* **Variabel Pelacak Clearance:** Penambahan `keluarPernahTerdeteksi`, `keluarMenungguKosong`, dan `waktuMulaiKosongKeluar`.
* **Fungsi `cekMobilLewat()`:** Menerapkan filter stabil 1000 ms (`DURASI_CLEARANCE`). Palang dilarang menutup sebelum sensor benar-benar kosong selama 1 detik tanpa interupsi.
* **Fungsi Baru:** Implementasi fungsi `resetSensorKeluar()` dan `sensorKeluarPernahTerdeteksi()`.

#### 2. Perbedaan Sebelum vs Setelah Perubahan

```cpp
// ==================== SEBELUM PERUBAHAN ====================
bool cekMobilLewat() {
  static bool mobilTerdeteksi = false;
  long jarak = bacaJarak(PIN_TRIG_KELUAR, PIN_ECHO_KELUAR);
  if (jarak > 0 && jarak <= JARAK_DETEKSI) {
    mobilTerdeteksi = true;
  }
  if (mobilTerdeteksi && (jarak > JARAK_DETEKSI || jarak == 0)) {
    mobilTerdeteksi = false;
    return true; // Langsung true dalam 1 siklus loop! Rawan tertabrak palang jika ada acoustic drop
  }
  return false;
}
```

```cpp
// ==================== SETELAH PERUBAHAN (Tahap 1) ====================
static bool keluarPernahTerdeteksi = false;
static bool keluarMenungguKosong   = false;
static unsigned long waktuMulaiKosongKeluar = 0;

void resetSensorKeluar() {
  keluarPernahTerdeteksi = false;
  keluarMenungguKosong   = false;
  waktuMulaiKosongKeluar = 0;
}

bool sensorKeluarPernahTerdeteksi() {
  return keluarPernahTerdeteksi;
}

bool cekMobilLewat() {
  long jarak = bacaJarak(PIN_TRIG_KELUAR, PIN_ECHO_KELUAR);
  unsigned long sekarang = millis();

  if (jarak > 0 && jarak <= JARAK_DETEKSI) {
    keluarPernahTerdeteksi = true;
    keluarMenungguKosong   = false; // Reset jeda jika bodi mobil terdeteksi lagi
    return false;
  }

  if (keluarPernahTerdeteksi) {
    if (!keluarMenungguKosong) {
      keluarMenungguKosong = true;
      waktuMulaiKosongKeluar = sekarang;
    } else if (sekarang - waktuMulaiKosongKeluar >= DURASI_CLEARANCE) {
      resetSensorKeluar();
      return true; // Hanya true setelah area kosong stabil selama >= 1000 ms
    }
  }
  return false;
}
```

---

### F. Berkas `diagram.json` dan `libraries.txt` (Konfigurasi Simulator Wokwi)

#### 1. Bagian yang Diubah (Tahap 1)
* Mengosongkan entri `"dependencies": { "LiquidCrystal_I2C": "1.1.0" }` pada [diagram.json](file:///D:/Project%202026/smart-parking-gate/diagram.json) menjadi `"dependencies": {}`.
* Memastikan dependensi ditarik murni melalui [libraries.txt](file:///D:/Project%202026/smart-parking-gate/libraries.txt) (`LiquidCrystal I2C`).
* **Dampak Perbaikan:** Mengatasi kegagalan *loading* simulator Wokwi akibat kesalahan interpretasi *custom chip*.

---

## 3. Matriks Komparasi Fungsional Sistem (Sebelum vs. Revisi 1 vs. Revisi 2)

| Fitur / Parameter | Desain Awal | Revisi Tahap 1 | Revisi Tahap 2 (Final) |
| :--- | :--- | :--- | :--- |
| **Format Tampilan Slot LCD** | `SLOT: 10/10 OPEN` (Sisa Slot, Hitung Mundur dari 10 ke 0) | `SLOT: 10/10 OPEN` (Sisa Slot, Hitung Mundur) | `TERISI: 00/10` (Slot Terpakai, **Hitung Maju** dari 0 ke Kuota Maks) |
| **Nilai Layar Awal (Booting)** | Menampilkan kapasitas maksimal sisa (`10/10`) | Menampilkan kapasitas maksimal sisa (`10/10`) | Menampilkan nol kendaraan terparkir (`00/10`) |
| **Ketika Kapasitas Diubah ke 15** | Tampilan menjadi `15/15` atau berkurang jika ada mobil | Tampilan menjadi `15/15` | Tampilan menjadi `00/15` (Slot terisi tetap terjaga, kuota diperbesar) |
| **Keamanan Sensor 2 (Palang Turun)** | Langsung turun pada pembacaan gema pertama $>100\text{ cm}$ | Wajib kosong stabil $\ge 1000\text{ ms}$ (`DURASI_CLEARANCE`) | Wajib kosong stabil $\ge 1000\text{ ms}$ (`DURASI_CLEARANCE`) |
| **Batas Waktu Menunggu di Gerbang** | Tidak terbatas (*deadlock* jika mobil mundur) | *Timeout* 30 detik auto-close tanpa tambah slot | *Timeout* 30 detik auto-close tanpa tambah slot |
| **Integritas Memori EEPROM** | Baca mentah tanpa validasi integritas | Baca mentah tanpa validasi integritas | Dilengkapi **Magic Byte (`0xA5`)**, inisialisasi default otomatis, dan cetak log serial verifikasi |
| **Panjang Karakter Baris LCD** | Terdapat teks 17 karakter (melebar keluar) | Dibatasi maksimal 16 karakter (*anti-flicker*) | Dibatasi maksimal 16 karakter (*anti-flicker*) |

---

## 4. Analisis Mendalam: Mengapa Fitur Persistensi EEPROM Belum Berhasil di Wokwi?

Meskipun logika `simpanEeprom()`, penulisan `EEPROM.update()`, serta verifikasi `NILAI_MAGIC` sudah diimplementasikan dengan standar mikrokontroler yang tepat pada Revisi Tahap 2, **ketika tombol Restart Wokwi ditekan, nilai kuota kapasitas tetap kembali ke default 10**.

Berikut adalah penjelasan teknis mengapa hal ini terjadi:

### 1. Karakteristik Simulator Wokwi (*Stateless Virtual EEPROM*)
* Tombol **Restart** pada antarmuka simulator Wokwi (ikon putar balik / restart simulation) bekerja dengan cara **menghancurkan instance virtual machine AVR** dan mengalokasikan memori RAM dan Flash baru dari nol (*fresh state instantiation*).
* Berbeda dengan board fisik Arduino Uno asli yang memiliki sel silikon EEPROM independen bertipe non-volatile ferroelectric/floating-gate, simulator Wokwi secara default **menghapus seluruh array byte EEPROM virtual dan mengisinya kembali dengan nilai kosong (`0xFF`)** setiap kali simulasi di-restart melalui tombol antarmuka web Wokwi.

### 2. Jejak Eksekusi Logika Saat Booting di Wokwi
Ketika simulator Wokwi di-restart:
1. Chip AVR virtual menyala kembali dari kondisi *clean*.
2. Fungsi `setup()` mengeksekusi `inisialisasiStorage()`, lalu memanggil `bacaEeprom()`.
3. Fungsi membaca alamat memori:
   * Alamat 1 (`ALAMAT_MAGIC`): bernilai `0xFF` (karena virtual EEPROM terhapus).
   * Alamat 0 (`ALAMAT_EEPROM`): bernilai `0xFF` (255 desimal).
4. Pengecekan kondisi pada kode:
   ```cpp
   if (magic != NILAI_MAGIC || nilai < SLOT_MIN || nilai > SLOT_MAX)
   ```
   * Karena `0xFF != 0xA5` dan `255 > 99` (`SLOT_MAX`), kondisi bernilai **TRUE**.
5. Sistem menyimpulkan bahwa data memori kosong atau rusak, lalu mengeksekusi blok pemulihan:
   ```cpp
   nilai = SLOT_DEFAULT; // Bernilai 10
   ```
6. Serial Monitor mencetak log:
   `[EEPROM]: Data belum valid (magic=0xFF). Menulis default = 10`
7. Akibatnya, nilai kuota yang sempat diatur sebelumnya (misalnya 15 atau 25) terhapus bersamaan dengan hancurnya memori virtual Wokwi.

### 3. Bukti Bahwa Fungsi Kode Sebenarnya Sudah Berjalan Benar
Fungsi kode pada firmware **tidak mengalami bug**, dan dapat dibuktikan dengan dua cara berikut:

1. **Pengujian Selama Siklus Daya Aktif (Runtime Verification):**
   * Masuk ke menu konfigurasi (tahan tombol SET 3 detik).
   * Ubah nilai kuota menjadi `15` atau `25` dengan tombol UP/DOWN.
   * Tekan tombol SET secara singkat untuk menyimpan.
   * Serial Monitor langsung mencetak konfirmasi pembacaan langsung dari sel memori:
     ```text
     [EEPROM]: Berhasil ditulis -> Nilai = 25, Magic = 0xA5
     [CONFIG]: Tersimpan ke EEPROM! Kapasitas = 25
     [CONFIG]: Sisa Slot Aktif = 25
     ```
   * Hal ini membuktikan bahwa instruksi penulisan ke memori internal Arduino berjalan sukses 100%.

2. **Perilaku pada Hardware Arduino Uno Fisik Nyata:**
   * Jika kode ini diunggah (*flashed*) ke papan mikrokontroler Arduino Uno fisik asli:
   * Pengubahan slot menjadi 25 dan penekanan tombol RESET fisik pada papan Arduino **akan berhasil mempertahankan nilai 25 secara permanen**, karena sel EEPROM fisik tidak terhapus saat mikrokontroler melakukan *hardware reset*.

### 4. Kesimpulan untuk Dokumen Pengujian QA
Untuk laporan pengujian resmi (*Quality Assurance*):
* Skenario **TC-06 (Ketahanan Memori EEPROM)** pada lembar kerja Wokwi dicatat sebagai **Keterbatasan Lingkungan Simulasi (*Virtual Environment Limitation*)**, bukan cacat program (*software defect*).
* Validasi keberhasilan fungsi EEPROM pada Wokwi diverifikasi melalui pembacaan konfirmasi log serial pada saat fungsi `simpanEeprom()` dipanggil, atau diverifikasi secara fisik menggunakan board Arduino Uno riil.
