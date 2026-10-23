#include <Arduino.h>
#include <Servo.h>

#include "config.h"
// #include "module_sensor.h"
// #include "module_display.h"
// #include "module_audio.h"
#include "module_storage.h"

StatusSistem statusSaatIni = STATUS_SIAGA;
int sudutPalang = SUDUT_TUTUP;
int kapasitasMaks = SLOT_DEFAULT;
int slotTerisi = 0;

Servo palangServo;
int targetSudut = SUDUT_TUTUP;
unsigned long waktuGerakTerakhir = 0;

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

// UNTUK DATA DUMMY PENGUJIAN SEMENTARA //
unsigned long waktuBukaPenuh = 0;

bool cekMobilMasuk () {
  int detik = (millis() / 1000) % 30;
  return (detik ==10);
}

bool cekMobilLewat () {
  if (waktuBukaPenuh == 0) {
    waktuBukaPenuh = millis();
  }
  if (millis() - waktuBukaPenuh >= 3000) {
    waktuBukaPenuh = 0;
    return true;
  }
  return false;
}
void perbaruiDisplay(StatusSistem s, int sissa, int total) {}
void perbaruiLed(StatusSistem s) {}
void perbaruiAudio() {}
void inisialisasiDisplay() {}
void inisialisasiAudio() {}
void inisialisasiSensor() {}


void setup() {
  Serial.begin(9600);
  Serial.println(F("[SYSTEM]: Memulai Inisialisai Program..."));

  palangServo.attach(PIN_SERVO);
  palangServo.write(SUDUT_TUTUP);
  sudutPalang = SUDUT_TUTUP;
  targetSudut = SUDUT_TUTUP;

  inisialisasiSensor();
  inisialisasiDisplay();
  inisialisasiAudio();
  inisialisasiStorage();

  Serial.println(F("[SYSTEM]: Setup Selesai. Masuk ke STATUS_SIAGA"));
}

void loop() {
  bool palangSampaiTujuan = gerakkanServoHalus();

  perbaruiAudio();
  perbaruiLed(statusSaatIni);

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
      } else if (cekMobilMasuk()) {
        Serial.println(F("[FSM]: Mobil Tervalidasi. Membuka Palang..."));
        targetSudut = SUDUT_BUKA;
        statusSaatIni = STATUS_BUKA;
      }
      break;
    
    case STATUS_BUKA:
      if (palangSampaiTujuan && sudutPalang == SUDUT_BUKA) {
        Serial.println(F("[FSM]: Palang terbuka penuh. Menunggu mObil lewat..."));
        statusSaatIni = STATUS_LEWAT;
      }
      break;
    
    case STATUS_LEWAT:
      if (cekMobilLewat()) {
        Serial.println(F("[FSM]: Palang tertutup sempurna. Status Siaga"));
        slotTerisi++;
        targetSudut = SUDUT_TUTUP;
        statusSaatIni = STATUS_TUTUP;
      }
      break;

    case STATUS_TUTUP:
      if (palangSampaiTujuan && sudutPalang == SUDUT_TUTUP) {
        Serial.println(F("[FSM]: Palang tertutup sempurna. Kembali Siaga."));
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
      }
      break;

    case STATUS_KONFIG:
      targetSudut = SUDUT_TUTUP;

      if (cekTombolUp()) {
        if (kapasitasMaks < SLOT_MAX) {
          kapasitasMaks++;
          Serial.print(F("[CONFIG]: Tombol UP untuk menambah slot"));
          Serial.print(kapasitasMaks);
          Serial.println(F("Slot"));
        } else {
          Serial.println((F("[CONFIG]: Slot Sudah menacpai nilai maksimum")));
        }
      }

      if (cekTombolDown()) {
        if (kapasitasMaks > SLOT_MIN) {
          kapasitasMaks--;
          Serial.print(F("[CONFIG]: Tombol DOWN untuk menambah slot"));
          Serial.print(kapasitasMaks);
          Serial.println(F("Slot"));
        } else {
          Serial.println((F("[CONFIG]: Slot Sudah menacpai nilai minimum")));
        }
      }

      if (cekTombolSet()) {
        simpanEeprom(kapasitasMaks);
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
