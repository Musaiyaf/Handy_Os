/*
 * HandyOS v1.1 — ESP32 + ST7735 Firmware
 * ========================================
 * Controls : UP=GPIO12  DOWN=GPIO14  LEFT=GPIO27  RIGHT=GPIO26  SEL=GPIO25
 * Display  : ST7735 128x160, CS=5 DC=2 RST=4 (VSPI)
 *
 * Menus (3×3 grid):
 *   WiFi | Bluetooth | Social
 *   Tools | Islam    | File
 *   Games | Settings | About
 *
 * Each menu has ≥ 2 sub-features.
 * WiFi menu has a 3rd sub-feature: OTA Web Updater (v1.1)
 */

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <BluetoothSerial.h>
#include <Preferences.h>
#include <time.h>
#include "boot_animation.h"
#include "main_menu_ui.h"

// ─── Display ────────────────────────────────────────────────────────────────
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// ─── Colour palette ─────────────────────────────────────────────────────────
#define C_BG   ST77XX_BLACK
#define C_TXT  ST77XX_WHITE
#define C_HDR  0x1082           // dark grey
#define C_SEL  ST77XX_CYAN
#define C_INT  ST77XX_BLUE
#define C_ERR  ST77XX_RED
#define C_OK   ST77XX_GREEN
#define C_YLW  ST77XX_YELLOW

// ─── Buttons ────────────────────────────────────────────────────────────────
#define BTN_UP    12
#define BTN_DOWN  14
#define BTN_LEFT  27
#define BTN_RIGHT 26
#define BTN_SEL   25

// ─── NVS ────────────────────────────────────────────────────────────────────
Preferences prefs;

// ─── BT Serial ──────────────────────────────────────────────────────────────
BluetoothSerial SerialBT;

// ─── State machine ──────────────────────────────────────────────────────────
enum AppState {
  ST_BOOT,
  ST_MENU,
  // WiFi
  ST_WIFI_SCAN, ST_WIFI_CONNECT, ST_WIFI_STATUS, ST_WIFI_OTA,
  // Bluetooth
  ST_BT_MONITOR, ST_BT_SEND,
  // Social
  ST_SOCIAL_QR, ST_SOCIAL_MSG,
  // Tools
  ST_TOOLS_CLOCK, ST_TOOLS_CALC,
  // Islam
  ST_ISLAM_PRAY, ST_ISLAM_QBL,
  // File
  ST_FILE_LOG, ST_FILE_VIEW,
  // Games
  ST_GAME_SNAKE, ST_GAME_REACT,
  // Settings
  ST_SET_BRIGHT, ST_SET_WIFI_CRED,
  // About
  ST_ABOUT_INFO, ST_ABOUT_DIAG,
};

AppState appState = ST_BOOT;
int      menuSel  = 0;   // 0-8

// ─── Button helpers ─────────────────────────────────────────────────────────
struct BtnState { bool prev; };
BtnState bUp, bDown, bLeft, bRight, bSel;

void initButtons() {
  pinMode(BTN_UP,    INPUT_PULLUP);
  pinMode(BTN_DOWN,  INPUT_PULLUP);
  pinMode(BTN_LEFT,  INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_SEL,   INPUT_PULLUP);
}
bool pressed(int pin, BtnState& s) {
  bool cur = (digitalRead(pin) == LOW);
  bool p   = cur && !s.prev;
  s.prev   = cur;
  return p;
}

// ─── UI helpers ─────────────────────────────────────────────────────────────
void drawHeader(const char* title) {
  tft.fillRect(0, 0, 128, 12, C_HDR);
  tft.setTextColor(C_SEL); tft.setTextSize(1);
  tft.setCursor(2, 3); tft.print(title);
}
void drawFooter(const char* hint) {
  tft.fillRect(0, 150, 128, 10, C_HDR);
  tft.setTextColor(C_TXT); tft.setTextSize(1);
  tft.setCursor(2, 152); tft.print(hint);
}
void clearBody() { tft.fillRect(0, 13, 128, 137, C_BG); }
void bodyText(int x, int y, uint16_t col, const char* txt, uint8_t sz = 1) {
  tft.setTextColor(col); tft.setTextSize(sz);
  tft.setCursor(x, y); tft.print(txt);
}

// ═══════════════════════════════════════════════════════════════════════════
// OTA WEB UPDATER
// ═══════════════════════════════════════════════════════════════════════════

