/**
 * =================================================================================================
 * SMART PARKING GATE: SISTEM GERBANG PARKIR OTOMATIS BERBASIS MIKROKONTROLER (ARDUINO UNO)
 * FILE KONFIGURASI PUSAT & KESEPAKATAN VARIABEL SISTEM (config.h)
 * =================================================================================================
 * 
 * TUJUAN FILE:
 * 1. Menjadi "Single Source of Truth" (Pusat Kebenaran Tunggal) seluruh variabel, konstanta, pinout, 
 *    dan tipe data untuk mencegah inkonsistensi antar anggota tim.
 * 2. Menyelaraskan nama variabel dalam Bahasa Indonesia yang deskriptif dan baku.
 * 3. Menegaskan pembagian kendali modul untuk masing-masing dari 6 anggota pengembang.
 * 4. Menjamin kesesuaian antarmuka fungsi (API Contract) sebelum penggabungan kode akhir di Wokwi.
 * 
 * PEMBAGIAN TANGGUNG JAWAB TIM (6 ANGGOTA):
 * - Anggota 1: Ketua Tim / System Architect (sketch.ino & FSM State Machine)
 * - Anggota 2: Sensor & Signal Conditioning Engineer (module_sensor.h)
 * - Anggota 3: HMI & Visual Interface Engineer (module_display.h)
 * - Anggota 4: Audio & Safety Alert Engineer (module_audio.h)
 * - Anggota 5: Memory Storage & Parameter Config Engineer (module_storage.h)
 * - Anggota 6: Hardware Schematic, Layout & QA Test Engineer (diagram.json & QA Edge Cases)
 * =================================================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

/* =================================================================================================
 * BAGIAN 1: SISTEM UTAMA, LOGIKA FSM, & KONTROL AKTUATOR PALANG
 * PENANGGUNG JAWAB: ANGGOTA 1 (Ketua Tim / System Architect)
 * LOKASI FILE     : sketch.ino
 * =================================================================================================
 * Deskripsi Tugas:
 * - Mengoordinasikan seluruh siklus kerja sistem dalam loop() utama berbasis Finite State Machine (FSM).
 * - Mengontrol motor servo gerbang (Pin D9) secara halus (interpolasi sudut non-blocking).
 * - Menangani transisi status (state transition) antar modul secara aman.
 */

// --- Alokasi Pin Aktuator Servo ---
#define PIN_SERVO_PALANG             9    // Pin PWM digital untuk kendali tuas motor servo palang gerbang

// --- Parameter Mekanis Servo Palang ---
#define SUDUT_PALANG_TERTUTUP        0    // Posisi sudut horizontal saat palang menutup (0 derajat)
#define SUDUT_PALANG_TERBUKA        90    // Posisi sudut vertikal saat palang membuka penuh (90 derajat)
#define INTERVAL_GERAK_SERVO_MS     15    // Penundaan non-blocking per 1 derajat gerakan (kecepatan halus ~1.35s)
#define LANGKAH_SUDUT_SERVO          1    // Besaran perubahan sudut per langkah interpolasi (derajat)

// --- Definisi Status Sistem (Finite State Machine / FSM) ---
enum StatusSistemGerbang {
    STATUS_SIAGA = 0,             // Palang tertutup mendatar, sistem menunggu kendaraan mendekat
    STATUS_MENUNGGU_VALIDASI,     // Kendaraan terdeteksi di Sensor 1, filter stabilisasi 2 detik berjalan
    STATUS_MEMBUKA,               // Palang sedang bergerak naik bertahap dari 0 derajat ke 90 derajat
    STATUS_MENUNGGU_LEWAT,        // Palang terbuka penuh di 90 derajat, menunggu kendaraan melewati Sensor 2
    STATUS_MENUTUP,               // Palang sedang bergerak turun bertahap dari 90 derajat ke 0 derajat
    STATUS_PARKIR_PENUH,          // Kuota slot habis (0), akses ditolak dan palang terkunci rapat
    STATUS_MODE_KONFIGURASI       // Menu khusus teknisi untuk mengatur kapasitas slot maksimum via tombol fisik
};

// --- Variabel Global Sistem (Diatur oleh Anggota 1) ---
extern StatusSistemGerbang statusSistemSaatIni;   // Menyimpan status aktif FSM gerbang saat ini
extern int sudutPalangSaatIni;                    // Menyimpan sudut aktual posisi motor servo (0 - 90 derajat)


