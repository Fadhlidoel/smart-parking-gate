#ifndef MODULE_AUDIO_H
#define MODULE_AUDIO_H

#include <Arduino.h>
#include "config.h"

// Status internal untuk Finite State Machine Audio
enum ModeAudio {
  AUDIO_DIAM = 0,
  AUDIO_MASUK,
  AUDIO_GERAK,
  AUDIO_PENUH,
  AUDIO_KLIK
};

static ModeAudio stateAudio = AUDIO_DIAM;
static unsigned long waktuNadaTerakhir = 0;
static uint8_t tahapNada = 0;

void inisialisasiAudio() {
  pinMode(PIN_BUZZER, OUTPUT);
  matikanAudio();
}

void matikanAudio() {
  noTone(PIN_BUZZER);
  stateAudio = AUDIO_DIAM;
  tahapNada = 0;
}

// Bip 2x konfirmasi kendaraan tervalidasi
void bunyiMasuk() {
  stateAudio = AUDIO_MASUK;
  tahapNada = 0;
  waktuNadaTerakhir = millis();
  tone(PIN_BUZZER, NADA_MASUK, BIP_PENDEK);
}

// Nada peringatan saat servo palang turun (diulangi)
void bunyiGerak() {
  if (stateAudio != AUDIO_GERAK) {
    stateAudio = AUDIO_GERAK;
    tahapNada = 0;
    waktuNadaTerakhir = millis();
    tone(PIN_BUZZER, NADA_TUTUP, BIP_PENDEK);
  }
}

// Nada panjang tanda akses ditolak/penuh
void bunyiPenuh() {
  if (stateAudio != AUDIO_PENUH) {
    stateAudio = AUDIO_PENUH;
    waktuNadaTerakhir = millis();
    tone(PIN_BUZZER, NADA_PENUH, BIP_PANJANG);
  }
}

// Umpan balik klik penekanan tombol
void bunyiKlik() {
  stateAudio = AUDIO_KLIK;
  waktuNadaTerakhir = millis();
  tone(PIN_BUZZER, NADA_KLIK, BIP_PENDEK);
}

// Dipanggil terus-menerus di loop() untuk merangkai nada kompleks tanpa delay()
void perbaruiAudio() {
  unsigned long sekarang = millis();

  // Matikan paksa bunyi peringatan palang jika gerbang sudah kembali diam
  if (stateAudio == AUDIO_GERAK && statusSaatIni != STATUS_TUTUP) {
    matikanAudio();
  }

  switch (stateAudio) {
    case AUDIO_MASUK:
      // Transisi: Bip 1 -> Jeda -> Bip 2 -> Selesai
      if (tahapNada == 0 && (sekarang - waktuNadaTerakhir >= BIP_PENDEK)) {
        tahapNada = 1;
        waktuNadaTerakhir = sekarang;
      } else if (tahapNada == 1 && (sekarang - waktuNadaTerakhir >= JEDA_NADA)) {
        tahapNada = 2;
        waktuNadaTerakhir = sekarang;
        tone(PIN_BUZZER, NADA_MASUK, BIP_PENDEK);
      } else if (tahapNada == 2 && (sekarang - waktuNadaTerakhir >= BIP_PENDEK)) {
        matikanAudio();
      }
      break;

    case AUDIO_GERAK:
      // Loop ritmik sirine berkelanjutan: Bip -> Jeda Panjang -> Bip
      if (tahapNada == 0 && (sekarang - waktuNadaTerakhir >= BIP_PENDEK)) {
        tahapNada = 1;
        waktuNadaTerakhir = sekarang;
      } else if (tahapNada == 1 && (sekarang - waktuNadaTerakhir >= JEDA_NADA * 2)) {
        tahapNada = 0;
        waktuNadaTerakhir = sekarang;
        tone(PIN_BUZZER, NADA_TUTUP, BIP_PENDEK);
      }
      break;

    case AUDIO_PENUH:
    case AUDIO_KLIK:
      // Tunggu durasi nada selesai sebelum beralih ke status diam
      if (sekarang - waktuNadaTerakhir >= (stateAudio == AUDIO_PENUH ? BIP_PANJANG : BIP_PENDEK)) {
        matikanAudio();
      }
      break;

    case AUDIO_DIAM:
    default:
      break;
  }
}

#endif // MODULE_AUDIO_H
