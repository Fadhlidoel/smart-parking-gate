#include <Arduino.h>
#include <Wire.h>
#include <Servo.h>

#include "config.h"
#include "module_sensor.h"
#include "module_display.h"
#include "module_audio.h"
#include "module_storage.h"

StatusSistem statusSaatIni = STATUS_SIAGA;
int sudutPalang = SUDUT_TUTUP;
int kapasitasMaks = SLOT_DEFAULT;
int slotTerisi = 0;

Servo palangServo;
int targetSudut = SUDUT_TUTUP;
unsigned long waktuGerakTerakhir = 0;
unsigned long waktuMulaiLewat = 0;     // Waktu palang terbuka penuh (awal hitungan TIMEOUT_LEWAT)
bool logSensorKeluar = false;          // Penanda log "Sensor 2 terhalang" sudah dicetak

bool gerakkanServoHalus() {
  if (millis() - waktuGerakTerakhir >= JEDA_SERVO) {
    waktuGerakTerakhir = millis();
    
    if (sudutPalang < targetSudut) {
      sudutPalang += LANGKAH_SERVO;
      palangServo.write(sudutPalang);
    } else if (sudutPalang > targetSudut) {
      sudutPalang -= LANGKAH_SERVO;
      palangServo.write(sudutPalang);
    }
  }
  return (sudutPalang == targetSudut);
}

void setup() {
  Serial.begin(9600);
  delay(100);
  Serial.println(F("[SYSTEM]: Memulai Inisialisai Program..."));

  palangServo.attach(PIN_SERVO);
  palangServo.write(SUDUT_TUTUP);
  sudutPalang = SUDUT_TUTUP;
  targetSudut = SUDUT_TUTUP;

  inisialisasiSensor();
  inisialisasiDisplay();
  inisialisasiAudio();
  inisialisasiStorage();
  tampilkanSplash();

  Serial.println(F("[SYSTEM]: Setup Selesai. Masuk ke STATUS_SIAGA"));
}

void loop() {
  bool palangSampaiTujuan = gerakkanServoHalus();

  perbaruiAudio();
  perbaruiLed(statusSaatIni);
  perbaruiLcd(statusSaatIni, hitungSisaSlot(), kapasitasMaks);

  switch (statusSaatIni) {

    case STATUS_SIAGA:
      targetSudut = SUDUT_TUTUP;

      if (cekTombolSetTahan() && sudutPalang == SUDUT_TUTUP) {
        Serial.println(F("\n============================="));
        Serial.println(F("[CONFIG]: Masuk ke Menu Konfigurasi Slot Parkir"));
        Serial.print(F("[CONFIG]: Slot saat ini:"));
        Serial.println(kapasitasMaks);
        Serial.println(F("[CONFIG]: Gunakan tombol Up/Down lalu tekan Set"));
        Serial.println(F("============================="));
        statusSaatIni = STATUS_KONFIG;
        break;
      }
      if (hitungSisaSlot() <= 0) {
        statusSaatIni = STATUS_PENUH;
      } else {
        long jarakMasuk = bacaJarak(PIN_TRIG_MASUK, PIN_ECHO_MASUK);
        if (jarakMasuk > 0 && jarakMasuk <= JARAK_DETEKSI) {
          Serial.println(F("[FSM]: Mobil Terdeteksi di Sensor 1. Memulai validasi 2 detik..."));
          resetValidasiMasuk();  // Hapus sisa status validasi dari mobil sebelumnya
          statusSaatIni = STATUS_VALIDASI;
        }
      }
      break;

    case STATUS_VALIDASI:
      targetSudut = SUDUT_TUTUP;
      if (cekMobilMasuk()) {
        Serial.println(F("[FSM]: Validasi Sukses! Membuka Palang..."));
        bunyiMasuk();
        targetSudut = SUDUT_BUKA;
        statusSaatIni = STATUS_BUKA;
      } else if (!masukSedangTerdeteksi) {
        Serial.println(F("[FSM]: Mobil keluar sebelum 2 detik, kembali ke STATUS_SIAGA"));
        resetValidasiMasuk();
        statusSaatIni = STATUS_SIAGA;
      }
      break;
    
    case STATUS_BUKA:
      if (palangSampaiTujuan && sudutPalang == SUDUT_BUKA) {
        Serial.println(F("[FSM]: Palang terbuka penuh. Menunggu mobil lewat (batas 30 detik)..."));
        resetSensorKeluar();
        waktuMulaiLewat = millis();
        logSensorKeluar = false;
        statusSaatIni = STATUS_LEWAT;
      }
      break;
    
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

    case STATUS_TUTUP:
      if (palangSampaiTujuan && sudutPalang == SUDUT_TUTUP) {
        Serial.println(F("[FSM]: Palang tertutup sempurna. Kembali ke STATUS_SIAGA."));
        matikanAudio();
        statusSaatIni = STATUS_SIAGA;
      }
      break;
    
    case STATUS_PENUH:
      targetSudut = SUDUT_TUTUP;
      if (cekTombolSetTahan() && sudutPalang == SUDUT_TUTUP) {
        Serial.println(F("\n============================="));
        Serial.println(F("[CONFIG]: Masuk ke Menu Konfigurasi Slot Parkir"));
        Serial.print(F("[CONFIG]: Slot saat ini:"));
        Serial.println(kapasitasMaks);
        Serial.println(F("============================="));
        statusSaatIni = STATUS_KONFIG;
      } else if (hitungSisaSlot() > 0) {
        statusSaatIni = STATUS_SIAGA;
      } else {
        long jarakMasuk = bacaJarak(PIN_TRIG_MASUK, PIN_ECHO_MASUK);
        if (jarakMasuk > 0 && jarakMasuk <= JARAK_DETEKSI) {
          punyiPenuh();
        }
      }
      break;

    case STATUS_KONFIG:
      targetSudut = SUDUT_TUTUP;

      if (cekTombolUp()) {
        if (kapasitasMaks < SLOT_MAX) {
          kapasitasMaks++;
          bunyiKlik();
          Serial.print(F("[CONFIG]: Tombol UP -> Slot bertambah: "));
          Serial.print(kapasitasMaks);
          Serial.println(F(" Slot"));
        } else {
          Serial.println(F("[CONFIG]: Slot sudah mencapai nilai maksimum"));
        }
      }

      if (cekTombolDown()) {
        if (kapasitasMaks > SLOT_MIN) {
          kapasitasMaks--;
          bunyiKlik();
          Serial.print(F("[CONFIG]: Tombol DOWN -> Slot berkurang: "));
          Serial.print(kapasitasMaks);
          Serial.println(F(" Slot"));
        } else {
          Serial.println(F("[CONFIG]: Slot sudah mencapai nilai minimum"));
        }
      }

      if (cekTombolSet()) {
        simpanEeprom(kapasitasMaks);
        bunyiKlik();
        Serial.println(F("\n----------------------------------------"));
        Serial.print  (F("[CONFIG]: Tersimpan ke EEPROM! Kapasitas = "));
        Serial.println(kapasitasMaks);
        Serial.print  (F("[CONFIG]: Sisa Slot Aktif = "));
        Serial.println(hitungSisaSlot());
        Serial.println(F("[CONFIG]: Keluar dari Pengaturan -> Kembali SIAGA"));
        Serial.println(F("----------------------------------------\n"));
        statusSaatIni = STATUS_SIAGA;
      }
      break;
  }

}