/* =================================================================================================
 * BAGIAN 2: PENGONDISIAN SINYAL & AKUISISI SENSOR ULTRASONIK
 * PENANGGUNG JAWAB: ANGGOTA 2 (Sensor & Signal Conditioning Engineer)
 * LOKASI FILE     : module_sensor.h
 * =================================================================================================
 * Deskripsi Tugas:
 * - Menghitung jarak objek dengan sensor ultrasonik HC-SR04 secara non-blocking.
 * - Menerapkan filter penundaan validasi keberadaan kendaraan (presence debounce) selama 2000 ms.
 * - Mendeteksi fase pembebasan jalur pada Sensor 2 (exit clearance) setelah palang terlewati.
 */

// --- Alokasi Pin Sensor Ultrasonik Masuk (Sensor 1 - Sebelum Palang) ---
#define PIN_TRIG_SENSOR_MASUK        4    // Pin trigger pemancar pulsa ultrasonik pada Sensor 1 (Entry)
#define PIN_ECHO_SENSOR_MASUK        5    // Pin echo penerima pantulan gelombang ultrasonik pada Sensor 1 (Entry)

// --- Alokasi Pin Sensor Ultrasonik Keluar/Bebas (Sensor 2 - Setelah Palang) ---
#define PIN_TRIG_SENSOR_KELUAR       6    // Pin trigger pemancar pulsa ultrasonik pada Sensor 2 (Exit)
#define PIN_ECHO_SENSOR_KELUAR       7    // Pin echo penerima pantulan gelombang ultrasonik pada Sensor 2 (Exit)

// --- Parameter Ambang Batas Geometri & Akustik Sensor ---
#define JARAK_AMBANG_DETEKSI_CM    100    // Batas jarak deteksi (< 100 cm = mobil ada; >= 100 cm = lajur kosong)
#define DURASI_VALIDASI_MASUK_MS  2000    // Waktu mobil harus berhenti stabil di Sensor 1 agar tervalidasi (2 detik)
#define INTERVAL_BACA_SENSOR_MS     50    // Interval pembacaan sensor berkala untuk kestabilan akustik
#define TIMEOUT_ECHO_MIKRODETIK  15000    // Waktu tunggu maksimum pulsa echo (~2.5 meter jangkauan aman)

// --- Deklarasi Prototipe Fungsi Kesepakatan Antarmuka Sensor ---
void inisialisasiSensor();
long bacaJarakUltrasonikCm(uint8_t pinTrig, uint8_t pinEcho);
bool apakahMobilTerdeteksiValidMasuk();    // Bernilai true hanya jika Sensor 1 mendeteksi objek stabil >= 2 detik
bool apakahMobilTelahTuntasLewat();       // Bernilai true jika Sensor 2 sempat tertutup lalu kembali bersih (>100 cm)
void resetFilterValidasiSensorMasuk();     // Mereset penghitung waktu jika mobil pergi sebelum 2 detik


/* =================================================================================================
 * BAGIAN 3: INDIKATOR VISUAL, LAYAR LCD 16X2 I2C & LAMPU LED
 * PENANGGUNG JAWAB: ANGGOTA 3 (HMI & Visual Interface Engineer)
 * LOKASI FILE     : module_display.h
 * =================================================================================================
 * Deskripsi Tugas:
 * - Menampilkan teks status, kuota slot, dan panduan pengemudi pada layar LCD 16x2 I2C tanpa kedip.
 * - Mengatur pola nyala dan kedipan LED Merah (D12) dan LED Hijau (D11) menggunakan millis().
 */

// --- Konfigurasi Layar LCD 16x2 I2C ---
#define ALAMAT_I2C_LCD            0x27    // Alamat default komunikasi I2C modul pcf8574 LCD
#define JUMLAH_KOLOM_LCD            16    // Jumlah karakter horizontal per baris LCD
#define JUMLAH_BARIS_LCD             2    // Jumlah baris karakter vertikal pada LCD

// --- Alokasi Pin Indikator Lampu LED ---
#define PIN_LED_HIJAU               11    // Pin keluaran LED Hijau (Penanda akses diizinkan / aman melintas)
#define PIN_LED_MERAH               12    // Pin keluaran LED Merah (Penanda akses ditolak / palang tutup / siaga)

// --- Parameter Pewaktuan Kedipan LED (Non-blocking) ---
#define INTERVAL_KEDIP_CEPAT_MS    150    // Kecepatan kedip cepat LED Merah saat palang bergerak naik/turun
#define INTERVAL_KEDIP_LAMBAT_MS   500    // Kecepatan kedip lambat LED Merah saat kondisi kuota parkir penuh
#define INTERVAL_SEGARKAN_LCD_MS   250    // Periode pembaruan layar LCD agar bebas kedip (anti-flicker)

