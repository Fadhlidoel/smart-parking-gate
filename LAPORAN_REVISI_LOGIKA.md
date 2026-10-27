# LAPORAN REVISI SISTEM & PEMBARUAN LOGIKA SMART PARKING GATE
**Dokumen Resmi Pembaruan Kode & Penyelarasan Dokumen Proyek**
*Tanggal Pembaruan: 27 Oktober 2026*  
*Target Pembaca: Seluruh Anggota Tim (Anggota 1 s.d. Anggota 6)*

---

## 1. Ringkasan Eksekutif

Laporan ini merangkum perbaikan teknis pada simulasi Wokwi, penyempurnaan logika keselamatan (*safety interlock*), penambahan filter waktu sensor, serta sinkronisasi antara implementasi kode (`.ino`, `.h`, `.json`) dengan dokumen proyek ([README.md](file:///D:/Project%202026/smart-parking-gate/README.md) dan [manual_book_smart_parking_gate.md](file:///D:/Project%202026/smart-parking-gate/manual_book_smart_parking_gate.md)).

Sistem kini telah teruji berjalan stabil pada simulator Wokwi tanpa *freeze*, palang terlindung dari risiko menutup mendadak karena *glitch* sensor, dan gerbang tidak akan macet terbuka (*deadlock*) jika kendaraan batal melintas.

---

## 2. Tabel Komparasi: Logika Lama vs. Logika Baru

| Area Sistem | Logika Lama | Logika Baru (Revisi) | Alasan & Dampak Keamanan |
| :--- | :--- | :--- | :--- |
| **Clearance Sensor 2 (Palang Menutup)** | Palang langsung diperintahkan turun begitu pembacaan Sensor 2 berubah dari $\le 100\text{ cm}$ menjadi $> 100\text{ cm}$ (hanya 1 siklus *loop*). | Sensor 2 harus mendeteksi objek $\le 100\text{ cm}$, kemudian area sensor harus kosong ($> 100\text{ cm}$) **stabil selama minimal 1000 ms** (`DURASI_CLEARANCE`). Jika objek terdeteksi lagi sebelum 1 detik selesai, pewaktu diulang dari nol. | **Anti-Benturan (*Anti-Crush*):** Mencegah palang turun saat bodi mobil masih melintas jika terjadi kegagalan pantulan gema akustik sesaat (*acoustic drop/glitch*). |
| **Batas Waktu Tunggu Lintasan (`STATUS_LEWAT`)** | Tidak ada batas waktu (*infinite wait*). Jika Sensor 2 tidak pernah tersentuh, palang berada di $90^\circ$ selamanya. | Diberikan batas waktu **30 detik** (`TIMEOUT_LEWAT`). Jika dalam 30 detik sejak palang terbuka penuh Sensor 2 tidak tersentuh, palang menutup otomatis secara bertahap ke $0^\circ$. | **Anti-Macet (*Anti-Deadlock*):** Jika pengendara membatalkan niat masuk/mundur setelah palang naik, gerbang otomatis kembali mengunci tanpa intervensi manual. |
| **Pencatatan Slot pada Jalur Timeout** | N/A (belum ada *timeout*). | Jika palang menutup karena *timeout* 30 detik (bukan karena mobil melewati Sensor 2), fungsi `tambahSlot()` **TIDAK dipanggil**. | **Integritas Data:** Kuota slot parkir tidak berkurang jika kendaraan fisik tidak benar-benar masuk. |
| **Reset Validasi Sensor 1 (`STATUS_SIAGA`)** | Status internal validasi tidak di-reset saat terdeteksi kendaraan baru di Sensor 1. | Fungsi `resetValidasiMasuk()` dipanggil tepat saat mendeteksi kendaraan baru masuk batas deteksi. | **Pencegahan Bypass:** Memastikan setiap kendaraan baru benar-benar diuji diam stabil selama 2000 ms penuh. |
| **Akses Menu Konfigurasi (`STATUS_KONFIG`)** | Buku manual menyebutkan hanya bisa dari `STATUS_SIAGA`, tetapi kode juga mengizinkan dari `STATUS_PENUH`. | Dokumen dan kode diselaraskan: Tombol SET ditahan 3 detik diizinkan dari `STATUS_SIAGA` maupun `STATUS_PENUH`, dengan syarat mutlak **palang berada di posisi $0^\circ$ (tertutup)**. | **Operasional Lapangan:** Saat kuota penuh, teknisi harus bisa menambah kuota slot tanpa perlu merestart mikrokontroler. |
| **Ambang Batas Jarak Deteksi** | Dokumen teks menuliskan $< 100\text{ cm}$, kode program memakai $\le 100\text{ cm}$. | Seluruh dokumen dan komentar kode diselaraskan menjadi $\le 100\text{ cm}$. | Konsistensi batasan matematis antara dokumen teknis dan eksekusi program. |
| **Konfigurasi Wokwi (`diagram.json`)** | Terdapat *field* `"dependencies": { "LiquidCrystal_I2C": "1.1.0" }`. | *Field* dikosongkan menjadi `"dependencies": {}`. Library LCD diletakkan di `libraries.txt`. | **Mengatasi Simulasi Macet:** Menghilangkan *error* tak kasat mata di browser akibat Wokwi mengira dependensi tersebut adalah *custom chip* GitHub. |

---

## 3. Rincian Berkas yang Mengalami Perubahan

### A. [config.h](file:///D:/Project%202026/smart-parking-gate/config.h)
1. **Penambahan Parameter Pewaktuan Baru:**
   ```c
   #define DURASI_CLEARANCE       1000     // Sensor 2 harus kosong stabil selama ini sebelum palang menutup (ms)
   #define TIMEOUT_LEWAT         30000     // Batas tunggu mobil menyentuh Sensor 2 sejak palang terbuka penuh (ms)
   ```
2. **Pembaruan Prototipe Fungsi Sensor:**
   ```c
   void resetSensorKeluar();            // Reset status Sensor 2 setiap kali palang selesai membuka
   bool sensorKeluarPernahTerdeteksi(); // True jika Sensor 2 sudah terhalang sejak reset terakhir
   ```
3. **Penyelarasan Komentar Batas Jarak:**
   `JARAK_DETEKSI` diselaraskan menjadi `(<= 100 cm ada mobil)`.
4. **Penambahan Kasus Uji QA:**
   Menambahkan `TC-07` (uji jeda 1 detik Sensor 2) dan `TC-08` (uji batas waktu 30 detik).

---

### B. [module_sensor.h](file:///D:/Project%202026/smart-parking-gate/module_sensor.h)
1. **Variabel Pelacak Internal Sensor 2:**
   Ditambahkan variabel pewaktu non-blocking:
   - `keluarPernahTerdeteksi`: Menandai apakah bodi kendaraan sudah mulai memasuki area Sensor 2.
   - `keluarMenungguKosong`: Menandai fase hitung mundur jeda 1 detik.
   - `waktuMulaiKosongKeluar`: Menyimpan stempel waktu `millis()` saat area Sensor 2 mulai kosong.
2. **Logika Baru `cekMobilLewat()`:**
   - Objek terdeteksi ($\le 100\text{ cm}$): mengaktifkan penanda deteksi dan membatalkan hitungan kosong jika sedang berjalan.
   - Objek menjauh ($> 100\text{ cm}$): memulai pencacahan waktu `millis()`.
   - Mengembalikan nilai `true` hanya jika durasi kosong telah mencapai $\ge 1000\text{ ms}$.
3. **Fungsi Pendukung Baru:**
   - `resetSensorKeluar()`: Mengembalikan semua status Sensor 2 ke kondisi awal sesaat setelah palang terangkat sempurna.
   - `sensorKeluarPernahTerdeteksi()`: Digunakan oleh FSM utama untuk memastikan bahwa proteksi *timeout* 30 detik dinonaktifkan jika kendaraan sudah berada di tengah gerbang.

---

### C. [sketch.ino](file:///D:/Project%202026/smart-parking-gate/sketch.ino)
1. **Variabel Tambahan:**
   - `unsigned long waktuMulaiLewat`: Mencatat waktu saat palang mencapai $90^\circ$ (`SUDUT_BUKA`).
   - `bool logSensorKeluar`: Mengontrol agar pesan serial konfirmasi kendaraan di Sensor 2 hanya tercetak sekali (tidak membanjiri Serial Monitor).
2. **Pembaruan State `STATUS_BUKA`:**
   Saat palang sampai ke $90^\circ$:
   - Memanggil `resetSensorKeluar()`.
   - Menyetel `waktuMulaiLewat = millis()`.
   - Berpindah ke `STATUS_LEWAT`.
3. **Penyempurnaan State `STATUS_LEWAT`:**
   - **Kondisi 1 (Mobil Selesai Melintas):** Memeriksa `cekMobilLewat()`. Jika bernilai `true` (kosong stabil 1 detik), memanggil `tambahSlot()`, membunyikan `bunyiGerak()`, dan menutup palang (`STATUS_TUTUP`).
   - **Kondisi 2 (Mobil Sedang di Sensor 2):** Memeriksa `sensorKeluarPernahTerdeteksi()`. Sistem menahan palang tetap di atas selama kendaraan belum tuntas lewat.
   - **Kondisi 3 (Timeout 30 Detik):** Jika Sensor 2 belum pernah terhalang dan waktu telah melebihi 30 detik, palang langsung menutup ke `STATUS_TUTUP` **tanpa menambah hitungan kuota**.
4. **Pembersihan State `STATUS_SIAGA`:**
   Memanggil `resetValidasiMasuk()` sesaat sebelum beralih ke `STATUS_VALIDASI`.

---

### D. [diagram.json](file:///D:/Project%202026/smart-parking-gate/diagram.json)
1. Menghapus dependensi eksternal yang salah target (`"dependencies": {}`).
2. Menjamin kompatibilitas penuh dengan `libraries.txt`.

---

### E. Penyelarasan Dokumen ([README.md](file:///D:/Project%202026/smart-parking-gate/README.md) & [manual_book_smart_parking_gate.md](file:///D:/Project%202026/smart-parking-gate/manual_book_smart_parking_gate.md))
1. **Karakter LCD 16 Kolom:**
   - Mengubah teks baris 2 `STATUS_TUTUP` dari `"AWAS PALANG TURUN"` (17 karakter) menjadi `"PALANG TURUN!"` (13 karakter, pas 16 kolom).
   - Mengubah teks baris 2 `STATUS_KONFIG` dari `"KAPASITAS: < XX >"` (17 karakter) menjadi `"KAPASITAS: <XX>"` (15 karakter).
2. **Keterbukaan Mengenai Karakteristik *Non-blocking*:**
   Dokumen diperbarui untuk menerangkan bahwa sistem murni *non-blocking* di dalam `loop()`, dengan dua pengecualian teknis yang terukur:
   - `delay()` pada `tampilkanSplash()` di `setup()` (hanya saat *booting*).
   - Batas tunggu maksimal 15 ms pada fungsi `pulseIn()` di modul ultrasonik.
3. **Matriks Pengujian QA yang Disinkronkan:**
   Tabel skenario pengujian di README dan Manual Book kini seragam memiliki 8 skenario pengujian (`TC-01` sampai `TC-08`).

---

## 4. Matriks Kasus Uji QA Resmi (TC-01 s.d. TC-08)

| ID Uji | Skenario Pengujian | Prosedur Simulasi Wokwi | Respon yang Diharapkan |
| :---: | :--- | :--- | :--- |
| **TC-01** | Filter Pejalan Kaki / Objek Sekilas | Geser slider Sensor 1 ke $\le 100\text{ cm}$ selama 1 detik, lalu kembalikan ke $> 100\text{ cm}$. | Palang tetap tertutup ($0^\circ$). Timer validasi ter-reset. Status kembali `STATUS_SIAGA`. |
| **TC-02** | Validasi Mobil Masuk | Tahan slider Sensor 1 pada $\le 100\text{ cm}$ selama minimal 2 detik penuh. | Palang naik perlahan ke $90^\circ$, LED Hijau menyala, status masuk `STATUS_LEWAT`. |
| **TC-03** | Penolakan Kuota Penuh | Naikkan kuota terisi hingga sisa slot $= 0$. Berikan picuan pada Sensor 1. | Palang tetap terkunci di $0^\circ$. LED Merah berkedip lambat. Akses ditolak. |
| **TC-04** | Proteksi Tabrakan Palang | Saat palang terbuka, geser Sensor 2 ke $\le 100\text{ cm}$ dan biarkan di sana. | Palang tetap bertahan di $90^\circ$ dan dilarang turun selama sensor masih mendeteksi bodi mobil. |
| **TC-05** | *Safety Interlock* Tombol Config | Tahan tombol SET selama 3 detik saat palang sedang membuka atau mobil sedang melintas. | Perintah diabaikan. Menu pengaturan kuota ditolak demi keselamatan pengendara. |
| **TC-06** | Ketahanan Memori EEPROM | Ubah kuota slot menjadi 25 melalui menu konfigurasi, simpan, lalu tekan tombol *Restart* Wokwi. | Setelah menyala kembali, kapasitas slot tetap tersimpan bernilai 25. |
| **TC-07** | **Filter Clearance 1000 ms (Baru)** | Saat palang di $90^\circ$, geser Sensor 2 ke $\le 100\text{ cm}$, geser ke $> 100\text{ cm}$ selama 0.5 detik, lalu geser lagi ke $\le 100\text{ cm}$. | Palang tetap berada di $90^\circ$. Palang baru turun setelah Sensor 2 benar-benar kosong stabil selama 1 detik penuh. Slot terisi bertambah 1. |
| **TC-08** | **Auto-Close Timeout 30s (Baru)** | Picu pembukaan palang via Sensor 1, namun **jangan sentuh** Sensor 2 sama sekali selama 30 detik. | Palang otomatis turun perlahan ke $0^\circ$. Sistem kembali ke siaga dan **slot terisi tidak bertambah**. |

---

## 5. Agenda Tindak Lanjut untuk Tim

1. **Anggota 4 (Audio & Buzzer):**
   - Mengisi implementasi nada non-blocking pada [module_audio.h](file:///D:/Project%202026/smart-parking-gate/module_audio.h) (bip ganda saat tervalidasi, bunyi peringatan berkala saat palang turun, dan nada panjang saat slot penuh).
2. **Integrasi Peringatan Kuota Penuh:**
   - Memanggil `bunyiPenuh()` saat Sensor 1 mendeteksi kendaraan ketika sistem berada dalam kondisi `STATUS_PENUH`.
3. **Pengujian Menyeluruh oleh Anggota 6 (QA):**
   - Menjalankan uji coba TC-01 sampai TC-08 pada simulator Wokwi untuk bahan dokumentasi laporan dan presentasi proyek.
