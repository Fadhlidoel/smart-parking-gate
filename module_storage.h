#ifndef MODULE_STORAGE_H
#define MODULE_STORAGE_H

#include <Arduino.h>
#include <EEPROM.h>
#include "config.h"         

struct DataTombol {
  uint8_t       pin;           
  bool          bacaTerakhir;  
  bool          stabil;        
  unsigned long waktuUbah;     
};

static DataTombol tombolUp   = { PIN_TOMBOL_UP,   HIGH, HIGH, 0 };
static DataTombol tombolDown = { PIN_TOMBOL_DOWN, HIGH, HIGH, 0 };
static DataTombol tombolSet  = { PIN_TOMBOL_SET,  HIGH, HIGH, 0 };

static unsigned long setWaktuTekan   = 0;      
static bool          setTahanTerpicu = false;  
static bool          setEventTahan   = false;  
static bool          setEventPendek  = false;  
static unsigned long setWaktuPendek  = 0;      

static bool deteksiTekan(DataTombol &t) {
  bool bacaan = digitalRead(t.pin);
  unsigned long sekarang = millis();

  if (bacaan != t.bacaTerakhir) {
    t.bacaTerakhir = bacaan;
    t.waktuUbah = sekarang;
  }

  if ((sekarang - t.waktuUbah) >= DEBOUNCE_TOMBOL && bacaan != t.stabil) {
    t.stabil = bacaan;
    if (t.stabil == LOW) {
      return true;  
    }
  }
  return false;
}

static void perbaruiTombolSet() {
  bool stabilSebelumnya = tombolSet.stabil;
  bool baruDitekan = deteksiTekan(tombolSet);
  unsigned long sekarang = millis();

  if (baruDitekan) {
    setWaktuTekan = sekarang;
    setTahanTerpicu = false;
  }

  if (tombolSet.stabil == LOW && !setTahanTerpicu &&
      (sekarang - setWaktuTekan) >= TAHAN_CONFIG) {
    setTahanTerpicu = true;
    setEventTahan = true;
    setEventPendek = false;  
  }

  if (stabilSebelumnya == LOW && tombolSet.stabil == HIGH && !setTahanTerpicu) {
    setEventPendek = true;
    setWaktuPendek = sekarang;
  }
}

void inisialisasiStorage() {
  pinMode(PIN_TOMBOL_UP,   INPUT_PULLUP);
  pinMode(PIN_TOMBOL_DOWN, INPUT_PULLUP);
  pinMode(PIN_TOMBOL_SET,  INPUT_PULLUP);
  bacaEeprom();
  slotTerisi = 0;
}

void bacaEeprom() {
  byte magic = EEPROM.read(ALAMAT_MAGIC);
  int nilai = EEPROM.read(ALAMAT_EEPROM);

  if (magic != NILAI_MAGIC || nilai < SLOT_MIN || nilai > SLOT_MAX) {
    nilai = SLOT_DEFAULT;
    EEPROM.update(ALAMAT_EEPROM, (byte)nilai);
    EEPROM.update(ALAMAT_MAGIC, (byte)NILAI_MAGIC);
    Serial.println(F("[EEPROM]: Tidak valid, reset ke default"));
    Serial.print(magic, HEX);
    Serial.print(F("). Menulis Default = "));
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
  Serial.print(F("[EEPROM]: Disimpan ke EEPROM. Kapasitas = "));
  Serial.print(EEPROM.read(ALAMAT_EEPROM));
  Serial.print(F(", magic = 0x"));
  Serial.println(EEPROM.read(ALAMAT_MAGIC), HEX);
}

bool cekTombolSetTahan() {
  perbaruiTombolSet();
  if (setEventTahan) {
    setEventTahan = false;
    return true;
  }
  return false;
}

bool cekTombolUp() {
  return deteksiTekan(tombolUp);
}

bool cekTombolDown() {
  return deteksiTekan(tombolDown);
}

bool cekTombolSet() {
  perbaruiTombolSet();
  if (setEventPendek) {
    setEventPendek = false;
    if (millis() - setWaktuPendek < 500) {
      return true;
    }
  }
  return false;
}

int hitungSisaSlot() {
  int sisa = kapasitasMaks - slotTerisi;
  return (sisa > 0) ? sisa : 0;
}

void tambahSlot() {
  if (slotTerisi < kapasitasMaks) {
    slotTerisi++;
  }
}

void kurangiSlot() {
  if (slotTerisi > 0) {
    slotTerisi--;
  }
}

#endif 