// --- Deklarasi Prototipe Fungsi Kesepakatan Antarmuka Tampilan ---
void inisialisasiTampilanVisual();
void perbaruiTampilanLcd(StatusSistemGerbang status, int slotTersedia, int totalKapasitas);
void perbaruiIndikatorLed(StatusSistemGerbang status);
void tampilkanPesanSplashAwal();
void tampilkanMenuKonfigurasiLcd(int kapasitasSementara);


/* =================================================================================================
 * BAGIAN 4: NOTIFIKASI AUDIO & PERINGATAN KEAMANAN (BUZZER)
 * PENANGGUNG JAWAB: ANGGOTA 4 (Audio & Safety Alert Engineer)
 * LOKASI FILE     : module_audio.h
 * =================================================================================================
 * Deskripsi Tugas:
 * - Menghasilkan umpan balik audio dengan berbagai pola nada (bip, alarm, peringatan, klik).
 * - Menjamin eksekusi nada bersifat non-blocking (tidak menggunakan fungsi delay()).
 */

// --- Alokasi Pin Modul Buzzer ---
#define PIN_BUZZER                  13    // Pin output sinyal audio untuk menggerakkan buzzer

// --- Parameter Frekuensi Nada Audio (Hertz) ---
#define NADA_BIP_VALIDASI_HZ      2000    // Frekuensi nada tinggi pendek saat mobil sukses tervalidasi masuk
#define NADA_ALARM_TUTUP_HZ       1500    // Frekuensi ritmik peringatan ketika palang gerbang sedang bergerak turun
#define NADA_AKSES_DITOLAK_HZ      800    // Frekuensi rendah nada panjang tanda akses ditolak (parkir penuh)
#define NADA_KLIK_TOMBOL_HZ       2500    // Frekuensi tinggi sangat singkat untuk umpan balik penekanan tombol fisik

// --- Parameter Durasi Pola Audio (Milidetik) ---
#define DURASI_BIP_PENDEK_MS        80    // Durasi nada bip pendek konfirmasi
#define DURASI_BIP_PANJANG_MS      600    // Durasi nada bip panjang alarm penolakan
#define DURASI_JEDA_RITMIK_MS      120    // Jeda antar-nada pada peringatan berkala

// --- Deklarasi Prototipe Fungsi Kesepakatan Antarmuka Audio ---
void inisialisasiAudio();
void bunyikanNadaSuksesMasuk();           // Bunyi bip 2x saat mobil berhasil tervalidasi
void bunyikanAlarmGerbangBergerak();      // Bunyi peringatan keselamatan saat palang bergerak turun
void bunyikanAlarmAksesDitolak();         // Bunyi peringatan panjang saat gerbang penuh
void bunyikanNadaKlikTombol();            // Bunyi umpan balik setiap tombol fisik ditekan
void perbaruiPolaAudio();                // Dipanggil di loop() untuk pemrosesan ritme audio non-blocking
void matikanAudio();                     // Mematikan suara buzzer seketika


/* =================================================================================================
 * BAGIAN 5: PERSISTENSI MEMORI EEPROM & MANAJEMEN TOMBOL FISIK
 * PENANGGUNG JAWAB: ANGGOTA 5 (Memory Storage & Parameter Config Engineer)
 * LOKASI FILE     : module_storage.h
 * =================================================================================================
 * Deskripsi Tugas:
 * - Mengelola pembacaan 3 tombol navigasi (UP, DOWN, SET) dengan filter anti-lentingan (debounce).
 * - Mendeteksi penekanan tombol SET selama 3 detik untuk masuk/keluar Mode Konfigurasi.
 * - Menyimpan dan membaca nilai kuota kapasitas maksimum parkir secara permanen di memori EEPROM.
 */

// --- Alokasi Pin Tombol Fisik (Konfigurasi INPUT_PULLUP: Aktif ketika bernilai LOW) ---
#define PIN_TOMBOL_TAMBAH_SLOT       2    // Tombol UP: Menambah kuota kapasitas maksimal parkir
#define PIN_TOMBOL_KURANG_SLOT       8    // Tombol DOWN: Mengurangi kuota kapasitas maksimal parkir
#define PIN_TOMBOL_PENGATURAN_SET   10    // Tombol SET: Tahan 3 dtk untuk menu konfigurasi; Tekan 1x untuk simpan