// HTML page served to the browser (stored in flash)
static const char OTA_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>HandyOS OTA Update</title>
<style>
  body{background:#0a0a0a;color:#fff;font-family:monospace;display:flex;
       flex-direction:column;align-items:center;justify-content:center;
       min-height:100vh;margin:0;padding:20px;box-sizing:border-box;}
  h2{color:#07ff00;margin-bottom:6px;font-size:1.4em;}
  p{color:#888;font-size:.9em;margin:4px 0;}
  .card{background:#111;border:1px solid #222;border-radius:12px;
        padding:28px 24px;max-width:420px;width:100%;}
  label{display:block;background:#1a1a1a;border:2px dashed #333;
        border-radius:8px;padding:30px 16px;text-align:center;
        cursor:pointer;color:#555;transition:border-color .2s;}
  label:hover,label.has-file{border-color:#07ff00;color:#07ff00;}
  input[type=file]{display:none;}
  button{margin-top:16px;width:100%;padding:14px;background:#07ff00;
         color:#000;border:none;border-radius:8px;font-size:1em;
         font-weight:bold;cursor:pointer;letter-spacing:1px;}
  button:disabled{background:#1a4a1a;color:#333;cursor:not-allowed;}
  #prog{margin-top:16px;display:none;}
  #bar-wrap{background:#1a1a1a;border-radius:4px;height:14px;overflow:hidden;}
  #bar{height:100%;width:0%;background:#07ff00;transition:width .3s;}
  #pct{text-align:center;color:#07ff00;margin-top:6px;font-size:.9em;}
  #msg{text-align:center;margin-top:12px;font-size:.9em;}
  .ok{color:#07ff00;} .err{color:#ff3333;}
</style>
</head>
<body>
<div class="card">
  <h2>&#9889; HandyOS OTA</h2>
  <p>Select firmware .bin file and tap Upload.</p>
  <p style="color:#444">Device will reboot after flashing.</p>
  <br>
  <form id="frm" enctype="multipart/form-data">
    <label id="lbl" for="file">&#128194; Tap to choose .bin file</label>
    <input type="file" id="file" name="firmware" accept=".bin">
    <button id="btn" type="button" disabled onclick="upload()">UPLOAD FIRMWARE</button>
  </form>
  <div id="prog">
    <div id="bar-wrap"><div id="bar"></div></div>
    <div id="pct">0%</div>
  </div>
  <div id="msg"></div>
</div>
<script>
document.getElementById('file').onchange=function(){
  var n=this.files[0]?this.files[0].name:'';
  document.getElementById('lbl').textContent=n||'Tap to choose .bin file';
  document.getElementById('lbl').className=n?'has-file':'';
  document.getElementById('btn').disabled=!n;
};
function upload(){
  var f=document.getElementById('file').files[0];
  if(!f)return;
  var fd=new FormData();
  fd.append('firmware',f);
  var x=new XMLHttpRequest();
  x.open('POST','/update');
  document.getElementById('prog').style.display='block';
  document.getElementById('btn').disabled=true;
  x.upload.onprogress=function(e){
    if(e.lengthComputable){
      var p=Math.round(e.loaded/e.total*100);
      document.getElementById('bar').style.width=p+'%';
      document.getElementById('pct').textContent=p+'%';
    }
  };
  x.onload=function(){
    if(x.status==200){
      document.getElementById('bar').style.width='100%';
      document.getElementById('pct').textContent='100%';
      document.getElementById('msg').innerHTML='<span class="ok">&#10003; Flash complete! Rebooting...</span>';
    } else {
      document.getElementById('msg').innerHTML='<span class="err">&#10007; Upload failed ('+x.status+')</span>';
      document.getElementById('btn').disabled=false;
    }
  };
  x.onerror=function(){
    document.getElementById('msg').innerHTML='<span class="err">&#10007; Connection error</span>';
    document.getElementById('btn').disabled=false;
  };
  x.send(fd);
}
</script>
</body>
</html>
)rawhtml";

// ── OTA globals ───────────────────────────────────────────────────────────
WebServer otaServer(80);
bool      otaServerRunning = false;

// ── Helper: filled rectangle shorthand used in OTA handlers ──────────────
inline void sFill(int x, int y, int w, int h, uint16_t col) {
  tft.fillRect(x, y, w, h, col);
}

// ── Route: GET / — serve upload page ─────────────────────────────────────
void otaHandleRoot() {
  otaServer.send_P(200, "text/html", OTA_HTML);
}

// ── Route: POST /update — called after upload finishes ───────────────────
void otaHandleUpdate() {
  bool ok = !Update.hasError();
  otaServer.send(200, "text/plain", ok ? "OK" : "FAIL");
  if (ok) {
    drawHeader("OTA UPDATE");
    tft.fillRect(0, 13, 128, 137, C_BG);
    tft.setTextColor(C_OK);  tft.setTextSize(1);
    tft.setCursor(4, 24);  tft.print("Flash complete!");
    tft.setTextColor(C_YLW); tft.setCursor(4, 38); tft.print("Rebooting...");
    sFill(7, 55, 114, 8, C_OK);
    delay(1500);
    ESP.restart();
  }
}

// ── Route: upload handler — streams .bin to flash ────────────────────────
void otaHandleUpload() {
  HTTPUpload& upload = otaServer.upload();

  if (upload.status == UPLOAD_FILE_START) {
    drawHeader("OTA UPDATE");
    tft.fillRect(0, 13, 128, 137, C_BG);
    tft.setTextColor(C_TXT); tft.setTextSize(1);
    tft.setCursor(4, 20); tft.print("Receiving firmware");
    tft.setTextColor(C_ERR); tft.setCursor(4, 32); tft.print("Do not power off!");
    tft.drawRect(6, 48, 116, 10, C_HDR);
    Update.begin(UPDATE_SIZE_UNKNOWN);

  } else if (upload.status == UPLOAD_FILE_WRITE) {
    Update.write(upload.buf, upload.currentSize);
    static int pulse = 0;
    pulse = (pulse + 4) % 110;
    sFill(7, 49, 114, 8, C_BG);
    sFill(7, 49, pulse, 8, C_OK);
    sFill(0, 62, 128, 10, C_BG);
    tft.setTextColor(C_TXT); tft.setTextSize(1);
    tft.setCursor(4, 62);
    tft.print(upload.totalSize / 1024); tft.print(" KB received");

  } else if (upload.status == UPLOAD_FILE_END) {
    Update.end(true);
    sFill(7, 49, 114, 8, C_OK);
    tft.setTextColor(C_YLW); tft.setCursor(4, 76); tft.print("Flashing done!");
  }
}

// ── Start / stop helpers ─────────────────────────────────────────────────
void otaServerStart() {
  otaServer.on("/",       HTTP_GET,  otaHandleRoot);
  otaServer.on("/update", HTTP_POST, otaHandleUpdate, otaHandleUpload);
  otaServer.begin();
  otaServerRunning = true;
}
void otaServerStop() {
  otaServer.stop();
  otaServerRunning = false;
}

// ── Draw OTA screen ───────────────────────────────────────────────────────
void featureOTA_draw() {
  drawHeader("WiFi  OTA Update");
  tft.fillRect(0, 13, 128, 137, C_BG);
  drawFooter("SEL:start/stop  L:back");

  if (WiFi.status() != WL_CONNECTED) {
    bodyText(4, 22, C_ERR, "WiFi not connected!");
    bodyText(4, 38, C_TXT, "Connect via WiFi");
    bodyText(4, 54, C_TXT, "Scan first.");
    return;
  }

  if (!otaServerRunning) {
    bodyText(4, 20, C_TXT, "Web OTA Update");
    bodyText(4, 36, C_TXT, "Press SEL to start");
    bodyText(4, 52, C_TXT, "server, then open");
    bodyText(4, 68, C_TXT, "browser:");
    tft.setTextColor(C_YLW); tft.setTextSize(1);
    tft.setCursor(4, 84); tft.print("http://");
    tft.print(WiFi.localIP());
  } else {
    bodyText(4, 20, C_OK,  "Server ACTIVE");
    bodyText(4, 36, C_TXT, "Open in browser:");
    tft.setTextColor(C_YLW); tft.setTextSize(1);
    tft.setCursor(4, 52); tft.print("http://");
    tft.print(WiFi.localIP());
    bodyText(4, 70, C_TXT, "Choose .bin & upload");
    bodyText(4, 86, C_TXT, "Device auto-reboots");
    bodyText(4, 102, C_ERR, "SEL = Stop server");
  }
}

// ── Enter OTA screen ─────────────────────────────────────────────────────
void featureOTA_enter() {
  appState = ST_WIFI_OTA;
  featureOTA_draw();
}

// ── OTA tick: handle web server + button input ────────────────────────────
void featureOTA_tick() {
  if (otaServerRunning) otaServer.handleClient();

  if (pressed(BTN_SEL, bSel)) {
    if (WiFi.status() != WL_CONNECTED) return;
    if (!otaServerRunning) otaServerStart();
    else                   otaServerStop();
    featureOTA_draw();
  }
  if (pressed(BTN_LEFT, bLeft)) {
    // Go back to WiFi status; keep server running so upload can finish
    featureWifiStatus_draw();
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// ① WIFI
// ═══════════════════════════════════════════════════════════════════════════
struct WiFiNet { char ssid[33]; int rssi; };
WiFiNet nets[12]; int netCount = 0;
char savedSSID[33] = "", savedPASS[65] = "";
int wifiSel = 0;

void featureWifiScan_draw() {
  drawHeader("WiFi  Scan");
  clearBody();
  drawFooter("U/D:sel  SEL:conn  L:back");
  if (netCount == 0) { bodyText(4, 30, C_YLW, "Scanning..."); return; }
  for (int i = 0; i < min(netCount, 8); i++) {
    uint16_t c = (i == wifiSel) ? C_SEL : C_TXT;
    char buf[32];
    snprintf(buf, sizeof(buf), "%s %ddBm", nets[i].ssid, nets[i].rssi);
    bodyText(4, 16 + i * 16, c, buf);
  }
}
void featureWifiScan_enter() {
  appState = ST_WIFI_SCAN;
  netCount = WiFi.scanNetworks();
  netCount = min(netCount, 12);
  for (int i = 0; i < netCount; i++) {
    strncpy(nets[i].ssid, WiFi.SSID(i).c_str(), 32);
    nets[i].rssi = WiFi.RSSI(i);
  }
  wifiSel = 0;
  featureWifiScan_draw();
}
void featureWifiConnect_enter() {
  appState = ST_WIFI_CONNECT;
  drawHeader("WiFi  Connect");
  clearBody();
  bodyText(4, 20, C_TXT, "Connecting to:");
  bodyText(4, 35, C_YLW, nets[wifiSel].ssid);
  bodyText(4, 55, C_TXT, "Using saved cred.");
  tft.display();
  WiFi.begin(nets[wifiSel].ssid, savedPASS);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries++ < 20) {
    delay(500); tft.print(".");
  }
  clearBody();
  if (WiFi.status() == WL_CONNECTED) {
    bodyText(4, 30, C_OK,  "Connected!");
    bodyText(4, 50, C_TXT, WiFi.localIP().toString().c_str());
  } else {
    bodyText(4, 30, C_ERR, "Failed.");
    bodyText(4, 50, C_TXT, "Check credentials");
  }
  drawFooter("SEL:back");
}
void featureWifiStatus_draw() {
  appState = ST_WIFI_STATUS;
  drawHeader("WiFi  Status");
  clearBody();
  drawFooter("SEL:back");
  if (WiFi.status() == WL_CONNECTED) {
    bodyText(4, 20, C_OK,  "Connected");
    bodyText(4, 36, C_TXT, WiFi.SSID().c_str());
    bodyText(4, 52, C_TXT, WiFi.localIP().toString().c_str());
    char buf[20]; snprintf(buf, sizeof(buf), "RSSI: %d dBm", WiFi.RSSI());
    bodyText(4, 68, C_TXT, buf);
  } else {
    bodyText(4, 30, C_ERR, "Not connected");
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// ② BLUETOOTH
// ═══════════════════════════════════════════════════════════════════════════
char btLog[6][22] = {};
int  btLogHead = 0;
bool btStarted = false;
char btSendBuf[22] = "";
int  btSendPos = 0;

void featureBtMonitor_enter() {
  appState = ST_BT_MONITOR;
  if (!btStarted) { SerialBT.begin("HandyOS"); btStarted = true; }
  drawHeader("BT Monitor");
  clearBody();
  drawFooter("SEL:back");
  bodyText(4, 20, C_YLW, "Waiting for data...");
}
void featureBtMonitor_tick() {
  while (SerialBT.available()) {
    char c = SerialBT.read();
    if (c == '\n' || btSendPos >= 21) {
      btLog[btLogHead % 6][btSendPos] = '\0';
      btLogHead++;
      btSendPos = 0;
      clearBody();
      for (int i = 0; i < 6; i++) {
        int idx = (btLogHead - 6 + i + 6) % 6;
        bodyText(2, 16 + i * 20, C_TXT, btLog[idx]);
      }
    } else {
      btLog[btLogHead % 6][btSendPos++] = c;
    }
  }
}
void featureBtSend_enter() {
  appState = ST_BT_SEND;
  if (!btStarted) { SerialBT.begin("HandyOS"); btStarted = true; }
  drawHeader("BT Send");
  clearBody();
  drawFooter("U/D:char L/R:pos SEL:send");
  btSendPos = 0; btSendBuf[0] = 'A'; btSendBuf[1] = '\0';
  bodyText(4, 40, C_YLW, btSendBuf);
}
void featureBtSend_tick() {
  if (pressed(BTN_UP,    bUp))    { btSendBuf[btSendPos]++; }
  if (pressed(BTN_DOWN,  bDown))  { btSendBuf[btSendPos]--; }
  if (pressed(BTN_RIGHT, bRight)) { btSendPos = min(btSendPos + 1, 20); btSendBuf[btSendPos] = 'A'; btSendBuf[btSendPos+1]='\0'; }
  if (pressed(BTN_LEFT,  bLeft))  { if (btSendPos > 0) { btSendBuf[btSendPos] = '\0'; btSendPos--; } }
  if (pressed(BTN_SEL,   bSel))   {
    SerialBT.println(btSendBuf);
    clearBody();
    bodyText(4, 40, C_OK, "Sent!"); delay(800);
    featureBtSend_enter(); return;
  }
  clearBody();
  bodyText(4, 40, C_YLW, btSendBuf);
  // cursor underline
  tft.drawLine(4 + btSendPos * 6, 49, 4 + btSendPos * 6 + 5, 49, C_SEL);
}

// ═══════════════════════════════════════════════════════════════════════════
// ③ SOCIAL
// ═══════════════════════════════════════════════════════════════════════════
// Simple QR placeholder (draws a visual mock)
void featureSocialQR_enter() {
  appState = ST_SOCIAL_QR;
  drawHeader("Social  QR Code");
  clearBody();
  drawFooter("SEL:back");
  // Draw a fake QR pattern as a visual placeholder
  tft.fillRect(24, 20, 80, 80, C_TXT);
  tft.fillRect(26, 22, 76, 76, C_BG);
  // Corner squares
  for (int s = 0; s < 3; s++) {
    tft.fillRect(28 + s, 24 + s, 20 - s*2, 20 - s*2, s%2 ? C_BG : C_TXT);
    tft.fillRect(74 + s, 24 + s, 20 - s*2, 20 - s*2, s%2 ? C_BG : C_TXT);
    tft.fillRect(28 + s, 70 + s, 20 - s*2, 20 - s*2, s%2 ? C_BG : C_TXT);
  }
  // Inner pixels
  for (int r = 0; r < 4; r++)
    for (int c = 0; c < 4; c++)
      if ((r + c) % 2 == 0)
        tft.fillRect(55 + c*6, 40 + r*6, 5, 5, C_TXT);
  bodyText(6, 106, C_YLW, "t.me/HandyOS");
}

void featureSocialMsg_enter() {
  appState = ST_SOCIAL_MSG;
  drawHeader("Social  Message");
  clearBody();
  drawFooter("SEL:back");
  bodyText(4, 20, C_SEL,  "Status Message:");
  prefs.begin("social", true);
  String msg = prefs.getString("msg", "Hello World!");
  prefs.end();
  bodyText(4, 38, C_TXT, msg.c_str());
  bodyText(4, 70, C_YLW, "Edit via BT serial:");
  bodyText(4, 86, C_TXT, "Send: SETMSG:text");
}

// ═══════════════════════════════════════════════════════════════════════════
// ④ TOOLS — Clock & Calculator
// ═══════════════════════════════════════════════════════════════════════════
void featureClockDraw() {
  time_t now; struct tm ti;
  time(&now); localtime_r(&now, &ti);
  char tBuf[12], dBuf[20];
  snprintf(tBuf, sizeof(tBuf), "%02d:%02d:%02d", ti.tm_hour, ti.tm_min, ti.tm_sec);
  snprintf(dBuf, sizeof(dBuf), "%04d-%02d-%02d", ti.tm_year+1900, ti.tm_mon+1, ti.tm_mday);
  tft.fillRect(0, 25, 128, 80, C_BG);
  bodyText(8, 40, C_YLW, tBuf, 2);
  bodyText(14, 75, C_TXT, dBuf);
  const char* days[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
  bodyText(46, 92, C_SEL, days[ti.tm_wday]);
}
void featureClock_enter() {
  appState = ST_TOOLS_CLOCK;
  drawHeader("Tools  World Clock");
  clearBody();
  drawFooter("SEL:back");
  if (WiFi.status() == WL_CONNECTED) {
    configTime(0, 0, "pool.ntp.org");
    bodyText(4, 110, C_OK, "NTP synced");
  } else {
    bodyText(4, 110, C_ERR, "No WiFi - local time");
  }
  featureClockDraw();
}

// Simple integer calculator
int calcA = 0, calcB = 0, calcStep = 0;
char calcOp = '+';
void featureCalc_enter() {
  appState = ST_TOOLS_CALC;
  calcA = 0; calcB = 0; calcStep = 0; calcOp = '+';
  drawHeader("Tools  Calculator");
  clearBody();
  drawFooter("U/D:num  L/R:op  SEL:=");
  bodyText(4, 30, C_TXT, "Enter A:");
  bodyText(4, 45, C_YLW, "0");
}
void featureCalc_tick() {
  bool redraw = false;
  if (calcStep == 0) {
    if (pressed(BTN_UP,    bUp))    { calcA++; redraw = true; }
    if (pressed(BTN_DOWN,  bDown))  { calcA--; redraw = true; }
    if (pressed(BTN_SEL,   bSel))   { calcStep = 1; redraw = true; }
    if (redraw) {
      clearBody();
      char buf[12]; itoa(calcA, buf, 10);
      bodyText(4, 30, C_TXT, "Enter A:");
      bodyText(4, 45, C_YLW, buf);
    }
  } else if (calcStep == 1) {
    if (pressed(BTN_LEFT,  bLeft))  { calcOp = (calcOp == '+') ? '-' : (calcOp == '-') ? '*' : (calcOp == '*') ? '/' : '+'; redraw=true; }
    if (pressed(BTN_RIGHT, bRight)) { calcOp = (calcOp == '+') ? '/' : (calcOp == '/') ? '*' : (calcOp == '*') ? '-' : '+'; redraw=true; }
    if (pressed(BTN_SEL,   bSel))   { calcStep = 2; redraw = true; }
    if (redraw) {
      clearBody();
      char buf[30]; snprintf(buf,sizeof(buf),"%d  %c  ?", calcA, calcOp);
      bodyText(4, 30, C_TXT, "Choose op:");
      bodyText(4, 45, C_YLW, buf);
    }
  } else if (calcStep == 2) {
    if (pressed(BTN_UP,    bUp))    { calcB++; redraw = true; }
    if (pressed(BTN_DOWN,  bDown))  { calcB--; redraw = true; }
    if (pressed(BTN_SEL,   bSel))   { calcStep = 3; redraw = true; }
    if (redraw) {
      clearBody();
      char buf[12]; itoa(calcB, buf, 10);
      bodyText(4, 30, C_TXT, "Enter B:");
      bodyText(4, 45, C_YLW, buf);
    }
  } else if (calcStep == 3 && redraw == false) {
    long res = 0;
    bool err = false;
    switch (calcOp) {
      case '+': res = (long)calcA + calcB; break;
      case '-': res = (long)calcA - calcB; break;
      case '*': res = (long)calcA * calcB; break;
      case '/': if (calcB == 0) err = true; else res = calcA / calcB; break;
    }
    clearBody();
    char expr[40];
    snprintf(expr, sizeof(expr), "%d %c %d =", calcA, calcOp, calcB);
    bodyText(4, 30, C_TXT, expr);
    if (err) bodyText(4, 50, C_ERR, "Div by zero!");
    else { char rb[16]; ltoa(res, rb, 10); bodyText(4, 50, C_OK, rb, 2); }
    drawFooter("SEL:new calc");
    calcStep = 4;
  } else if (calcStep == 4) {
    if (pressed(BTN_SEL, bSel)) featureCalc_enter();
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// ⑤ ISLAM — Prayer times (static, Kuala Lumpur) & Qibla direction
// ═══════════════════════════════════════════════════════════════════════════
void featurePray_enter() {
  appState = ST_ISLAM_PRAY;
  drawHeader("Islam  Prayer Times");
  clearBody();
  drawFooter("SEL:back");
  // Hardcoded KL times (update via WiFi API in a fuller version)
  const char* prayers[] = {"Fajr","Dhuhr","Asr","Maghrib","Isha"};
  const char* times[]   = {"05:48","13:07","16:29","19:26","20:41"};
  bodyText(4, 18, C_YLW, "Kuala Lumpur (static)");
  for (int i = 0; i < 5; i++) {
    bodyText(4,  34 + i*20, C_SEL, prayers[i]);
    bodyText(72, 34 + i*20, C_TXT, times[i]);
  }
}

void featureQibla_enter() {
  appState = ST_ISLAM_QBL;
  drawHeader("Islam  Qibla");
  clearBody();
  drawFooter("SEL:back");
  // Qibla from KL ≈ 292° (NW)
  int cx = 64, cy = 75, r = 40;
  tft.drawCircle(cx, cy, r, C_TXT);
  // Cardinal points
  bodyText(cx-3, cy-r-8, C_TXT, "N");
  bodyText(cx-3, cy+r+2, C_TXT, "S");
  bodyText(cx+r+3,cy-3,  C_TXT, "E");
  bodyText(cx-r-9,cy-3,  C_TXT, "W");
  // Qibla arrow at ~292°
  float ang = radians(292 - 90);
  int ax = cx + (int)(r * cos(ang));
  int ay = cy + (int)(r * sin(ang));
  tft.drawLine(cx, cy, ax, ay, C_YLW);
  tft.fillCircle(ax, ay, 3, C_YLW);
  bodyText(cx-16, cy+r+14, C_SEL, "Qibla: 292deg");
}

// ═══════════════════════════════════════════════════════════════════════════
// ⑥ FILE — Logger & Viewer (NVS-based)
// ═══════════════════════════════════════════════════════════════════════════
void featureFileLog_enter() {
  appState = ST_FILE_LOG;
  drawHeader("File  Data Logger");
  clearBody();
  drawFooter("SEL:log now  L:back");
  prefs.begin("logger", false);
  int cnt = prefs.getInt("count", 0);
  char buf[24];
  snprintf(buf, sizeof(buf), "Logged: %d entries", cnt);
  bodyText(4, 30, C_TXT, buf);
  bodyText(4, 50, C_YLW, "Press SEL to log");
  bodyText(4, 65, C_TXT, "Uptime + free heap");
  prefs.end();
}
void featureFileLog_tick() {
  if (pressed(BTN_SEL, bSel)) {
    prefs.begin("logger", false);
    int cnt = prefs.getInt("count", 0) + 1;
    prefs.putInt("count", cnt);
    char key[8]; snprintf(key, sizeof(key), "e%d", cnt % 10);
    char val[32];
    snprintf(val, sizeof(val), "up=%lu h=%u", millis()/1000, (unsigned)ESP.getFreeHeap());
    prefs.putString(key, val);
    prefs.end();
    clearBody();
    char buf[24]; snprintf(buf, sizeof(buf), "Logged entry #%d", cnt);
    bodyText(4, 40, C_OK, buf);
    bodyText(4, 60, C_TXT, val);
    delay(1000);
    featureFileLog_enter();
  }
}

void featureFileView_enter() {
  appState = ST_FILE_VIEW;
  drawHeader("File  Log Viewer");
  clearBody();
  drawFooter("SEL:back");
  prefs.begin("logger", true);
  int cnt = prefs.getInt("count", 0);
  char buf[24]; snprintf(buf, sizeof(buf), "Total: %d entries", cnt);
  bodyText(4, 18, C_YLW, buf);
  for (int i = 0; i < min(cnt, 5); i++) {
    char key[8]; snprintf(key, sizeof(key), "e%d", (cnt - i - 1) % 10 + 1);
    String val = prefs.getString(key, "(empty)");
    bodyText(2, 34 + i*20, C_TXT, val.c_str());
  }
  prefs.end();
}

// ═══════════════════════════════════════════════════════════════════════════
// ⑦ GAMES — Snake & Reaction Timer
// ═══════════════════════════════════════════════════════════════════════════

// ── Snake ──
#define SN_COLS 16
#define SN_ROWS 14
#define SN_SZ   7
#define SN_OX   0
#define SN_OY   15
struct SnPt { int8_t x, y; };
SnPt  snBody[SN_COLS * SN_ROWS];
int   snLen, snDir; // 0=R 1=D 2=L 3=U
SnPt  snFood;
bool  snAlive;
unsigned long snLast = 0;

void snPlaceFood() {
  snFood.x = random(SN_COLS); snFood.y = random(SN_ROWS);
}
void snStart() {
  snLen = 3; snDir = 0; snAlive = true;
  for (int i = 0; i < snLen; i++) { snBody[i].x = 4-i; snBody[i].y = 2; }
  snPlaceFood();
}
void snDraw() {
  tft.fillRect(SN_OX, SN_OY, SN_COLS*SN_SZ, SN_ROWS*SN_SZ, C_BG);
  tft.fillRect(snFood.x*SN_SZ+SN_OX, snFood.y*SN_SZ+SN_OY, SN_SZ-1, SN_SZ-1, C_YLW);
  for (int i = 0; i < snLen; i++) {
    uint16_t c = (i==0) ? C_SEL : C_OK;
    tft.fillRect(snBody[i].x*SN_SZ+SN_OX, snBody[i].y*SN_SZ+SN_OY, SN_SZ-1, SN_SZ-1, c);
  }
}
void featureSnake_enter() {
  appState = ST_GAME_SNAKE;
  drawHeader("Games  Snake");
  clearBody();
  drawFooter("UDLR:dir SEL:quit");
  snStart(); snDraw();
}
void featureSnake_tick() {
  if (!snAlive) {
    if (pressed(BTN_SEL, bSel)) featureSnake_enter();
    return;
  }
  if (pressed(BTN_UP,    bUp))    if (snDir!=1) snDir=3;
  if (pressed(BTN_DOWN,  bDown))  if (snDir!=3) snDir=1;
  if (pressed(BTN_LEFT,  bLeft))  if (snDir!=0) snDir=2;
  if (pressed(BTN_RIGHT, bRight)) if (snDir!=2) snDir=0;
  if (pressed(BTN_SEL,   bSel))   { appState = ST_MENU; drawMainMenu(); return; }

  if (millis() - snLast < 200) return;
  snLast = millis();

  SnPt nh = snBody[0];
  const int8_t dx[] = {1,0,-1,0}, dy[] = {0,1,0,-1};
  nh.x += dx[snDir]; nh.y += dy[snDir];
  if (nh.x < 0 || nh.x >= SN_COLS || nh.y < 0 || nh.y >= SN_ROWS) { snAlive=false; goto dead; }
  for (int i=1;i<snLen;i++) if (snBody[i].x==nh.x && snBody[i].y==nh.y) { snAlive=false; goto dead; }
  memmove(&snBody[1], &snBody[0], (snLen-1)*sizeof(SnPt));
  snBody[0] = nh;
  if (nh.x==snFood.x && nh.y==snFood.y) { if (snLen<SN_COLS*SN_ROWS) snLen++; snPlaceFood(); }
  snDraw();
  return;
dead:
  clearBody();
  bodyText(20, 50, C_ERR, "GAME OVER", 2);
  char sc[16]; snprintf(sc,sizeof(sc),"Score: %d", snLen-3);
  bodyText(30, 80, C_YLW, sc);
  drawFooter("SEL:restart");
}

// ── Reaction Timer ──
unsigned long reactTarget = 0;
int reactState = 0; // 0=wait 1=ready 2=done
unsigned long reactStart = 0;
void featureReact_enter() {
  appState = ST_GAME_REACT;
  reactState = 0;
  drawHeader("Games  Reflex Test");
  clearBody();
  drawFooter("SEL:start");
  bodyText(10, 50, C_TXT, "Press SEL when", 1);
  bodyText(10, 65, C_TXT, "screen goes GREEN", 1);
}
void featureReact_tick() {
  if (reactState == 0) {
    if (pressed(BTN_SEL, bSel)) {
      reactState = 1;
      reactTarget = millis() + random(2000, 5000);
      clearBody();
      tft.fillRect(0,13,128,137, C_ERR);
      bodyText(30,75,C_TXT,"WAIT...",2);
    }
  } else if (reactState == 1) {
    if (millis() >= reactTarget) {
      reactState = 2;
      reactStart = millis();
      tft.fillRect(0,13,128,137, C_OK);
      bodyText(20,75,C_TXT,"PRESS!",2);
    }
    if (pressed(BTN_SEL, bSel)) {
      reactState = 0;
      clearBody();
      bodyText(20,60,C_ERR,"Too early!",1);
      drawFooter("SEL:retry");
    }
  } else {
    if (pressed(BTN_SEL, bSel)) {
      unsigned long rt = millis() - reactStart;
      clearBody();
      char buf[24]; snprintf(buf,sizeof(buf),"%lu ms", rt);
      bodyText(20,50,C_TXT,"Your time:",1);
      bodyText(30,70,C_YLW,buf,2);
      const char* grade = rt<200?"Elite!":rt<350?"Great!":rt<500?"Good":"Slow";
      bodyText(40,105,C_SEL,grade);
      drawFooter("SEL:retry");
      reactState = 0;
    }
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// ⑧ SETTINGS — Brightness & WiFi Creds
// ═══════════════════════════════════════════════════════════════════════════
int brightLevel = 255; // PWM value, pin 33
void featureBright_enter() {
  appState = ST_SET_BRIGHT;
  drawHeader("Settings  Brightness");
  clearBody();
  drawFooter("U/D:adjust SEL:save");
  prefs.begin("settings", true);
  brightLevel = prefs.getInt("bright", 255);
  prefs.end();
  ledcSetup(0, 5000, 8);
  ledcAttachPin(33, 0);
  ledcWrite(0, brightLevel);
  char buf[8]; itoa(brightLevel, buf, 10);
  bodyText(4, 40, C_TXT, "Level (0-255):");
  bodyText(4, 56, C_YLW, buf, 2);
  tft.drawRect(4, 90, 120, 12, C_TXT);
  tft.fillRect(5, 91, (int)(brightLevel * 118 / 255), 10, C_SEL);
}
void featureBright_tick() {
  bool ch = false;
  if (pressed(BTN_UP,   bUp))   { brightLevel = min(255, brightLevel+5); ch=true; }
  if (pressed(BTN_DOWN, bDown)) { brightLevel = max(0,   brightLevel-5); ch=true; }
  if (ch) {
    ledcWrite(0, brightLevel);
    clearBody();
    char buf[8]; itoa(brightLevel, buf, 10);
    bodyText(4, 40, C_TXT, "Level (0-255):");
    bodyText(4, 56, C_YLW, buf, 2);
    tft.drawRect(4, 90, 120, 12, C_TXT);
    tft.fillRect(5, 91, (int)(brightLevel * 118 / 255), 10, C_SEL);
  }
  if (pressed(BTN_SEL, bSel)) {
    prefs.begin("settings", false);
    prefs.putInt("bright", brightLevel);
    prefs.end();
    clearBody();
    bodyText(20,60,C_OK,"Saved!",2);
    delay(800); featureBright_enter();
  }
}

// WiFi Creds (simple BT-based entry)
void featureWifiCred_enter() {
  appState = ST_SET_WIFI_CRED;
  drawHeader("Settings  WiFi Creds");
  clearBody();
  drawFooter("SEL:back");
  bodyText(4, 20, C_YLW, "Send via BT Serial:");
  bodyText(4, 36, C_TXT, "SSID:yourSSID");
  bodyText(4, 52, C_TXT, "PASS:yourPass");
  prefs.begin("wifi", true);
  String s = prefs.getString("ssid","(none)");
  prefs.end();
  bodyText(4, 75, C_SEL, "Saved SSID:");
  bodyText(4, 90, C_TXT, s.c_str());
}
void featureWifiCred_tick() {
  while (SerialBT.available()) {
    String line = SerialBT.readStringUntil('\n');
    if (line.startsWith("SSID:")) {
      String s = line.substring(5); s.trim();
      s.toCharArray(savedSSID, 33);
      prefs.begin("wifi",false); prefs.putString("ssid",s); prefs.end();
    }
    if (line.startsWith("PASS:")) {
      String p = line.substring(5); p.trim();
      p.toCharArray(savedPASS, 65);
      prefs.begin("wifi",false); prefs.putString("pass",p); prefs.end();
    }
  }
  if (pressed(BTN_SEL, bSel)) { appState = ST_MENU; drawMainMenu(); }
}

// ═══════════════════════════════════════════════════════════════════════════
// ⑨ ABOUT
// ═══════════════════════════════════════════════════════════════════════════
void featureAboutInfo_enter() {
  appState = ST_ABOUT_INFO;
  drawHeader("About  HandyOS");
  clearBody();
  drawFooter("SEL:back");
  bodyText(4, 20, C_YLW, "HandyOS v1.1", 1);
  bodyText(4, 36, C_TXT, "ESP32 + ST7735");
  bodyText(4, 52, C_TXT, "9 menus + OTA");
  bodyText(4, 68, C_TXT, "github.com/HandyOS");
  bodyText(4, 90, C_SEL, "Build: " __DATE__);
}
void featureAboutDiag_enter() {
  appState = ST_ABOUT_DIAG;
  drawHeader("About  Diagnostics");
  clearBody();
  drawFooter("SEL:back");
  char buf[28];
  snprintf(buf,sizeof(buf),"Heap: %u B", (unsigned)ESP.getFreeHeap());
  bodyText(4, 22, C_TXT, buf);
  snprintf(buf,sizeof(buf),"Flash: %u KB", (unsigned)(ESP.getFlashChipSize()/1024));
  bodyText(4, 38, C_TXT, buf);
  snprintf(buf,sizeof(buf),"CPU: %u MHz", (unsigned)ESP.getCpuFreqMHz());
  bodyText(4, 54, C_TXT, buf);
  snprintf(buf,sizeof(buf),"Uptime: %lus", millis()/1000);
  bodyText(4, 70, C_TXT, buf);
  snprintf(buf,sizeof(buf),"WiFi: %s", WiFi.status()==WL_CONNECTED?"ON":"OFF");
  bodyText(4, 86, C_TXT, buf);
  snprintf(buf,sizeof(buf),"BT: %s", btStarted?"ON":"OFF");
  bodyText(4,102, C_TXT, buf);
}

// ═══════════════════════════════════════════════════════════════════════════
// MENU NAVIGATION
// ═══════════════════════════════════════════════════════════════════════════
void menuNavigate() {
  int prev = menuSel;
  if (pressed(BTN_UP,    bUp))    menuSel = (menuSel < 3)     ? menuSel     : menuSel - 3;
  if (pressed(BTN_DOWN,  bDown))  menuSel = (menuSel > 5)     ? menuSel     : menuSel + 3;
  if (pressed(BTN_LEFT,  bLeft))  menuSel = (menuSel % 3 == 0)? menuSel     : menuSel - 1;
  if (pressed(BTN_RIGHT, bRight)) menuSel = (menuSel % 3 == 2)? menuSel     : menuSel + 1;
  if (prev != menuSel) {
    drawMenuCell(prev, false);
    drawMenuCell(menuSel, true);
    drawMenuLabel(menuSel);
  }
  if (pressed(BTN_SEL, bSel)) {
    switch (menuSel) {
      case 0: featureWifiScan_enter(); break;
      case 1: featureBtMonitor_enter(); break;
      case 2: featureSocialQR_enter(); break;
      case 3: featureClock_enter(); break;
      case 4: featurePray_enter(); break;
      case 5: featureFileLog_enter(); break;
      case 6: featureSnake_enter(); break;
      case 7: featureBright_enter(); break;
      case 8: featureAboutInfo_enter(); break;
    }
  }
}

// Sub-feature cycling (L/R within a menu)
void subNav() {
  // Within WiFi: SEL=scan→connect, L=status
  if (appState == ST_WIFI_SCAN) {
    if (pressed(BTN_UP,   bUp))    { wifiSel = max(0, wifiSel-1); featureWifiScan_draw(); }
    if (pressed(BTN_DOWN, bDown))  { wifiSel = min(netCount-1, wifiSel+1); featureWifiScan_draw(); }
    if (pressed(BTN_LEFT, bLeft))  { featureWifiStatus_draw(); return; }
    if (pressed(BTN_RIGHT,bRight)) { featureOTA_enter(); return; }
    if (pressed(BTN_SEL,  bSel))   { featureWifiConnect_enter(); return; }
  }
  if (appState == ST_WIFI_CONNECT || appState == ST_WIFI_STATUS) {
    if (pressed(BTN_RIGHT, bRight)) { featureOTA_enter(); return; }
    if (pressed(BTN_SEL, bSel)) { appState = ST_MENU; drawMainMenu(); }
  }
  if (appState == ST_WIFI_OTA) {
    featureOTA_tick();
  }
  // BT: L/R to switch sub-features
  if (appState == ST_BT_MONITOR) {
    featureBtMonitor_tick();
    if (pressed(BTN_RIGHT, bRight)) { featureBtSend_enter(); return; }
    if (pressed(BTN_SEL,   bSel))   { appState = ST_MENU; drawMainMenu(); }
  }
  if (appState == ST_BT_SEND) {
    featureBtSend_tick();
  }
  // Social: L/R to switch
  if (appState == ST_SOCIAL_QR) {
    if (pressed(BTN_RIGHT, bRight)) { featureSocialMsg_enter(); return; }
    if (pressed(BTN_SEL,   bSel))   { appState = ST_MENU; drawMainMenu(); }
  }
  if (appState == ST_SOCIAL_MSG) {
    if (pressed(BTN_LEFT, bLeft)) { featureSocialQR_enter(); return; }
    if (pressed(BTN_SEL,  bSel))  { appState = ST_MENU; drawMainMenu(); }
  }
  // Tools: L/R switch
  if (appState == ST_TOOLS_CLOCK) {
    featureClockDraw();
    if (pressed(BTN_RIGHT, bRight)) { featureCalc_enter(); return; }
    if (pressed(BTN_SEL,   bSel))   { appState = ST_MENU; drawMainMenu(); }
    delay(500);
  }
  if (appState == ST_TOOLS_CALC) {
    featureCalc_tick();
    if (pressed(BTN_LEFT, bLeft)) { featureClock_enter(); return; }
  }
  // Islam
  if (appState == ST_ISLAM_PRAY) {
    if (pressed(BTN_RIGHT, bRight)) { featureQibla_enter(); return; }
    if (pressed(BTN_SEL,   bSel))   { appState = ST_MENU; drawMainMenu(); }
  }
  if (appState == ST_ISLAM_QBL) {
    if (pressed(BTN_LEFT, bLeft)) { featurePray_enter(); return; }
    if (pressed(BTN_SEL,  bSel))  { appState = ST_MENU; drawMainMenu(); }
  }
  // File
  if (appState == ST_FILE_LOG) {
    featureFileLog_tick();
    if (pressed(BTN_RIGHT, bRight)) { featureFileView_enter(); return; }
    if (pressed(BTN_LEFT,  bLeft))  { appState = ST_MENU; drawMainMenu(); }
  }
  if (appState == ST_FILE_VIEW) {
    if (pressed(BTN_LEFT, bLeft)) { featureFileLog_enter(); return; }
    if (pressed(BTN_SEL,  bSel))  { appState = ST_MENU; drawMainMenu(); }
  }
  // Games
  if (appState == ST_GAME_SNAKE)  featureSnake_tick();
  if (appState == ST_GAME_REACT) {
    featureReact_tick();
    if (appState == ST_GAME_REACT && pressed(BTN_LEFT, bLeft)) { featureSnake_enter(); return; }
  }
  // Settings
  if (appState == ST_SET_BRIGHT) {
    featureBright_tick();
    if (pressed(BTN_RIGHT, bRight)) { featureWifiCred_enter(); return; }
  }
  if (appState == ST_SET_WIFI_CRED) {
    featureWifiCred_tick();
    if (pressed(BTN_LEFT, bLeft)) { featureBright_enter(); return; }
  }
  // About
  if (appState == ST_ABOUT_INFO) {
    if (pressed(BTN_RIGHT, bRight)) { featureAboutDiag_enter(); return; }
    if (pressed(BTN_SEL,   bSel))   { appState = ST_MENU; drawMainMenu(); }
  }
  if (appState == ST_ABOUT_DIAG) {
    if (pressed(BTN_LEFT, bLeft)) { featureAboutInfo_enter(); return; }
    if (pressed(BTN_SEL,  bSel))  { appState = ST_MENU; drawMainMenu(); }
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// SETUP & LOOP
// ═══════════════════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  initButtons();

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(0);
  tft.fillScreen(C_BG);

  // Load saved WiFi creds
  prefs.begin("wifi", true);
  prefs.getString("ssid","").toCharArray(savedSSID, 33);
  prefs.getString("pass","").toCharArray(savedPASS, 65);
  prefs.end();

  // Boot animation
  drawBootScreen();
  tft.setTextColor(C_SEL); tft.setTextSize(1);
  tft.setCursor(28, 97);  tft.print("HandyOS  v1.1");
  tft.setCursor(20, 110); tft.setTextColor(C_TXT); tft.print("ESP32 + ST7735");
  delay(2500);

  drawMainMenu();
  appState = ST_MENU;
}

void loop() {
  if (appState == ST_MENU) {
    menuNavigate();
  } else {
    subNav();
  }
  delay(10);
}
