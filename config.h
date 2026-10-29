/**
 * =================================================================================================
 * SMART PARKING GATE: SISTEM GERBANG PARKIR OTOMATIS BERBASIS ARDUINO UNO
 * FILE KONFIGURASI PUSAT & DEFINISI VARIABEL (config.h)
 * =================================================================================================
 * 
 * Acuan tunggal konfigurasi pin, konstanta, enum status, dan prototipe fungsi
 * untuk seluruh anggota tim pengembang.
 * 
 * PEMBAGIAN TUGAS:
 * - Anggota 1: Logika Utama FSM & Kontrol Servo (sketch.ino)
 * - Anggota 2: Pembacaan Sensor Ultrasonik (module_sensor.h)
 * - Anggota 3: Tampilan LCD 16x2 I2C & Indikator LED (module_display.h)
 * - Anggota 4: Notifikasi Audio & Alarm Buzzer (module_audio.h)
 * - Anggota 5: Memori EEPROM & Tombol Pengaturan (module_storage.h)
 * - Anggota 6: Desain Skematik Wokwi & Pengujian QA (diagram.json)
 * =================================================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

/* =================================================================================================
 * 1. KONTROL SISTEM UTAMA, FSM, & SERVO (Anggota 1 - sketch.ino)
 * =================================================================================================
 */

// Pin Servo
#define PIN_SERVO                 9     // Pin PWM servo palang gerbang

// Parameter Gerak Servo
#define SUDUT_TUTUP               0     // Sudut palang menutup (horizontal)
#define SUDUT_BUKA               90     // Sudut palang membuka (vertikal)
#define JEDA_SERVO               15     // Delay per derajat gerak servo (ms)
#define LANGKAH_SERVO             1     // Langkah pergeseran sudut servo

// Status Sistem (FSM)
enum StatusSistem {
    STATUS_SIAGA = 0,                   // Palang tutup, menunggu kendaraan datang
    STATUS_VALIDASI,                    // Mobil terdeteksi, verifikasi 2 detik
    STATUS_BUKA,                        // Palang bergerak membuka (0 -> 90)
    STATUS_LEWAT,                       // Palang terbuka, menunggu mobil lewat
    STATUS_TUTUP,                       // Palang bergerak menutup (90 -> 0)
    STATUS_PENUH,                       // Slot habis, palang terkunci rapat
    STATUS_KONFIG                       // Menu pengaturan kuota slot
};

// Variabel Global Sistem
extern StatusSistem statusSaatIni;       // Status aktif sistem FSM
extern int sudutPalang;                 // Posisi sudut servo saat ini


/* =================================================================================================
 * 2. SENSOR ULTRASONIK HC-SR04 (Anggota 2 - module_sensor.h)
 * =================================================================================================
 */

// Pin Sensor Masuk (Sensor 1 - Sebelum Palang)
#define PIN_TRIG_MASUK            4     // Pin Trigger sensor masuk
#define PIN_ECHO_MASUK            5     // Pin Echo sensor masuk

// Pin Sensor Keluar (Sensor 2 - Setelah Palang)
#define PIN_TRIG_KELUAR           6     // Pin Trigger sensor keluar
#define PIN_ECHO_KELUAR           7     // Pin Echo sensor keluar

// Parameter Jarak & Waktu Deteksi
#define JARAK_DETEKSI           100     // Batas deteksi mobil (<= 100 cm ada mobil)
#define DURASI_VALIDASI        2000     // Waktu henti mobil di sensor masuk (ms)
#define INTERVAL_SENSOR          50     // Jeda pembacaan sensor berkala (ms)
#define TIMEOUT_ECHO          15000     // Batas waktu pulsa echo (mikrodetik)
#define DURASI_CLEARANCE       1000     // Sensor 2 harus kosong stabil selama ini sebelum palang menutup (ms)
#define TIMEOUT_LEWAT         30000     // Batas tunggu mobil menyentuh Sensor 2 sejak palang terbuka penuh (ms)

