// ============================================================================
// PIN SCANNER — cari pin tombol fisik di board NetherCap
// Semua kandidat pin dibaca terus dan ditampilkan live di HP (192.168.4.1)
// plus serial 115200. Pencet tombol satu-satu sambil lihat halaman ini:
// pin yang angkanya berubah = pin tombol itu.
//
// Tidak mengubah flash lain; habis ini tinggal flash balik firmware yang mau dipakai.
// ============================================================================
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

static const char* AP_NAME = "PIN-SCANNER";
static const char* AP_PASS = "scan1234";

struct PinDef { uint8_t gpio; const char* label; bool digital; };

// A0 = satu-satunya ADC. Sisanya GPIO digital yang umum dipakai tombol.
static const PinDef PINS[] = {
  { 0,  "D3  (GPIO0)",  false },
  { 2,  "D4  (GPIO2)",  false },
  { 4,  "D2  (GPIO4)",  false },
  { 5,  "D1  (GPIO5)",  false },
  { 12, "D6  (GPIO12)", false },
  { 13, "D7  (GPIO13)", false },
  { 14, "D5  (GPIO14)", false },
  { 15, "D8  (GPIO15)", false },
  { 16, "D0  (GPIO16)", false },
};
static const uint8_t NPINS = sizeof(PINS) / sizeof(PINS[0]);

ESP8266WebServer server(80);

int   adcRaw   = 0;
uint8_t digState[NPINS];
uint8_t digBase[NPINS];       // nilai "idle" saat boot
bool    adcActive = false;

void samplePins(){
  adcRaw = analogRead(A0);
  adcActive = (adcRaw < 1015);                 // A0 nyambung ke sesuatu kalau bukan mendekati 1023
  for(uint8_t i = 0; i < NPINS; i++) digState[i] = digitalRead(PINS[i].gpio);
}

String json(){
  samplePins();
  String s = "{\"adc\":" + String(adcRaw) + ",\"adcActive\":" + String(adcActive ? "true" : "false") + ",\"pins\":[";
  for(uint8_t i = 0; i < NPINS; i++){
    if(i) s += ",";
    s += "{\"label\":\"" + String(PINS[i].label) + "\",\"v\":" + String(digState[i]) +
         ",\"base\":" + String(digBase[i]) + ",\"changed\":" + String(digState[i] != digBase[i] ? "true" : "false") + "}";
  }
  return s + "]}";
}

const char PAGE[] PROGMEM = R"HTML(
<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Pin Scanner</title><style>
body{font-family:monospace;background:#111;color:#eee;margin:0;padding:1em}
h2{font-family:Arial;margin:.2em 0}
.adc{font-size:1.6em;background:#1b1b1b;padding:.6em;border-radius:8px;margin:.5em 0}
table{width:100%;border-collapse:collapse;margin-top:.6em}
td,th{padding:.45em;border-bottom:1px solid #333;text-align:left}
tr.hit{background:#1f7a3d;color:#fff;font-weight:bold}
</style></head><body>
<h2>Pin Scanner</h2>
<div class="adc">A0 : <b id="adc">-</b> <span id="adcst" style="font-size:.6em;color:#8ab4f8"></span></div>
<table id="tb"><tr><th>Pin</th><th>Nilai</th><th>Idle saat boot</th></tr></table>
<p style="color:#8ab4f8;font-size:.85em">Pencet tombol satu-satu. Baris hijau = pin yang berubah dari kondisi idle.</p>
<script>
async function tick(){
  try{
    let s=await (await fetch('/api/pins')).json();
    document.getElementById('adc').textContent=s.adc;
    document.getElementById('adcst').textContent=s.adcActive?'(A0 tersambung ke rangkaian)':'(A0 mengambang / kosong)';
    let t=document.getElementById('tb');
    t.innerHTML='<tr><th>Pin</th><th>Nilai</th><th>Idle saat boot</th></tr>';
    s.pins.forEach(p=>{t.innerHTML+='<tr class="'+(p.changed?'hit':'')+'"><td>'+p.label+
      '</td><td>'+p.v+'</td><td>'+p.base+'</td></tr>';});
  }catch(e){}
}
tick();setInterval(tick,400);
</script></body></html>
)HTML";

void setup(){
  Serial.begin(115200);
  delay(200);

  // pin sebagai input pullup -> tombol ke GND akan terbaca 0 saat ditekan
  for(uint8_t i = 0; i < NPINS; i++) pinMode(PINS[i].gpio, INPUT_PULLUP);
  delay(50);
  for(uint8_t i = 0; i < NPINS; i++) digBase[i] = digitalRead(PINS[i].gpio);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_NAME, AP_PASS);
  server.on("/", HTTP_GET, [](){ server.send_P(200, "text/html", PAGE); });
  server.on("/api/pins", HTTP_GET, [](){ server.send(200, "application/json", json()); });
  server.onNotFound([](){ server.sendHeader("Location", "http://192.168.4.1/"); server.send(302, "text/plain", ""); });
  server.begin();

  Serial.printf("[SCAN] AP %s http://%s\n", AP_NAME, WiFi.softAPIP().toString().c_str());
  Serial.print("[SCAN] idle:");
  Serial.print(" A0="); Serial.print(analogRead(A0));
  for(uint8_t i = 0; i < NPINS; i++){ Serial.print(" "); Serial.print(PINS[i].gpio); Serial.print("="); Serial.print(digBase[i]); }
  Serial.println();
}

void loop(){
  server.handleClient();
  static uint32_t last = 0, lastPrint = 0;
  if(millis() - last > 50){
    last = millis();
    uint8_t before[NPINS]; int adcBefore = adcRaw;
    for(uint8_t i = 0; i < NPINS; i++) before[i] = digState[i];
    samplePins();
    bool ch = (adcBefore != adcRaw);
    for(uint8_t i = 0; i < NPINS; i++) if(before[i] != digState[i]) ch = true;
    if(ch && millis() - lastPrint > 150){
      lastPrint = millis();
      Serial.print("[SCAN] A0="); Serial.print(adcRaw);
      for(uint8_t i = 0; i < NPINS; i++){ Serial.print(" "); Serial.print(PINS[i].gpio); Serial.print("="); Serial.print(digState[i]); }
      Serial.println();
    }
  }
  delay(2);
}