// --- Parameter Memori EEPROM & Batasan Kuota Slot ---
#define ALAMAT_EEPROM_KAPASITAS      0    // Alamat indeks byte EEPROM internal tempat menyimpan kuota slot maksimum
#define KAPASITAS_SLOT_DEFAULT      10    // Kapasitas default bawaan pabrik jika data EEPROM kosong/rusak
#define BATAS_MINIMAL_SLOT           1    // Batas paling sedikit slot parkir yang diizinkan sistem
#define BATAS_MAKSIMAL_SLOT         99    // Batas paling banyak slot parkir yang diizinkan sistem

// --- Parameter Waktu Debounce & Penekanan Tombol ---
#define DURASI_DEBOUNCE_TOMBOL_MS   50    // Filter penstabil kontak mekanis tombol dari lentingan listrik
#define DURASI_TAHAN_MASUK_CFG_MS 3000    // Durasi tombol SET harus ditahan (3 detik) untuk memicu Mode Konfigurasi

// --- Variabel Global Kuota Parkir (Dikelola oleh Anggota 5) ---
extern int kapasitasMaksimalSlot;         // Total kuota slot parkir yang dialokasikan (tersimpan di EEPROM)
extern int jumlahSlotTerisi;              // Jumlah kendaraan yang saat ini sedang berada di dalam area parkir

// --- Deklarasi Prototipe Fungsi Kesepakatan Antarmuka Penyimpanan & Tombol ---
void inisialisasiPenyimpananDanTombol();
void bacaKapasitasDariEeprom();
void simpanKapasitasKeEeprom(int nilaiKapasitasBaru);
bool apakahTombolSetDitahan3Detik();       // Mendeteksi apakah tombol SET ditekan dan ditahan >= 3000 ms
bool apakahTombolTambahDitekan();          // Mendeteksi klik tombol UP (sudah ter-filter debounce)
bool apakahTombolKurangDitekan();          // Mendeteksi klik tombol DOWN (sudah ter-filter debounce)
bool apakahTombolSetDitekanSekali();       // Mendeteksi klik singkat tombol SET untuk konfirmasi simpan
int  hitungSisaSlotTersedia();            // Menghitung: (kapasitasMaksimalSlot - jumlahSlotTerisi)
void tambahSlotTerisi();                  // Dipanggil saat mobil berhasil melewati Sensor 2 ke dalam gerbang
void kurangiSlotTerisi();                 // Fungsi cadangan jika ada gerbang keluar di kemudian hari


/* =================================================================================================
 * BAGIAN 6: STANDAR PENGKABELAN SIRKUIT & VALIDASI PENJAMIN MUTU (QA)
 * PENANGGUNG JAWAB: ANGGOTA 6 (Hardware Schematic, Layout & QA Test Engineer)
 * LOKASI FILE     : diagram.json & Dokumen Matriks Pengujian
 * =================================================================================================
 * Pedoman Konvensi Pengkabelan Wokwi (diagram.json):
 * - Kabel Daya VCC (+5V)          : Warna Merah ("red")
 * - Kabel Ground (GND)            : Warna Hitam ("black")
 * - Jalur Komunikasi I2C (A4/A5)  : Warna Biru ("blue") untuk SDA, Kuning ("yellow") untuk SCL
 * - Sinyal Data Sensor & Tombol   : Warna Hijau ("green") atau Oranye ("orange")
 * - Sinyal PWM Servo & Buzzer     : Warna Ungu ("purple")
 *
 * Skenario Pengujian Kunci yang Wajib Divalidasi:
 * - TC-01: Mobil lewat di Sensor 1 < 2 detik -> Palang tetap menutup (0 derajat).
 * - TC-02: Mobil berhenti di Sensor 1 >= 2 detik -> Palang membuka halus ke 90 derajat.
 * - TC-03: Slot parkir = 0 -> Palang menolak membuka, LED Merah kedip lambat, buzzer berbunyi alarm.
 * - TC-04: Mobil berhenti di Sensor 2 -> Palang tidak boleh turun sampai Sensor 2 kembali bersih.
 * - TC-05: Tombol SET ditekan saat palang membuka/mobil lewat -> Perintah konfigurasi diabaikan (Safety Interlock).
 * - TC-06: Simulasi di-restart -> Nilai kapasitas slot tetap terjaga sesuai pembacaan EEPROM.
 * =================================================================================================
 */

#endif // CONFIG_H
