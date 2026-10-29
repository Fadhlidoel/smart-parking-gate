// ============================================================
// module_display.h
// HMI & Visual Interface : LCD 16x2 I2C + LED Merah/Hijau
// Penulis : Anggota 3 (HMI & Visual Interface Engineer)
//
// Prinsip:
//  - Non-blocking: semua pewaktuan memakai millis(), tanpa delay()
//    (satu-satunya pengecualian: tampilkanSplash() yang hanya
//    dipanggil sekali di setup(), sebelum loop dimulai).
//  - Bebas kedip: LCD TIDAK pernah di-clear saat runtime. Setiap
//    baris ditulis 16 karakter penuh (dipadding spasi) dan hanya
//    ditulis ulang jika isi layar memang berubah.
//
// Dependensi: config.h  +  library "LiquidCrystal I2C"
// ============================================================
#ifndef MODULE_DISPLAY_H
#define MODULE_DISPLAY_H

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "config.h"

LiquidCrystal_I2C lcd(ALAMAT_LCD, 16, 2);

// ---------- Cache keadaan layar (anti-kedip) ----------
static int8_t        _lcdStatusTerakhir = -1;
static int16_t       _lcdSisaTerakhir   = -1;
static int16_t       _lcdTotalTerakhir  = -1;
static unsigned long _lcdWaktuTerakhir  = 0;

static void _resetCacheLcd() {
  _lcdStatusTerakhir = -1;
  _lcdSisaTerakhir   = -1;
  _lcdTotalTerakhir  = -1;
}

// Tulis satu baris penuh (16 kolom). Sisa kolom diisi spasi sehingga
// sisa teks lama tertimpa tanpa perlu lcd.clear().
static void _tulisBaris(uint8_t baris, const char* teks) {
  lcd.setCursor(0, baris);
  uint8_t i = 0;
  while (i < 16 && teks[i] != '\0') { lcd.write((uint8_t)teks[i]); i++; }
  while (i < 16)                    { lcd.write((uint8_t)' ');     i++; }
}

// ============================================================
// inisialisasiDisplay()
// ============================================================
void inisialisasiDisplay() {
  pinMode(PIN_LED_MERAH, OUTPUT);
  pinMode(PIN_LED_HIJAU, OUTPUT);
  digitalWrite(PIN_LED_MERAH, LOW);
  digitalWrite(PIN_LED_HIJAU, LOW);

  Wire.begin();
  lcd.init();
  lcd.backlight();
  lcd.clear();            // clear hanya sekali, saat boot
  _resetCacheLcd();
}

// ============================================================
// tampilkanSplash()  - panggil SEKALI di setup()
// Layar pembuka + progress bar + uji lampu (kedua LED menyala).
// ============================================================
void tampilkanSplash() {
  digitalWrite(PIN_LED_MERAH, HIGH);   // lamp test
  digitalWrite(PIN_LED_HIJAU, HIGH);

  _tulisBaris(0, "SMART PARKING");
  _tulisBaris(1, "GATE SYSTEM v1.0");
  delay(1500);

  _tulisBaris(0, "MEMUAT SISTEM...");
  for (uint8_t i = 0; i < 16; i++) {   // progress bar 16 kolom
    lcd.setCursor(i, 1);
    lcd.write((uint8_t)255);           // blok penuh (karakter ROM LCD)
    delay(60);
  }
  delay(300);

  digitalWrite(PIN_LED_MERAH, LOW);
  digitalWrite(PIN_LED_HIJAU, LOW);
  _resetCacheLcd();                    // paksa layar digambar ulang
}

// ============================================================
// tampilkanMenuConfig(nilaiSementara)
// Menu teknisi. Hanya menulis ulang jika nilai berubah.
// ============================================================
void tampilkanMenuConfig(int nilaiSementara) {
  nilaiSementara = constrain(nilaiSementara, 0, 99);

  if (_lcdStatusTerakhir == STATUS_KONFIG &&
      _lcdTotalTerakhir  == nilaiSementara) return;

  char buf[17];
  _tulisBaris(0, "ATUR MAX SLOT");
  snprintf(buf, sizeof(buf), "KAPASITAS: <%02d>", nilaiSementara);
  _tulisBaris(1, buf);

  _lcdStatusTerakhir = STATUS_KONFIG;
  _lcdTotalTerakhir  = nilaiSementara;
  _lcdSisaTerakhir   = -1;
}