// Prototipe Fungsi Sensor
void inisialisasiSensor();
long bacaJarak(uint8_t pinTrig, uint8_t pinEcho);
bool cekMobilMasuk();                    // True jika mobil stabil >= 2 detik di Sensor 1
bool cekMobilLewat();                    // True jika Sensor 2 pernah terhalang lalu kosong stabil >= DURASI_CLEARANCE
void resetValidasiMasuk();               // Reset timer validasi jika mobil pergi sebelum 2 detik
void resetSensorKeluar();                // Reset status Sensor 2 setiap kali palang selesai membuka
bool sensorKeluarPernahTerdeteksi();     // True jika Sensor 2 sudah terhalang sejak reset terakhir


/* =================================================================================================
 * 3. TAMPILAN LCD 16X2 I2C & INDIKATOR LED (Anggota 3 - module_display.h)
 * =================================================================================================
 */

// Konfigurasi LCD I2C
#define ALAMAT_LCD             0x27     // Alamat I2C LCD
#define KOLOM_LCD                16     // Jumlah kolom LCD
#define BARIS_LCD                 2     // Jumlah baris LCD

// Pin Indikator LED
#define PIN_LED_HIJAU            11     // LED Hijau (akses terbuka / aman melintas)
#define PIN_LED_MERAH            12     // LED Merah (berhenti / tutup / penuh)

// Parameter Waktu Tampilan & Kedip
#define KEDIP_CEPAT             150     // Jeda kedip cepat saat palang bergerak (ms)
#define KEDIP_LAMBAT            500     // Jeda kedip lambat saat parkir penuh (ms)
#define REFRESH_LCD             250     // Periode pembaruan teks LCD anti-kedip (ms)

// Prototipe Fungsi Tampilan Visual
void inisialisasiDisplay();
void perbaruiLcd(StatusSistem status, int sisaSlot, int totalSlot);
void perbaruiLed(StatusSistem status);
void tampilkanSplash();
void tampilkanMenuConfig(int nilaiSementara);


/* =================================================================================================
 * 4. AUDIO & ALARM BUZZER (Anggota 4 - module_audio.h)
 * =================================================================================================
 */

// Pin Buzzer
#define PIN_BUZZER               13     // Pin modul buzzer

// Frekuensi Nada Audio (Hz)
#define NADA_MASUK             2000     // Nada bip konfirmasi mobil tervalidasi
#define NADA_TUTUP             1500     // Nada peringatan saat palang turun
#define NADA_PENUH              800     // Nada panjang tanda parkir penuh
#define NADA_KLIK              2500     // Nada klik umpan balik penekanan tombol

// Durasi Pola Audio (ms)
#define BIP_PENDEK               80     // Durasi bip pendek
#define BIP_PANJANG             600     // Durasi bip panjang
#define JEDA_NADA               120     // Jeda antar-bip ritmik

// Prototipe Fungsi Audio
void inisialisasiAudio();
void bunyiMasuk();                      // Bip 2x tanda sukses validasi
void bunyiGerak();                      // Peringatan saat palang bergerak turun
void bunyiPenuh();                      // Nada tolak saat parkir penuh
void bunyiKlik();                       // Umpan balik klik tombol
void perbaruiAudio();                   // Pemrosesan ritme audio non-blocking di loop()
void matikanAudio();                    // Matikan suara buzzer seketika


/* =================================================================================================
 * 5. MEMORI EEPROM & TOMBOL PENGATURAN (Anggota 5 - module_storage.h)
 * =================================================================================================
 */

// Pin Tombol Fisik (INPUT_PULLUP: Aktif LOW)
#define PIN_TOMBOL_UP             2     // Tombol tambah kapasitas slot
#define PIN_TOMBOL_DOWN           8     // Tombol kurang kapasitas slot
#define PIN_TOMBOL_SET           10     // Tombol pengaturan & simpan EEPROM

