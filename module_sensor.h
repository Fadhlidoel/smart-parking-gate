
#ifndef MODULE_SENSOR_H
#define MODULE_SENSOR_H

#include <Arduino.h>
#include "config.h"

// Status internal sensor
static unsigned long waktuMulaiMasuk = 0;
static bool masukSedangTerdeteksi = false;
static bool masukSudahDihitung = false;

static bool keluarSedangTerdeteksi = false;

// Membaca jarak dalam cm
long bacaJarak(uint8_t pinTrig, uint8_t pinEcho) {
  digitalWrite(pinTrig, LOW);
  delayMicroseconds(2);

  digitalWrite(pinTrig, HIGH);
  delayMicroseconds(10);
  digitalWrite(pinTrig, LOW);

  unsigned long durasi = pulseIn(
    pinEcho, HIGH, TIMEOUT_ECHO
  );

  if (durasi == 0) {
    return -1;  // Tidak ada pantulan valid
  }

  return (long)(durasi * 0.0343 / 2.0);
}

// Inisialisasi kedua sensor
void inisialisasiSensor() {
  pinMode(PIN_TRIG_MASUK, OUTPUT);
  pinMode(PIN_ECHO_MASUK, INPUT);

  pinMode(PIN_TRIG_KELUAR, OUTPUT);
  pinMode(PIN_ECHO_KELUAR, INPUT);

  digitalWrite(PIN_TRIG_MASUK, LOW);
  digitalWrite(PIN_TRIG_KELUAR, LOW);
}

// True sekali setelah kendaraan terdeteksi
// stabil selama DURASI_VALIDASI
bool cekMobilMasuk() {
  long jarak = bacaJarak(
    PIN_TRIG_MASUK, PIN_ECHO_MASUK
  );

  bool terdeteksi =
    (jarak > 0 && jarak <= JARAK_DETEKSI);

  if (!terdeteksi) {
    masukSedangTerdeteksi = false;
    masukSudahDihitung = false;
    waktuMulaiMasuk = 0;
    return false;
  }

  if (!masukSedangTerdeteksi) {
    masukSedangTerdeteksi = true;
    waktuMulaiMasuk = millis();
    return false;
  }

  if (!masukSudahDihitung &&
      millis() - waktuMulaiMasuk >= DURASI_VALIDASI) {
    masukSudahDihitung = true;
    return true;
  }

  return false;
}

// True sekali setelah kendaraan melewati
// sensor keluar dan area sensor kembali kosong
bool cekMobilLewat() {
  long jarak = bacaJarak(
    PIN_TRIG_KELUAR, PIN_ECHO_KELUAR
  );

  bool terdeteksi =
    (jarak > 0 && jarak <= JARAK_DETEKSI);

  if (terdeteksi) {
    keluarSedangTerdeteksi = true;
    return false;
  }

  if (keluarSedangTerdeteksi) {
    keluarSedangTerdeteksi = false;
    return true;
  }

  return false;
}

// Batalkan validasi kendaraan masuk
void resetValidasiMasuk() {
  waktuMulaiMasuk = 0;
  masukSedangTerdeteksi = false;
  masukSudahDihitung = false;
}

#endif
