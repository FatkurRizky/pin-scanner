# Pin Scanner — ESP8266

Sketch kecil untuk **mencari pin tombol fisik** di board ESP8266 yang pinout-nya
nggak terdokumentasi (mis. perangkat NetherCap). Semua kandidat pin dibaca terus
dan ditampilkan live di browser HP.

Dipakai kalau tombol di perangkat "nggak jalan" tapi nggak tahu tombolnya
tersambung ke pin mana.

## Cara kerja

- `A0` dibaca sebagai ADC (satu-satunya pin analog di ESP8266).
- GPIO `0, 2, 4, 5, 12, 13, 14, 15, 16` dibaca digital dengan `INPUT_PULLUP`.
- Nilai "idle" dicatat sekali saat boot; baris yang berubah dari kondisi idle
  ditandai hijau di halaman.
- Perubahan juga dicetak ke serial 115200.

## Pakai

```bash
cd pin_scanner
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2:eesz=4M2M,xtal=160 --build-path ./build .
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp8266:esp8266:nodemcuv2:eesz=4M2M,xtal=160 --input-dir ./build
```

1. Sambung HP ke AP `PIN-SCANNER` (password `scan1234`).
2. Buka `http://192.168.4.1`.
3. Pencet tombol satu per satu, lihat baris mana yang berubah.

Halaman menampilkan:

```
A0 : 1023  (A0 mengambang / kosong)
Pin            Nilai  Idle saat boot
D3  (GPIO0)     1      1
D4  (GPIO2)     1      1
D2  (GPIO4)     0      1     <- baris hijau: pin ini yang berubah
...
```

## Membaca hasil

| Yang terlihat | Artinya |
| :--- | :--- |
| Satu baris GPIO berubah `1 -> 0` saat tombol ditekan | tombol aktif LOW di pin itu (`INPUT_PULLUP`) |
| `A0` berubah dari ~1023 ke nilai lain, berbeda tiap tombol | tombol di satu pin ADC (rangkain resistor) |
| `A0` tetap 1023 | `A0` tidak tersambung ke tombol |
| Nggak ada yang berubah sama sekali | tombol memakai pin lain / tidak tersambung / kabel putus |

Hasilnya dipakai untuk menyetel `BTN_ADC`, `BTN_PIN_ADC`, atau daftar `BTN_PINS`
di sketch ZeroTwin (ESP8266 maupun ESP32).

## Catatan

- Sketch ini **mengganti** firmware yang ada di board (flash 0x0). Simpan `.bin`
  firmware lamanya dulu kalau mau balik.
- Pin `GPIO16` di ESP8266 tidak punya pull-up internal yang normal; kalau tombolnya
  di situ, nilainya bisa tidak stabil — itu informasi, bukan bug.
- Hasil compile: RAM 36%, flash 272520 B (core esp8266 3.1.2).