// Parameter EEPROM & Batas Slot
#define ALAMAT_EEPROM             0     // Alamat byte penyimpanan slot di EEPROM
#define ALAMAT_MAGIC              1     // Alamat byte magic number untuk validasi EEPROM
#define NILAI_MAGIC              0xA5   // Nilai magic number untuk validasi EEPROM
#define SLOT_DEFAULT             10     // Nilai kapasitas bawaan default
#define SLOT_MIN                  1     // Batas minimal slot parkir
#define SLOT_MAX                 99     // Batas maksimal slot parkir

// Parameter Waktu Tombol
#define DEBOUNCE_TOMBOL          50     // Filter debounce tombol (ms)
#define TAHAN_CONFIG           3000     // Waktu tahan tombol SET untuk masuk menu config (ms)

// Variabel Global Kuota Slot
extern int kapasitasMaks;               // Total kapasitas slot di EEPROM
extern int slotTerisi;                  // Jumlah slot terisi saat ini

// Prototipe Fungsi Penyimpanan & Tombol
void inisialisasiStorage();
void bacaEeprom();
void simpanEeprom(int nilaiBaru);
bool cekTombolSetTahan();                // True jika tombol SET ditahan >= 3 detik
bool cekTombolUp();                     // True jika tombol UP ditekan (ter-filter debounce)
bool cekTombolDown();                   // True jika tombol DOWN ditekan (ter-filter debounce)
bool cekTombolSet();                    // True jika tombol SET ditekan singkat untuk simpan
int  hitungSisaSlot();                  // Menghitung sisa slot: kapasitasMaks - slotTerisi
void tambahSlot();                      // Tambah slot terisi saat mobil masuk
void kurangiSlot();                     // Kurangi slot terisi (opsional jika ada gerbang keluar)


/* =================================================================================================
 * 6. KETENTUAN PENGKABELAN & SKENARIO UJI QA (Anggota 6 - diagram.json & QA)
 * =================================================================================================
 * Warna Kabel Wokwi:
 * - VCC (+5V) : Merah ("red")
 * - GND       : Hitam ("black")
 * - I2C Data  : SDA = Biru ("blue"), SCL = Kuning ("yellow")
 * - Sinyal IO : Hijau ("green") atau Oranye ("orange")
 * - PWM/Audio : Ungu ("purple")
 *
 * Skenario Pengujian Kunci:
 * - TC-01: Mobil di Sensor 1 < 2 detik -> Palang tetap tertutup (0 derajat).
 * - TC-02: Mobil di Sensor 1 >= 2 detik -> Palang membuka perlahan ke 90 derajat.
 * - TC-03: Sisa slot = 0 -> Akses ditolak, palang terkunci, LED Merah kedip lambat, buzzer bunyi tolak.
 * - TC-04: Mobil berhenti di Sensor 2 -> Palang tidak boleh turun sampai Sensor 2 bebas.
 * - TC-05: Tombol SET ditekan saat palang buka/mobil lewat -> Mode config ditolak (Safety Interlock).
 *          Config hanya bisa dibuka dari STATUS_SIAGA atau STATUS_PENUH dengan palang di 0 derajat.
 * - TC-06: Simulasi di-restart -> Nilai kapasitas slot tetap terjaga sesuai EEPROM.
 * - TC-07: Sensor 2 terhalang lalu kosong < 1 detik dan terhalang lagi -> Palang tetap 90 derajat.
 *          Palang baru menutup setelah Sensor 2 kosong stabil >= 1 detik (DURASI_CLEARANCE).
 * - TC-08: Palang terbuka tetapi Sensor 2 tidak pernah terhalang selama 30 detik (TIMEOUT_LEWAT)
 *          -> Palang menutup otomatis, slot terisi TIDAK bertambah.
 * =================================================================================================
 */

#endif // CONFIG_H