// ============================================================
// perbaruiLcd(status, sisaSlot, totalSlot)
// Dipanggil tiap iterasi loop(). Dibatasi oleh REFRESH_LCD, dan
// hanya menulis jika status / angka slot berubah.
//
// CATATAN untuk STATUS_KONFIG: argumen totalSlot dipakai sebagai
// nilai sementara yang sedang diedit (delegasi ke
// tampilkanMenuConfig), jadi sketch cukup memanggil:
//   perbaruiLcd(statusSaatIni, hitungSisaSlot(), nilaiSementara);
// ============================================================
void perbaruiLcd(StatusSistem status, int terisiSlot, int totalSlot) {
  unsigned long sekarang = millis();
  if (sekarang - _lcdWaktuTerakhir < REFRESH_LCD) return;
  _lcdWaktuTerakhir = sekarang;

  if (status == STATUS_KONFIG) {
    tampilkanMenuConfig(totalSlot);
    return;
  }

  terisiSlot = constrain(terisiSlot,  0, 99);
  totalSlot = constrain(totalSlot, 0, 99);

  bool berubah = (status != _lcdStatusTerakhir);
  if (status == STATUS_SIAGA &&
      (terisiSlot != _lcdSisaTerakhir || totalSlot != _lcdTotalTerakhir)) {
    berubah = true;
  }
  if (!berubah) return;

  char buf[17];
  switch (status) {
    case STATUS_SIAGA:
      snprintf(buf, sizeof(buf), "TERISI: %02d/%02d", terisiSlot, totalSlot);
      _tulisBaris(0, buf);
      _tulisBaris(1, "SILAKAN MENDEKAT");
      break;
    case STATUS_VALIDASI:
      _tulisBaris(0, "MENDETEKSI...");
      _tulisBaris(1, "MOHON TUNGGU 2S");
      break;
    case STATUS_BUKA:
      _tulisBaris(0, "MEMBUKA GERBANG");
      _tulisBaris(1, "HARAP BERHENTI");
      break;
    case STATUS_LEWAT:
      _tulisBaris(0, "SILAKAN MASUK");
      _tulisBaris(1, "HATI-HATI JALAN");
      break;
    case STATUS_TUTUP:
      _tulisBaris(0, "MENUTUP GERBANG");
      _tulisBaris(1, "PALANG TURUN!");   // 16 kolom: versi manual (17) dipersingkat
      break;
    case STATUS_PENUH:
      _tulisBaris(0, "PARKIR PENUH!");
      _tulisBaris(1, "AKSES DITOLAK");
      break;
    default:
      return;
  }

  _lcdStatusTerakhir = status;
  _lcdSisaTerakhir   = terisiSlot;
  _lcdTotalTerakhir  = totalSlot;
}

// ============================================================
// perbaruiLed(status)
// Pola LED (matriks 5.2):
//   SIAGA, VALIDASI : Merah ON statis
//   BUKA,  TUTUP    : Merah kedip cepat
//   LEWAT           : Hijau ON statis
//   PENUH           : Merah kedip lambat
//   KONFIG          : Merah + Hijau kedip bersamaan (lambat)
// Non-blocking (millis). Fasa kedip di-reset saat status berganti
// sehingga selalu mulai dari kondisi menyala.
// ============================================================
void perbaruiLed(StatusSistem status) {
  static int8_t        statusLama = -1;
  static unsigned long waktuToggle = 0;
  static bool          fasaNyala = true;

  unsigned long sekarang = millis();

  if (status != statusLama) {
    statusLama  = status;
    fasaNyala   = true;
    waktuToggle = sekarang;
  }

  unsigned long interval = 0;   // 0 = tidak berkedip
  switch (status) {
    case STATUS_BUKA:
    case STATUS_TUTUP:   interval = KEDIP_CEPAT;  break;
    case STATUS_PENUH:
    case STATUS_KONFIG:  interval = KEDIP_LAMBAT; break;
    default: break;
  }

  if (interval > 0 && (sekarang - waktuToggle) >= interval) {
    fasaNyala   = !fasaNyala;
    waktuToggle = sekarang;
  }

  bool merah, hijau;
  switch (status) {
    case STATUS_SIAGA:
    case STATUS_VALIDASI: merah = true;      hijau = false;     break;
    case STATUS_BUKA:
    case STATUS_TUTUP:
    case STATUS_PENUH:    merah = fasaNyala; hijau = false;     break;
    case STATUS_LEWAT:    merah = false;     hijau = true;      break;
    case STATUS_KONFIG:   merah = fasaNyala; hijau = fasaNyala; break;
    default:              merah = true;      hijau = false;     break; // fail-safe: merah
  }

  digitalWrite(PIN_LED_MERAH, merah ? HIGH : LOW);
  digitalWrite(PIN_LED_HIJAU, hijau ? HIGH : LOW);
}

#endif
