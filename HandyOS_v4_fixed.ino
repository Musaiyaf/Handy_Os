// ═══════════════════════════════════════════════════════════════════════════
//  HandyOS v4.1  –  ESP32 + ST7735 1.44" 128x128 TFT
//  Fixed: 3×3 grid alignment, correct 9-menu layout, WiFi separated from Social
// ═══════════════════════════════════════════════════════════════════════════

// ── Libraries ────────────────────────────────────────────────────────────────
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <SPIFFS.h>
#include <Preferences.h>

// ── Pin Definitions ──────────────────────────────────────────────────────────
#define TFT_CS   5
#define TFT_RST  4
#define TFT_DC   2
#define TFT_MOSI 23
#define TFT_CLK  18
#define TFT_BL   15

#define BTN_UP   13
#define BTN_DWN  12
#define BTN_LFT  14
#define BTN_RHT  27
#define BTN_MID  33
#define BTN_SET  32
#define BTN_RST  25

// ── Display ───────────────────────────────────────────────────────────────────
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST);

#define SCR_W 128
#define SCR_H 128

// ── Colour Palette ────────────────────────────────────────────────────────────
#define C_BG   0x0000
#define C_TXT  0xFFFF
#define C_SEL  0x07FF
#define C_HDR  0x18C3
#define C_DIM  0x4208
#define C_MID  0x0841
#define C_ACC  0xF800
#define C_GRN  0x07E0
#define C_YLW  0xFFE0
#define C_ORG  0xFD20

// ── Debounce ──────────────────────────────────────────────────────────────────
#define DB 200
unsigned long lastBtn = 0;

// ── Menu indices ──────────────────────────────────────────────────────────────
// 0=WiFi  1=Bluetooth  2=Social
// 3=Tools 4=Islam      5=File
// 6=Games 7=Settings   8=About

// ── App State Machine ─────────────────────────────────────────────────────────
enum AppState {
  ST_BOOT,
  ST_MENU,
  ST_WIFI_MAIN,
  ST_WIFI_SCAN,
  ST_WIFI_PASS,
  ST_WIFI_OTA,
  ST_FILES,
  ST_FILE_ACTION,
  ST_FILE_TEXT,
  ST_FILE_HEX,
  ST_COMING_SOON
};
AppState curState = ST_BOOT;

// ── Global Variables ──────────────────────────────────────────────────────────
char  wifiSSID[33] = "";
char  wfPass[65]   = "";
int   wfPassLen    = 0;
bool  wifiOK       = false;

int   menuSel      = 0;
int   wifiMenuSel  = 0;

#define MAX_NET 8
int   netCount = 0;
int   netSel   = 0;

#define MAX_FILES     40
#define FILES_VISIBLE  7
struct FileEntry { char name[64]; size_t size; };
FileEntry fileList[MAX_FILES];
int fileCount  = 0;
int fileScroll = 0;
int fileSel    = 0;

int  fileActionSel     = 0;
bool fileDeleteConfirm = false;

#define TV_COLS 21
#define TV_ROWS 13
char tvBuf[TV_COLS * TV_ROWS + 4];
long tvOffset   = 0;
bool tvEnd      = false;
char tvFileName[64];

#define HX_ROWS 6
#define HX_COLS 8
long hxOffset   = 0;
long hxFileSize = 0;
char hxFileName[64];

// Keyboard state
int kbRow = 0, kbCol = 0, kbPage = 0;

// Keyboard maps
const char KBL[3][11] = {"qwertyuiop","asdfghjkl ","zxcvbnm   "};
const char KBU[3][11] = {"QWERTYUIOP","ASDFGHJKL ","ZXCVBNM   "};
const char KBN[3][11] = {"1234567890","!@#$%^&*()","_-+=[]{}  "};
const char KBS[3][11] = {";:'\",.<>? ","|\\~`      ","          "};

const char* wifiMenuLabels[]   = {"Connect","OTA Update","Disconnect","Back"};
const char* fileActionLabels[] = {"View text","Hex dump","Rename","Delete","Info","Cancel"};

// OTA
WebServer otaServer(80);
bool otaServerRunning = false;

// ════════════════════════════════════════════════════════════════════════════
//  BITMAPS – Boot screen
// ════════════════════════════════════════════════════════════════════════════

static const unsigned char PROGMEM image_Background_bits[] = {
  0x7f,0x80,0x00,0x00,0x00,0x00,0x00,0x07,0xff,0xff,0xff,0xf0,0x00,0x00,0x00,0x00,
  0x80,0xc0,0x00,0x00,0x00,0x00,0x00,0x0c,0x00,0x00,0x00,0x18,0x00,0x00,0x00,0x00,
  0xbe,0x60,0x00,0x00,0x00,0x00,0x00,0x18,0xff,0xed,0xaa,0x8c,0x00,0x00,0x00,0x00,
  0x81,0x3f,0xff,0xff,0xff,0xff,0xff,0xf1,0x00,0x00,0x00,0x47,0xff,0xff,0xff,0xfe,
  0xbc,0x80,0x00,0x00,0x00,0x00,0x00,0x02,0x6d,0x57,0xff,0x20,0x00,0x00,0x00,0x01,
  0x82,0x7f,0xff,0xff,0x55,0x7f,0xff,0xfc,0x80,0x00,0x00,0x9f,0xff,0xff,0xff,0xd5,
  0xf9,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x1f,0xff,0xfe,0x40,0x00,0x00,0x00,0x01,
  0x7c,0xff,0xff,0xff,0xff,0xaa,0xbf,0xfe,0x3f,0xff,0xff,0x36,0xff,0xff,0xff,0xad,
  0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x60,0x00,0x01,0x80,0x00,0x00,0x00,0x01,
  0x03,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xc0,0x00,0x00,0xff,0xff,0xff,0xff,0xff,
  0x01,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x80,0x00,0x00,0x7f,0xff,0xff,0xff,0xfe
};

static const unsigned char PROGMEM image_cloud_bits[] = {
  0x00,0x00,0x00,0x07,0xc0,0x00,0x08,0x20,0x00,0x10,0x10,0x00,
  0x30,0x08,0x00,0x40,0x0e,0x00,0x80,0x01,0x00,0x80,0x00,0x80,
  0x40,0x00,0x80,0x3f,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

static const unsigned char PROGMEM image_display_brightness_bits[] = {
  0x01,0x00,0x21,0x08,0x10,0x10,0x03,0x80,0x8c,0x62,0x48,0x24,
  0x10,0x10,0x10,0x10,0x10,0x10,0x48,0x24,0x8c,0x62,0x03,0x80,
  0x10,0x10,0x21,0x08,0x01,0x00,0x00,0x00
};

static const unsigned char PROGMEM image_DolphinWait_bits[] = {
  0x00,0x00,0x00,0x3f,0xf0,0x00,0x00,0x00,0x00,0x00,0x01,0xc0,0x0e,0x00,0x00,0x00,
  0x00,0x00,0x06,0x00,0x01,0x80,0x00,0x00,0x00,0x00,0x18,0x00,0x00,0x60,0x00,0x00,
  0x00,0x00,0x60,0x00,0x00,0x18,0x00,0x00,0x00,0x00,0x80,0x00,0x00,0x04,0x00,0x00,
  0x00,0x01,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x01,0x00,0x00,
  0x00,0x04,0x00,0x00,0x00,0x00,0x80,0x00,0x00,0x08,0x00,0x00,0x00,0x00,0x80,0x00,
  0x00,0x08,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x40,0x00,
  0x00,0x10,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x20,0x03,0xc0,0x00,0x00,0x60,0x00,
  0x00,0x20,0x0d,0xf0,0x00,0x00,0xe0,0x00,0x00,0x40,0x13,0x98,0x00,0x01,0x30,0x00,
  0x00,0x40,0x27,0x0c,0x00,0x01,0x30,0x00,0x00,0x40,0x27,0x0c,0x00,0x01,0x70,0x00,
  0x00,0x80,0x4f,0x9e,0x00,0x03,0xfc,0x00,0x00,0x80,0x4f,0xfe,0x00,0x0c,0x02,0x00,
  0x00,0x80,0x4f,0xfe,0x00,0x30,0x01,0x00,0x00,0x80,0x47,0xfa,0x00,0xc0,0x00,0x80,
  0x00,0x80,0x47,0xfa,0x03,0x00,0x00,0x80,0x00,0x80,0x21,0xf4,0x0c,0x00,0x00,0x80,
  0x00,0x80,0x20,0x04,0x30,0x00,0x3c,0x80,0x01,0x00,0x17,0xe8,0x00,0x00,0xc2,0x80,
  0x01,0x00,0x18,0x10,0x00,0x03,0x01,0x80,0x01,0x00,0x10,0x08,0x00,0x0c,0x01,0x00,
  0x01,0x00,0x20,0x00,0x00,0x30,0x01,0x00,0x03,0x00,0x20,0x00,0x00,0xc0,0x01,0x00,
  0x07,0x00,0x00,0x00,0x03,0x00,0x02,0x00,0x0d,0x00,0x01,0x00,0x0c,0x00,0x04,0x00,
  0x0b,0x00,0x01,0x00,0x30,0x00,0x04,0x00,0x15,0x00,0x00,0x80,0xc0,0x00,0x08,0x00,
  0x2b,0x00,0x00,0x7f,0x00,0x00,0x10,0x00,0x35,0xc0,0x00,0x00,0x00,0x00,0x30,0x00,
  0x6b,0x08,0x00,0x00,0x00,0x00,0x70,0x00,0x54,0x00,0x00,0x00,0x3f,0x03,0xe0,0x00,
  0xa8,0x01,0x00,0x00,0xc0,0xff,0xe0,0x00,0xd0,0x00,0x00,0x01,0x00,0x40,0x70,0x00,
  0xb0,0x00,0x40,0x06,0x1e,0x40,0x38,0x00,0x60,0x00,0x30,0x1a,0x10,0x40,0x06,0x00,
  0xa0,0x00,0x1f,0xe2,0x10,0xe0,0x01,0x00,0x40,0x00,0x0f,0x83,0x03,0xb0,0x00,0x80,
  0xc0,0x00,0x3d,0x01,0xff,0x58,0x00,0x40,0x80,0x01,0xc2,0x00,0xfe,0xa8,0x00,0x60,
  0x80,0x02,0x04,0x00,0x0d,0x54,0x00,0x60,0x80,0x00,0x04,0x00,0x06,0xac,0x00,0x60,
  0x00,0x00,0x08,0x00,0x03,0x5f,0xc0,0x60,0x00,0x00,0x08,0x00,0x07,0xf8,0x38,0x60,
  0x00,0x00,0x08,0x00,0xff,0xc0,0x06,0xe0,0x00,0x00,0x04,0x3f,0xfc,0x00,0x01,0xe0,
  0x00,0x00,0x03,0xff,0xc0,0x00,0x00,0xc0,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0x80
};

// ════════════════════════════════════════════════════════════════════════════
//  BITMAPS – Main menu icons (16×16, 2 bytes per row = 32 bytes each)
// ════════════════════════════════════════════════════════════════════════════

// New icons from menu.ino
static const unsigned char PROGMEM image_paint_12_bits[] = {
  0x00,0x00,0x00,0x00,0x0f,0xf0,0x38,0x1c,0x60,0x06,0xc7,0xe3,
  0x1c,0x38,0x30,0x0c,0x01,0x80,0x07,0xe0,0x0c,0x30,0x00,0x00,
  0x01,0x80,0x01,0x80,0x00,0x00,0x00,0x00
};

static const unsigned char PROGMEM image_paint_13_bits[] = {
  0x00,0x00,0x01,0x00,0x01,0x80,0x01,0x40,0x01,0x20,0x09,0x20,
  0x05,0x40,0x03,0x80,0x03,0x80,0x05,0x40,0x09,0x20,0x01,0x20,
  0x01,0x40,0x01,0x80,0x01,0x00,0x00,0x00
};

static const unsigned char PROGMEM image_earth_bits[] = {
  0x07,0xc0,0x1e,0x70,0x27,0xf8,0x61,0xe4,0x43,0xe4,0x87,0xca,
  0x9f,0xf6,0xdf,0x82,0xdf,0x82,0xe3,0xc2,0x61,0xf4,0x70,0xf4,
  0x31,0xf8,0x1b,0xf0,0x07,0xc0,0x00,0x00
};

static const unsigned char PROGMEM image_paint_17_bits[] = {
  0x80,0xe0,0xc1,0x60,0x42,0x80,0x22,0x8c,0x13,0x0c,0x0a,0xb4,
  0x06,0x48,0x05,0xf0,0x0b,0x00,0x14,0xe0,0x29,0xb0,0x50,0xd8,
  0xa0,0x6c,0xc0,0x34,0x00,0x1c,0x00,0x00
};

static const unsigned char PROGMEM image_paint_16_bits[] = {
  0x04,0x10,0x1c,0x10,0x28,0x54,0x48,0x38,0x51,0x6d,0x90,0x38,
  0x90,0x54,0x90,0x10,0x88,0x10,0x88,0x06,0x46,0x1c,0x41,0xe4,
  0x20,0x08,0x18,0x30,0x07,0xc0,0x00,0x00
};

static const unsigned char PROGMEM image_paint_18_bits[] = {
  0x00,0x00,0x00,0x7c,0x00,0x00,0x83,0xfc,0x00,0x80,0x02,0x00,
  0x80,0x02,0x00,0x8f,0xff,0x00,0x90,0x00,0x80,0x97,0xff,0xc0,
  0xa4,0x00,0x40,0xa8,0x00,0x40,0xc8,0x00,0x80,0xd0,0x00,0x80,
  0x90,0x01,0x00,0xa0,0x01,0x00,0x7f,0xfe,0x00,0x00,0x00,0x00
};

static const unsigned char PROGMEM image_hand_high_five_bits[] = {
  0x00,0xc0,0x00,0x19,0x4c,0x00,0x15,0x54,0x00,0x15,0x54,0x00,
  0x15,0x54,0x00,0x13,0x69,0x80,0x0b,0x6a,0x80,0x09,0x4d,0x00,
  0x6c,0x19,0x00,0x95,0xfa,0x00,0xcc,0x04,0x00,0x22,0x6c,0x00,
  0x31,0x84,0x00,0x18,0x88,0x00,0x0c,0x88,0x00,0x06,0x10,0x00
};

static const unsigned char PROGMEM image_paint_15_bits[] = {
  0x03,0xc0,0x12,0x48,0x2c,0x34,0x40,0x02,0x23,0xc4,0x24,0x24,
  0xc8,0x13,0x88,0x11,0x88,0x11,0xc8,0x13,0x24,0x24,0x23,0xc4,
  0x40,0x02,0x2c,0x34,0x12,0x48,0x03,0xc0
};

static const unsigned char PROGMEM image_paint_14_bits[] = {
  0x07,0xc0,0x18,0x30,0x23,0x88,0x44,0x44,0x4b,0xa4,0x8c,0xa2,
  0x80,0xa2,0x81,0x42,0x82,0x82,0x83,0x82,0x40,0x04,0x42,0x84,
  0x23,0x88,0x18,0x30,0x07,0xc0,0x00,0x00
};

// ════════════════════════════════════════════════════════════════════════════
//  OTA HTML
// ════════════════════════════════════════════════════════════════════════════

static const char OTA_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html><html>
<head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>HandyOS OTA</title>
<style>
body{background:#0a0a0a;color:#fff;font-family:monospace;display:flex;
     flex-direction:column;align-items:center;justify-content:center;
     min-height:100vh;margin:0;padding:20px;box-sizing:border-box;}
h2{color:#07ff00;margin-bottom:6px;}
p{color:#888;font-size:.9em;margin:4px 0;}
.card{background:#111;border:1px solid #222;border-radius:12px;padding:28px 24px;max-width:420px;width:100%;}
label{display:block;background:#1a1a1a;border:2px dashed #333;border-radius:8px;
      padding:30px 16px;text-align:center;cursor:pointer;color:#555;transition:border-color .2s;}
label:hover,label.has-file{border-color:#07ff00;color:#07ff00;}
input[type=file]{display:none;}
button{margin-top:16px;width:100%;padding:14px;background:#07ff00;color:#000;
       border:none;border-radius:8px;font-size:1em;font-weight:bold;cursor:pointer;}
button:disabled{background:#1a4a1a;color:#333;cursor:not-allowed;}
#prog{margin-top:16px;display:none;}
#bar-wrap{background:#1a1a1a;border-radius:4px;height:14px;overflow:hidden;}
#bar{height:100%;width:0%;background:#07ff00;transition:width .3s;}
#pct{text-align:center;color:#07ff00;margin-top:6px;font-size:.9em;}
#msg{text-align:center;margin-top:12px;font-size:.9em;}
.ok{color:#07ff00;}.err{color:#ff3333;}
</style></head><body>
<div class="card">
<h2>&#9889; HandyOS OTA</h2>
<p>Select firmware .bin and tap Upload.</p>
<p style="color:#444">Device reboots after flashing.</p><br>
<form id="frm" enctype="multipart/form-data">
<label id="lbl" for="file">&#128194; Tap to choose .bin file</label>
<input type="file" id="file" name="firmware" accept=".bin">
<button id="btn" type="button" disabled onclick="upload()">UPLOAD FIRMWARE</button>
</form>
<div id="prog"><div id="bar-wrap"><div id="bar"></div></div><div id="pct">0%</div></div>
<div id="msg"></div></div>
<script>
document.getElementById('file').onchange=function(){
  var n=this.files[0]?this.files[0].name:'';
  document.getElementById('lbl').textContent=n||'Tap to choose .bin file';
  document.getElementById('lbl').className=n?'has-file':'';
  document.getElementById('btn').disabled=!n;
};
function upload(){
  var f=document.getElementById('file').files[0];if(!f)return;
  var fd=new FormData();fd.append('firmware',f);
  var x=new XMLHttpRequest();x.open('POST','/update');
  document.getElementById('prog').style.display='block';
  document.getElementById('btn').disabled=true;
  x.upload.onprogress=function(e){if(e.lengthComputable){
    var p=Math.round(e.loaded/e.total*100);
    document.getElementById('bar').style.width=p+'%';
    document.getElementById('pct').textContent=p+'%';}};
  x.onload=function(){if(x.status==200){
    document.getElementById('bar').style.width='100%';
    document.getElementById('pct').textContent='100%';
    document.getElementById('msg').innerHTML='<span class="ok">&#10003; Flash complete! Rebooting...</span>';
  }else{document.getElementById('msg').innerHTML='<span class="err">&#10007; Upload failed ('+x.status+')</span>';
    document.getElementById('btn').disabled=false;}};
  x.onerror=function(){document.getElementById('msg').innerHTML='<span class="err">&#10007; Connection error</span>';
    document.getElementById('btn').disabled=false;};
  x.send(fd);}
</script></body></html>
)rawhtml";

// ════════════════════════════════════════════════════════════════════════════
//  UTILITY HELPERS
// ════════════════════════════════════════════════════════════════════════════

void sFill(int x, int y, int w, int h, uint16_t col) {
  tft.fillRect(x, y, w, h, col);
}

void drawHdr(const char* label) {
  sFill(0, 0, SCR_W, 12, C_HDR);
  tft.setTextColor(C_TXT); tft.setTextSize(1);
  tft.setCursor((SCR_W - (int)strlen(label) * 6) / 2, 3);
  tft.print(label);
}

void drawHdrCyan(const char* label) {
  sFill(0, 0, SCR_W, 12, C_HDR);
  tft.setTextColor(C_SEL); tft.setTextSize(1);
  tft.setCursor(2, 3); tft.print(label);
  tft.drawLine(0, 12, SCR_W, 12, C_TXT);
}

bool btnDwnPressed() { return digitalRead(BTN_DWN) == LOW; }

int rssiToBars(int rssi) {
  if (rssi >= -55) return 3;
  if (rssi >= -70) return 2;
  if (rssi >= -85) return 1;
  return 0;
}

void fmtSize(size_t bytes, char* buf, int bufsz) {
  if      (bytes >= 1048576) snprintf(buf, bufsz, "%dM", (int)(bytes / 1048576));
  else if (bytes >= 1024)    snprintf(buf, bufsz, "%dK", (int)(bytes / 1024));
  else                       snprintf(buf, bufsz, "%dB", (int)bytes);
}

uint16_t extColor(const char* name) {
  const char* dot = strrchr(name, '.');
  if (!dot)                   return C_DIM;
  if (strcmp(dot,".txt")==0)  return C_GRN;
  if (strcmp(dot,".bin")==0)  return C_ORG;
  if (strcmp(dot,".json")==0) return C_SEL;
  if (strcmp(dot,".cfg")==0)  return C_YLW;
  return C_DIM;
}

// ── Forward declarations ──────────────────────────────────────────────────────
void drawMainMenu();
void drawWifiMain();
void drawWifiScan();
void drawWifiPass();
void drawOtaUpdate();
void drawFileManager();
void drawFileAction();
void drawTextViewer();
void drawHexViewer();
void doConnect();
void initFileManager();
void initTextViewer(const char*);
void initHexViewer(const char*);
void loadTextPage();
void doWifiScan();
void initWifiPass();

void goMenu()        { curState = ST_MENU;      menuSel = 0; drawMainMenu(); }
void returnToWifi()  { curState = ST_WIFI_MAIN; drawWifiMain(); }

// ════════════════════════════════════════════════════════════════════════════
//  BOOT SCREEN
// ════════════════════════════════════════════════════════════════════════════

void drawBootScreen() {
  tft.fillScreen(0x0);
  tft.drawBitmap(7,   37, image_DolphinWait_bits,        59, 54, 0xFFFF);
  tft.drawBitmap(0,    4, image_Background_bits,         128, 11, 0xFFFF);
  tft.drawLine(-8,  91, 162,  91, 0xFFFF);
  tft.drawBitmap(108, 19, image_display_brightness_bits, 15, 16, 0xFFFF);
  tft.drawBitmap(73,  22, image_cloud_bits,              17, 16, 0xFFFF);
  tft.drawBitmap(102, 43, image_cloud_bits,              17, 16, 0xFFFF);
  tft.drawBitmap(73,  59, image_cloud_bits,              17, 16, 0xFFFF);
  tft.drawBitmap(17,  21, image_cloud_bits,              17, 16, 0xFFFF);
}

// ════════════════════════════════════════════════════════════════════════════
//  MAIN MENU  3×3 grid  –  FIXED LAYOUT
//
//  Screen = 128×128
//  Top bar line at y=13  (1px)
//  Label bar at bottom   y=114..127  (14px)
//  Usable area: y=14 to y=113  = 100px tall, 128px wide
//
//  3 columns: each cell 38px wide, gap 3px, left margin 3px
//    col0 x=3   col1 x=44   col2 x=85
//  3 rows: each cell 30px tall, gap 3px, top margin 3px
//    row0 y=17  row1 y=50   row2 y=83
//
//  Icon centred inside 30×30 cell
// ════════════════════════════════════════════════════════════════════════════

// Cell top-left corners
static const int CELL_X[3] = { 3, 44, 85 };
static const int CELL_Y[3] = { 17, 50, 83 };
#define CELL_W 36
#define CELL_H 30

// Menu labels for bottom bar
static const char* MENU_LABELS[9] = {
  "WiFi", "Bluetooth", "Social",
  "Tools", "Islam", "File",
  "Games", "Settings", "About"
};

struct MenuIcon { const uint8_t* bmp; int w, h; };

// icon_file is 18×16 (3 bytes per row × 16 rows = 48 bytes)
// All others are 16×16 (2 bytes per row × 16 rows = 32 bytes)
static const MenuIcon MENU_ICONS[9] = {
  { image_paint_12_bits,     16, 16 },  // 0 WiFi
  { image_paint_13_bits,     16, 16 },  // 1 Bluetooth
  { image_earth_bits,        15, 16 },  // 2 Social
  { image_paint_17_bits,     14, 16 },  // 3 Tools
  { image_paint_16_bits,     16, 16 },  // 4 Islam
  { image_paint_18_bits,     18, 16 },  // 5 File
  { image_hand_high_five_bits,17,16 },  // 6 Games
  { image_paint_15_bits,     16, 16 },  // 7 Settings
  { image_paint_14_bits,     15, 16 }   // 8 About
};

void drawMenuCell(int idx, bool selected) {
  int row = idx / 3, col = idx % 3;
  int cx = CELL_X[col];
  int cy = CELL_Y[row];
  int iw = MENU_ICONS[idx].w;
  int ih = MENU_ICONS[idx].h;
  // Centre icon inside cell
  int ix = cx + (CELL_W - iw) / 2;
  int iy = cy + (CELL_H - ih) / 2;

  if (selected) {
    tft.fillRect(cx, cy, CELL_W, CELL_H, C_TXT);
    tft.drawRect(cx - 1, cy - 1, CELL_W + 2, CELL_H + 2, C_SEL);
    tft.drawBitmap(ix, iy, MENU_ICONS[idx].bmp, iw, ih, C_BG);
  } else {
    tft.fillRect(cx, cy, CELL_W, CELL_H, C_BG);
    tft.drawRect(cx, cy, CELL_W, CELL_H, C_TXT);
    tft.drawBitmap(ix, iy, MENU_ICONS[idx].bmp, iw, ih, C_TXT);
  }
}

void drawMenuLabel(int idx) {
  // Clear label area and print current item name
  sFill(0, 114, SCR_W, 14, C_HDR);
  tft.setTextColor(C_SEL); tft.setTextSize(1);
  const char* lbl = MENU_LABELS[idx];
  int lx = (SCR_W - (int)strlen(lbl) * 6) / 2;
  tft.setCursor(lx, 118);
  tft.print(lbl);
}

void drawMainMenu() {
  tft.fillScreen(C_BG);
  // Top decoration line
  tft.drawLine(0, 13, SCR_W, 13, C_TXT);
  // Draw all cells
  for (int i = 0; i < 9; i++) drawMenuCell(i, i == menuSel);
  // Bottom label
  drawMenuLabel(menuSel);
}

void loopMenu() {
  if (millis() - lastBtn < DB) return;
  int prev = menuSel;
  bool moved = false;

  if      (digitalRead(BTN_UP)  == LOW) { menuSel = (menuSel - 3 + 9) % 9; lastBtn = millis(); moved = true; }
  else if (btnDwnPressed())              { menuSel = (menuSel + 3) % 9;      lastBtn = millis(); moved = true; }
  else if (digitalRead(BTN_LFT) == LOW) {
    menuSel = (menuSel % 3 == 0) ? menuSel + 2 : menuSel - 1;
    lastBtn = millis(); moved = true;
  }
  else if (digitalRead(BTN_RHT) == LOW) {
    menuSel = (menuSel % 3 == 2) ? menuSel - 2 : menuSel + 1;
    lastBtn = millis(); moved = true;
  }
  else if (digitalRead(BTN_MID) == LOW) {
    lastBtn = millis();
    switch (menuSel) {
      case 0: curState = ST_WIFI_MAIN; drawWifiMain();  break;
      case 5: initFileManager();                         break;
      default:
        curState = ST_COMING_SOON;
        tft.fillScreen(C_BG); drawHdr("HandyOS");
        tft.setTextColor(C_SEL); tft.setCursor(14, 30);
        tft.print(MENU_LABELS[menuSel]);
        tft.setTextColor(C_DIM); tft.setCursor(4, 46); tft.print("Coming soon!");
        tft.setTextColor(C_DIM); tft.setCursor(4, 60); tft.print("RST = back");
        break;
    }
    return;
  }
  else if (digitalRead(BTN_RST) == LOW) {
    // RST on main menu has no parent – just redraw to be safe
    lastBtn = millis();
  }

  if (moved && prev != menuSel) {
    drawMenuCell(prev,    false);
    drawMenuCell(menuSel, true);
    drawMenuLabel(menuSel);
  }
}

// ════════════════════════════════════════════════════════════════════════════
//  WIFI MAIN MENU
// ════════════════════════════════════════════════════════════════════════════

void drawWifiMain() {
  tft.fillScreen(C_BG);
  drawHdrCyan("WiFi");
  tft.setTextSize(1);
  if (wifiOK) {
    char ssid[33]; WiFi.SSID().toCharArray(ssid, 33);
    tft.setTextColor(C_GRN); tft.setCursor(4, 16); tft.print(ssid);
    tft.setTextColor(C_DIM); tft.setCursor(4, 26); tft.print(WiFi.localIP());
  } else {
    tft.setTextColor(C_DIM); tft.setCursor(4, 20); tft.print("Not connected");
  }
  for (int i = 0; i < 4; i++) {
    int y = 40 + i * 16;
    bool sel = (i == wifiMenuSel);
    sFill(0, y, SCR_W, 14, sel ? C_SEL : C_BG);
    tft.setTextColor(sel ? C_BG : C_TXT);
    tft.setCursor(6, y + 3); tft.print(wifiMenuLabels[i]);
  }
}

void loopWifiMain() {
  if (millis() - lastBtn < DB) return;
  if      (digitalRead(BTN_UP)  == LOW) { wifiMenuSel = (wifiMenuSel - 1 + 4) % 4; lastBtn = millis(); drawWifiMain(); }
  else if (btnDwnPressed())              { wifiMenuSel = (wifiMenuSel + 1) % 4;      lastBtn = millis(); drawWifiMain(); }
  else if (digitalRead(BTN_RST) == LOW) { lastBtn = millis(); goMenu(); }
  else if (digitalRead(BTN_MID) == LOW) {
    lastBtn = millis();
    switch (wifiMenuSel) {
      case 0: curState = ST_WIFI_SCAN; doWifiScan();    break;
      case 1: curState = ST_WIFI_OTA;  drawOtaUpdate(); break;
      case 2: WiFi.disconnect(); wifiOK = false; drawWifiMain(); break;
      case 3: goMenu(); break;
    }
  }
}

// ════════════════════════════════════════════════════════════════════════════
//  WIFI SCAN LIST
// ════════════════════════════════════════════════════════════════════════════

void doWifiScan() {
  tft.fillScreen(C_BG); drawHdrCyan("WiFi Networks");
  tft.setTextColor(C_DIM); tft.setCursor(20, 50); tft.print("Scanning...");
  netCount = WiFi.scanNetworks();
  if (netCount > MAX_NET) netCount = MAX_NET;
  netSel = 0;
  drawWifiScan();
}

void drawWifiScan() {
  tft.fillScreen(C_BG); drawHdrCyan("WiFi Networks");
  if (netCount == 0) {
    tft.setTextColor(C_DIM); tft.setCursor(30, 60); tft.print("No networks");
    return;
  }
  tft.setTextSize(1);
  for (int i = 0; i < netCount; i++) {
    int y = 16 + i * 13;
    bool sel = (i == netSel);
    sFill(0, y, SCR_W, 12, sel ? C_SEL : C_BG);
    tft.setTextColor(sel ? C_BG : C_TXT);
    tft.setCursor(2, y + 2);
    int bars = rssiToBars(WiFi.RSSI(i));
    for (int b = 0; b < bars; b++)  tft.print("|");
    for (int b = bars; b < 3; b++)  tft.print(" ");
    tft.print(" ");
    String ssid = WiFi.SSID(i);
    if ((int)ssid.length() > 14) ssid = ssid.substring(0, 14);
    tft.print(ssid);
  }
}

void loopWifiScan() {
  if (millis() - lastBtn < DB) return;
  if      (digitalRead(BTN_UP)  == LOW) { netSel = (netSel - 1 + netCount) % netCount; lastBtn = millis(); drawWifiScan(); }
  else if (btnDwnPressed())              { netSel = (netSel + 1) % netCount;             lastBtn = millis(); drawWifiScan(); }
  else if (digitalRead(BTN_RST) == LOW) { lastBtn = millis(); curState = ST_WIFI_MAIN;  drawWifiMain(); }
  else if (digitalRead(BTN_MID) == LOW) {
    lastBtn = millis();
    WiFi.SSID(netSel).toCharArray(wifiSSID, 33);
    initWifiPass();
  }
}

// ════════════════════════════════════════════════════════════════════════════
//  WIFI CONNECT
// ════════════════════════════════════════════════════════════════════════════

void doConnect() {
  tft.fillScreen(C_BG); drawHdr("WiFi");
  tft.setTextColor(C_DIM); tft.setCursor(4, 20); tft.print("Connecting...");
  WiFi.begin(wifiSSID, wfPass);
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 10000) delay(200);
  tft.fillScreen(C_BG); drawHdr("WiFi");
  if (WiFi.status() == WL_CONNECTED) {
    wifiOK = true;
    tft.setTextColor(C_GRN); tft.setCursor(4, 20); tft.print("Connected!");
    tft.setTextColor(C_TXT); tft.setCursor(4, 32); tft.print(wifiSSID);
    tft.setTextColor(C_DIM); tft.setCursor(4, 44); tft.print(WiFi.localIP());
    tft.setTextColor(C_DIM); tft.setCursor(4, 60); tft.print("any btn = back");
  } else {
    wifiOK = false;
    tft.setTextColor(C_ACC); tft.setCursor(4, 20); tft.print("Failed!");
    tft.setTextColor(C_TXT); tft.setCursor(4, 34); tft.print("Wrong password?");
    tft.setTextColor(C_DIM); tft.setCursor(4, 48); tft.print("any btn = back");
  }
  while (digitalRead(BTN_MID) == HIGH && digitalRead(BTN_RST) == HIGH &&
         digitalRead(BTN_UP)  == HIGH && !btnDwnPressed()             &&
         digitalRead(BTN_LFT) == HIGH && digitalRead(BTN_RHT) == HIGH) {
    delay(50);
  }
  delay(200);
  curState = ST_WIFI_MAIN; drawWifiMain();
}

// ════════════════════════════════════════════════════════════════════════════
//  WIFI PASSWORD KEYBOARD
// ════════════════════════════════════════════════════════════════════════════

char kbChar(int r, int c) {
  if (r >= 3) return 0;
  const char* row = nullptr;
  if      (kbPage == 0) row = KBL[r];
  else if (kbPage == 1) row = KBU[r];
  else if (kbPage == 2) row = KBN[r];
  else                  row = KBS[r];
  if (!row) return 0;
  char ch = row[c];
  return (ch == ' ') ? 0 : ch;
}

int kbGroup(int c) {
  if (c <= 1) return 0; if (c <= 3) return 1;
  if (c <= 5) return 2; if (c <= 7) return 3;
  return 4;
}

void drawWifiPassInput() {
  tft.drawRect(2, 14, 124, 13, C_SEL);
  sFill(3, 15, 122, 11, C_HDR);
  tft.setTextColor(C_TXT); tft.setTextSize(1);
  int st = max(0, wfPassLen - 18);
  char dp[19]; strncpy(dp, wfPass + st, 18); dp[18] = '\0';
  tft.setCursor(4, 18); tft.print(dp);
  int filled = strlen(dp);
  for (int i = filled; i < 18; i++) { tft.setCursor(4 + i*6, 18); tft.print(" "); }
  sFill(4 + min(wfPassLen, 18) * 6, 16, 2, 9, C_SEL);
}

void drawWifiPassKey(int r, int c, bool sel) {
  int ky = 38 + r * 12;
  if (r >= 3) {
    ky = 38 + 3 * 12;
    if      (c <= 1) { bool s=(r==3&&c<=1);            sFill(2, ky,23,11,s?C_SEL:C_HDR); tft.setTextColor(s?C_BG:C_TXT); tft.setTextSize(1); tft.setCursor(5, ky+3);   tft.print("SHF"); }
    else if (c <= 3) { bool s=(r==3&&c>=2&&c<=3);      sFill(26,ky,23,11,s?C_SEL:C_HDR); tft.setTextColor(s?C_BG:C_TXT); tft.setCursor(29,ky+3); tft.print(kbPage<2?"123":"abc"); }
    else if (c <= 5) { bool s=(r==3&&c>=4&&c<=5);      sFill(50,ky,23,11,s?C_SEL:C_HDR); tft.setTextColor(s?C_BG:C_TXT); tft.setCursor(53,ky+3); tft.print("SPC"); }
    else if (c <= 7) { bool s=(r==3&&c>=6&&c<=7);      sFill(74,ky,23,11,s?C_SEL:C_ACC); tft.setTextColor(s?C_BG:C_TXT); tft.setCursor(77,ky+3); tft.print("DEL"); }
    else             { bool s=(r==3&&c>=8);             sFill(98,ky,28,11,s?C_SEL:C_GRN); tft.setTextColor(C_BG);         tft.setCursor(105,ky+3);tft.print("OK"); }
    return;
  }
  int kx = 2 + c * 12;
  char ch = kbChar(r, c);
  if (ch) {
    sFill(kx, ky, 11, 11, sel ? C_SEL : C_HDR);
    tft.setTextColor(sel ? C_BG : C_TXT); tft.setTextSize(1);
    tft.setCursor(kx+3, ky+3);
    char tmp[2] = {ch, 0}; tft.print(tmp);
  } else {
    sFill(kx, ky, 11, 11, C_BG);
  }
}

void drawWifiPass() {
  tft.fillScreen(C_BG);
  sFill(0, 0, SCR_W, 13, C_HDR);
  tft.setTextColor(C_ACC); tft.setTextSize(1);
  char hb[20]; snprintf(hb, 20, "%.16s", wifiSSID);
  tft.setCursor(2, 4); tft.print(hb);
  tft.drawRect(2, 14, 124, 13, C_SEL);
  sFill(3, 15, 122, 11, C_HDR);
  tft.setTextColor(C_TXT); tft.setTextSize(1);
  int st = max(0, wfPassLen - 18);
  char dp[19]; strncpy(dp, wfPass + st, 18); dp[18] = '\0';
  tft.setCursor(4, 18); tft.print(dp);
  sFill(4 + min(wfPassLen, 18) * 6, 16, 2, 9, C_SEL);
  const char* pn[] = {"abc","ABC","123","sym"};
  tft.setTextColor(C_DIM); tft.setTextSize(1); tft.setCursor(2, 30);
  tft.print("["); tft.print(pn[kbPage]); tft.print("] len:"); tft.print(wfPassLen);
  for (int r = 0; r < 3; r++) {
    for (int c = 0; c < 10; c++) {
      int kx = 2+c*12, ky = 38+r*12;
      bool s = (r == kbRow && c == kbCol);
      char ch = kbChar(r, c);
      if (ch) {
        sFill(kx, ky, 11, 11, s?C_SEL:C_HDR);
        tft.setTextColor(s?C_BG:C_TXT); tft.setTextSize(1);
        tft.setCursor(kx+3, ky+3); char tmp[2]={ch,0}; tft.print(tmp);
      } else { sFill(kx, ky, 11, 11, C_BG); }
    }
  }
  int ky = 38 + 3*12;
  { bool s=(kbRow==3&&kbCol<=1);           sFill(2, ky,23,11,s?C_SEL:C_HDR); tft.setTextColor(s?C_BG:C_TXT); tft.setTextSize(1); tft.setCursor(5, ky+3);  tft.print("SHF"); }
  { bool s=(kbRow==3&&kbCol>=2&&kbCol<=3); sFill(26,ky,23,11,s?C_SEL:C_HDR); tft.setTextColor(s?C_BG:C_TXT); tft.setCursor(29,ky+3); tft.print(kbPage<2?"123":"abc"); }
  { bool s=(kbRow==3&&kbCol>=4&&kbCol<=5); sFill(50,ky,23,11,s?C_SEL:C_HDR); tft.setTextColor(s?C_BG:C_TXT); tft.setCursor(53,ky+3); tft.print("SPC"); }
  { bool s=(kbRow==3&&kbCol>=6&&kbCol<=7); sFill(74,ky,23,11,s?C_SEL:C_ACC); tft.setTextColor(s?C_BG:C_TXT); tft.setCursor(77,ky+3); tft.print("DEL"); }
  { bool s=(kbRow==3&&kbCol>=8);           sFill(98,ky,28,11,s?C_SEL:C_GRN); tft.setTextColor(C_BG); tft.setCursor(105,ky+3); tft.print("OK"); }
}

void initWifiPass() {
  curState = ST_WIFI_PASS;
  memset(wfPass, 0, sizeof(wfPass)); wfPassLen = 0;
  kbRow = 0; kbCol = 0; kbPage = 0;
  drawWifiPass();
}

void loopWifiPass() {
  if (millis() - lastBtn < DB) return;
  int prevRow = kbRow, prevCol = kbCol;
  bool moved = false, typed = false;

  if      (digitalRead(BTN_UP)  == LOW) { kbRow = (kbRow-1+4)%4;    lastBtn=millis(); moved=true; }
  else if (btnDwnPressed())              { kbRow = (kbRow+1)%4;       lastBtn=millis(); moved=true; }
  else if (digitalRead(BTN_LFT) == LOW) { kbCol = (kbCol-1+10)%10;  lastBtn=millis(); moved=true; }
  else if (digitalRead(BTN_RHT) == LOW) { kbCol = (kbCol+1)%10;      lastBtn=millis(); moved=true; }
  else if (digitalRead(BTN_MID) == LOW) {
    lastBtn = millis();
    if (kbRow < 3) {
      char ch = kbChar(kbRow, kbCol);
      if (ch && wfPassLen < 64) { wfPass[wfPassLen++] = ch; wfPass[wfPassLen] = '\0'; typed = true; }
    } else {
      int g = kbGroup(kbCol);
      if      (g == 0) { kbPage = (kbPage==0)?1:(kbPage==1?0:kbPage); }
      else if (g == 1) { kbPage = (kbPage<2)?2:0; }
      else if (g == 2) { if (wfPassLen<64){ wfPass[wfPassLen++]=' '; wfPass[wfPassLen]='\0'; typed=true; } }
      else if (g == 3) { if (wfPassLen>0) { wfPass[--wfPassLen]='\0'; typed=true; } }
      else             { doConnect(); return; }
      if (!typed) { drawWifiPass(); return; }
    }
  }
  else if (digitalRead(BTN_SET) == LOW) {
    lastBtn = millis();
    if (wfPassLen > 0) { wfPass[--wfPassLen] = '\0'; typed = true; }
  }
  else if (digitalRead(BTN_RST) == LOW) {
    lastBtn = millis(); WiFi.disconnect();
    curState = ST_WIFI_SCAN; drawWifiScan(); return;
  }

  if (moved) {
    drawWifiPassKey(prevRow, prevCol, false);
    drawWifiPassKey(kbRow,   kbCol,   true);
  } else if (typed) {
    drawWifiPassInput();
    sFill(2, 28, 80, 10, C_BG);
    const char* pn[] = {"abc","ABC","123","sym"};
    tft.setTextColor(C_DIM); tft.setTextSize(1); tft.setCursor(2, 30);
    tft.print("["); tft.print(pn[kbPage]); tft.print("] len:"); tft.print(wfPassLen);
  }
}

// ════════════════════════════════════════════════════════════════════════════
//  OTA UPDATE
// ════════════════════════════════════════════════════════════════════════════

void otaHandleRoot()   { otaServer.send_P(200, "text/html", OTA_HTML); }

void otaHandleUpdate() {
  bool ok = !Update.hasError();
  otaServer.send(200, "text/plain", ok ? "OK" : "FAIL");
  if (ok) {
    tft.fillScreen(C_BG); drawHdr("OTA UPDATE");
    tft.setTextColor(C_GRN); tft.setTextSize(1); tft.setCursor(4,24); tft.print("Flash complete!");
    tft.setTextColor(C_YLW); tft.setCursor(4,38); tft.print("Rebooting...");
    sFill(7,55,114,8,C_GRN);
    delay(1500); ESP.restart();
  }
}

void otaHandleUpload() {
  HTTPUpload& upload = otaServer.upload();
  if (upload.status == UPLOAD_FILE_START) {
    tft.fillScreen(C_BG); drawHdr("OTA UPDATE");
    tft.setTextColor(C_TXT); tft.setTextSize(1); tft.setCursor(4,20); tft.print("Receiving firmware");
    tft.setTextColor(C_ACC); tft.setCursor(4,32); tft.print("Do not power off!");
    tft.drawRect(6,48,116,10,C_DIM);
    Update.begin(UPDATE_SIZE_UNKNOWN);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    Update.write(upload.buf, upload.currentSize);
    static int pulse = 0; pulse = (pulse + 4) % 110;
    sFill(7,49,114,8,C_BG); sFill(7,49,pulse,8,C_GRN);
    tft.setTextColor(C_TXT); tft.setTextSize(1);
    sFill(0,62,128,10,C_BG); tft.setCursor(4,62);
    tft.print(upload.totalSize/1024); tft.print(" KB received");
  } else if (upload.status == UPLOAD_FILE_END) {
    Update.end(true);
    sFill(7,49,114,8,C_GRN);
    tft.setTextColor(C_YLW); tft.setCursor(4,76); tft.print("Flashing done!");
  }
}

void otaServerStart() {
  otaServer.on("/",       HTTP_GET,  otaHandleRoot);
  otaServer.on("/update", HTTP_POST, otaHandleUpdate, otaHandleUpload);
  otaServer.begin(); otaServerRunning = true;
}

void otaServerStop() { otaServer.stop(); otaServerRunning = false; }

void drawOtaUpdate() {
  tft.fillScreen(C_BG); drawHdr("OTA UPDATE"); tft.setTextSize(1);
  if (!wifiOK) {
    tft.setTextColor(C_ACC); tft.setCursor(4,20); tft.print("WiFi not connected!");
    tft.setTextColor(C_DIM); tft.setCursor(4,32); tft.print("Connect WiFi first");
    tft.setTextColor(C_DIM); tft.setCursor(4,50); tft.print("then come back here");
    return;
  }
  if (!otaServerRunning) {
    tft.setTextColor(C_TXT); tft.setCursor(4,18); tft.print("Web OTA Update");
    tft.setTextColor(C_DIM); tft.setCursor(4,30); tft.print("Press MID to start");
    tft.setTextColor(C_DIM); tft.setCursor(4,42); tft.print("web server. Then");
    tft.setTextColor(C_DIM); tft.setCursor(4,54); tft.print("open browser and");
    tft.setTextColor(C_DIM); tft.setCursor(4,66); tft.print("go to device IP.");
    tft.setTextColor(C_YLW); tft.setCursor(4,82); tft.print("IP: "); tft.print(WiFi.localIP());
  } else {
    tft.setTextColor(C_GRN); tft.setCursor(4,18); tft.print("Server ACTIVE");
    tft.setTextColor(C_DIM); tft.setCursor(4,30); tft.print("Open browser:");
    tft.setTextColor(C_YLW); tft.setCursor(4,44); tft.print("http://"); tft.print(WiFi.localIP());
    tft.setTextColor(C_DIM); tft.setCursor(4,58); tft.print("Choose .bin & upload");
    tft.setTextColor(C_DIM); tft.setCursor(4,70); tft.print("Device auto-reboots");
    tft.setTextColor(C_ACC); tft.setCursor(4,85); tft.print("MID=Stop server");
  }
}

void loopOtaUpdate() {
  if (otaServerRunning) otaServer.handleClient();
  if (millis() - lastBtn < DB) return;
  if (digitalRead(BTN_RST) == LOW) { lastBtn=millis(); returnToWifi(); return; }
  if (digitalRead(BTN_MID) == LOW) {
    lastBtn = millis();
    if (!wifiOK) return;
    if (!otaServerRunning) otaServerStart(); else otaServerStop();
    drawOtaUpdate();
  }
}

// ════════════════════════════════════════════════════════════════════════════
//  FILE MANAGER
// ════════════════════════════════════════════════════════════════════════════

void initFileManager() {
  curState = ST_FILES; fileCount = 0; fileSel = 0; fileScroll = 0;
  if (!SPIFFS.begin(true)) {
    tft.fillScreen(C_BG); drawHdr("Files");
    tft.setTextColor(C_ACC); tft.setCursor(4,30); tft.print("SPIFFS error!"); return;
  }
  File root = SPIFFS.open("/");
  File f    = root.openNextFile();
  while (f && fileCount < MAX_FILES) {
    strncpy(fileList[fileCount].name, f.name(), 63);
    fileList[fileCount].name[63] = '\0';
    fileList[fileCount].size = f.size();
    fileCount++; f = root.openNextFile();
  }
  drawFileManager();
}

void drawFileManager() {
  tft.fillScreen(C_BG);
  sFill(0, 0, SCR_W, 12, C_HDR);
  tft.setTextColor(C_TXT); tft.setTextSize(1);
  char hbuf[24]; snprintf(hbuf, 24, "Files (%d/40)", fileCount);
  tft.setCursor(2, 3); tft.print(hbuf);

  size_t used  = SPIFFS.usedBytes();
  size_t total = SPIFFS.totalBytes();
  sFill(0, 12, SCR_W, 3, C_DIM);
  if (total > 0) { int w=(int)((long)SCR_W*(long)used/(long)total); sFill(0,12,w,3,C_SEL); }

  for (int i = 0; i < FILES_VISIBLE; i++) {
    int fi = fileScroll + i;
    if (fi >= fileCount) break;
    int y = 16 + i*16;
    bool sel = (fi == fileSel);
    sFill(0, y, SCR_W, 15, sel ? C_SEL : (i%2==0 ? C_BG : C_MID));
    sFill(1, y+4, 3, 7, extColor(fileList[fi].name));
    tft.setTextColor(sel ? C_BG : C_TXT);
    tft.setCursor(7, y+4);
    char trunc[17]; strncpy(trunc, fileList[fi].name, 16); trunc[16]='\0';
    tft.print(trunc);
    char sz[8]; fmtSize(fileList[fi].size, sz, 8);
    tft.setTextColor(sel ? C_BG : C_DIM);
    tft.setCursor(SCR_W - (int)strlen(sz)*6 - 2, y+4); tft.print(sz);
  }

  if (fileCount > FILES_VISIBLE) {
    sFill(SCR_W-3, 16, 3, FILES_VISIBLE*16, C_DIM);
    int thumbH = max(4, (FILES_VISIBLE*FILES_VISIBLE*16)/fileCount);
    int thumbY = 16 + (fileScroll*(FILES_VISIBLE*16-thumbH))/max(1,fileCount-FILES_VISIBLE);
    sFill(SCR_W-3, thumbY, 3, thumbH, C_SEL);
  }
  tft.setTextColor(C_DIM); tft.setCursor(0, SCR_H-9);
  tft.print("MID=action  RST=back");
}

void loopFileManager() {
  if (millis() - lastBtn < DB) return;
  if (digitalRead(BTN_UP) == LOW) {
    lastBtn=millis();
    if (fileSel>0) { fileSel--; if (fileSel<fileScroll) fileScroll=fileSel; drawFileManager(); }
  } else if (btnDwnPressed()) {
    lastBtn=millis();
    if (fileSel<fileCount-1) { fileSel++; if (fileSel>=fileScroll+FILES_VISIBLE) fileScroll=fileSel-FILES_VISIBLE+1; drawFileManager(); }
  } else if (digitalRead(BTN_RST) == LOW) {
    lastBtn=millis(); goMenu();
  } else if (digitalRead(BTN_MID) == LOW || digitalRead(BTN_SET) == LOW) {
    lastBtn=millis();
    if (fileCount>0) { fileActionSel=0; fileDeleteConfirm=false; curState=ST_FILE_ACTION; drawFileAction(); }
  }
}

// ── File Action ───────────────────────────────────────────────────────────────

void drawFileAction() {
  tft.fillScreen(C_BG);
  sFill(0, 0, SCR_W, 12, C_HDR);
  tft.setTextColor(C_TXT); tft.setTextSize(1);
  char trunc[18]; strncpy(trunc, fileList[fileSel].name, 17); trunc[17]='\0';
  tft.setCursor(2,3); tft.print(trunc);
  if (fileDeleteConfirm) {
    tft.setTextColor(C_ACC); tft.setCursor(4,22); tft.print("Delete file?");
    tft.setTextColor(C_TXT); tft.setCursor(4,36); tft.print(trunc);
    tft.setTextColor(C_DIM); tft.setCursor(4,56); tft.print("MID=YES   RST=NO");
    return;
  }
  for (int i=0; i<6; i++) {
    int y=16+i*16; bool sel=(i==fileActionSel);
    sFill(0,y,SCR_W,14,sel?C_SEL:C_BG);
    tft.setTextColor(sel?C_BG:C_TXT); tft.setCursor(6,y+3); tft.print(fileActionLabels[i]);
  }
}

void loopFileAction() {
  if (millis() - lastBtn < DB) return;
  if (fileDeleteConfirm) {
    if (digitalRead(BTN_MID) == LOW) { lastBtn=millis(); SPIFFS.remove(fileList[fileSel].name); initFileManager(); }
    else if (digitalRead(BTN_RST)==LOW) { lastBtn=millis(); fileDeleteConfirm=false; drawFileAction(); }
    return;
  }
  if      (digitalRead(BTN_UP)  == LOW) { fileActionSel=(fileActionSel-1+6)%6; lastBtn=millis(); drawFileAction(); }
  else if (btnDwnPressed())              { fileActionSel=(fileActionSel+1)%6;   lastBtn=millis(); drawFileAction(); }
  else if (digitalRead(BTN_RST) == LOW) { lastBtn=millis(); curState=ST_FILES; drawFileManager(); }
  else if (digitalRead(BTN_MID) == LOW) {
    lastBtn=millis();
    switch (fileActionSel) {
      case 0: initTextViewer(fileList[fileSel].name); break;
      case 1: initHexViewer(fileList[fileSel].name);  break;
      case 2:
        tft.fillScreen(C_BG); drawHdr(fileList[fileSel].name);
        tft.setTextColor(C_SEL); tft.setCursor(10,40); tft.print("Rename: coming soon");
        tft.setTextColor(C_DIM); tft.setCursor(4,56);  tft.print("RST = back");
        break;
      case 3: fileDeleteConfirm=true; drawFileAction(); break;
      case 4: {
        tft.fillScreen(C_BG); sFill(0,0,SCR_W,12,C_HDR);
        tft.setTextColor(C_TXT); tft.setTextSize(1); tft.setCursor(2,3); tft.print("File Info");
        char sz[16]; fmtSize(fileList[fileSel].size, sz, 16);
        tft.setTextColor(C_TXT); tft.setCursor(4,18); tft.print(fileList[fileSel].name);
        tft.setTextColor(C_DIM); tft.setCursor(4,34); tft.print("Size: "); tft.print(sz);
        tft.setTextColor(C_DIM); tft.setCursor(4,44); tft.print("Bytes: "); tft.print((unsigned int)fileList[fileSel].size);
        tft.setTextColor(C_DIM); tft.setCursor(4,60); tft.print("RST = back");
        break;
      }
      case 5: curState=ST_FILES; drawFileManager(); break;
    }
  }
}

// ── Text Viewer ───────────────────────────────────────────────────────────────

void loadTextPage() {
  File f = SPIFFS.open(tvFileName);
  if (!f) { tvEnd=true; return; }
  f.seek(tvOffset);
  int idx=0, col=0;
  memset(tvBuf, 0, sizeof(tvBuf));
  int maxCh = TV_COLS * TV_ROWS;
  while (f.available() && idx < maxCh) {
    char c = (char)f.read();
    if (c=='\n' || col>=TV_COLS) {
      tvBuf[idx++]='\n'; col=0;
      if (c!='\n' && idx<maxCh) { tvBuf[idx++]=c; col++; }
    } else { tvBuf[idx++]=c; col++; }
  }
  tvEnd = !f.available(); f.close();
}

void drawTextViewer() {
  tft.fillScreen(C_BG);
  sFill(0,0,SCR_W,12,C_HDR); tft.setTextColor(C_TXT); tft.setTextSize(1);
  char trunc[18]; strncpy(trunc,tvFileName,17); trunc[17]='\0';
  tft.setCursor(2,3); tft.print(trunc);
  tft.setTextColor(C_TXT);
  int y=14, line=0, col=0;
  for (int i=0; tvBuf[i] && line<TV_ROWS; i++) {
    if (tvBuf[i]=='\n') { y+=8; line++; col=0; continue; }
    tft.setCursor(col*6, y); tft.print(tvBuf[i]); col++;
    if (col>=TV_COLS) { y+=8; line++; col=0; }
  }
  tft.setTextColor(C_DIM); tft.setCursor(0, SCR_H-9);
  tft.print(tvEnd ? "(end) MID=rewind" : "MID/DWN=next RST=bk");
}

void initTextViewer(const char* fname) {
  strncpy(tvFileName, fname, 63); tvFileName[63]='\0';
  tvOffset=0; tvEnd=false;
  curState=ST_FILE_TEXT; loadTextPage(); drawTextViewer();
}

void loopTextViewer() {
  if (millis()-lastBtn < DB) return;
  if (digitalRead(BTN_RST)==LOW) { lastBtn=millis(); curState=ST_FILE_ACTION; drawFileAction(); }
  else if (digitalRead(BTN_MID)==LOW || btnDwnPressed()) {
    lastBtn=millis();
    if (tvEnd) { tvOffset=0; }
    else {
      File f=SPIFFS.open(tvFileName);
      if (f) {
        f.seek(tvOffset);
        int count=0, col=0;
        while (f.available() && count<TV_COLS*TV_ROWS) {
          char c=(char)f.read(); count++;
          if (c=='\n') col=0; else { col++; if (col>=TV_COLS) col=0; }
        }
        tvOffset+=count; f.close();
      }
    }
    loadTextPage(); drawTextViewer();
  }
}

// ── Hex Viewer ────────────────────────────────────────────────────────────────

void drawHexViewer() {
  tft.fillScreen(C_BG);
  sFill(0,0,SCR_W,12,C_HDR); tft.setTextColor(C_TXT); tft.setTextSize(1);
  char trunc[18]; strncpy(trunc,hxFileName,17); trunc[17]='\0';
  tft.setCursor(2,3); tft.print(trunc);
  File f=SPIFFS.open(hxFileName);
  if (!f) return;
  f.seek(hxOffset);
  uint8_t rowbuf[HX_COLS];
  for (int row=0; row<HX_ROWS; row++) {
    int y=14+row*18;
    memset(rowbuf,0,HX_COLS);
    int got=f.read(rowbuf,HX_COLS);
    if (got<=0) break;
    tft.setTextColor(C_DIM);
    char off[5]; snprintf(off,5,"%04lX",hxOffset+(long)row*HX_COLS);
    tft.setCursor(0,y); tft.print(off);
    tft.setTextColor(C_TXT);
    for (int c=0; c<got; c++) {
      char hx[3]; snprintf(hx,3,"%02X",rowbuf[c]);
      tft.setCursor(26+c*18,y); tft.print(hx);
    }
    tft.setTextColor(C_DIM);
    for (int c=0; c<got; c++) {
      char asc=(rowbuf[c]>=0x20&&rowbuf[c]<0x7F)?(char)rowbuf[c]:'.';
      tft.setCursor(26+HX_COLS*18+4+c*6,y); tft.print(asc);
    }
  }
  f.close();
  tft.setTextColor(C_DIM);
  char foot[26]; snprintf(foot,26,"%04lX/%04lX RST=back",hxOffset,hxFileSize);
  tft.setCursor(0,SCR_H-9); tft.print(foot);
}

void initHexViewer(const char* fname) {
  strncpy(hxFileName,fname,63); hxFileName[63]='\0';
  hxOffset=0;
  File f=SPIFFS.open(hxFileName);
  hxFileSize=f?(long)f.size():0L;
  if (f) f.close();
  curState=ST_FILE_HEX; drawHexViewer();
}

void loopHexViewer() {
  if (millis()-lastBtn < DB) return;
  int pageBytes=HX_ROWS*HX_COLS; bool redraw=false;
  if      (digitalRead(BTN_RST)==LOW) { lastBtn=millis(); curState=ST_FILE_ACTION; drawFileAction(); }
  else if (digitalRead(BTN_MID)==LOW || btnDwnPressed()) {
    lastBtn=millis();
    if (hxOffset+pageBytes<hxFileSize) { hxOffset+=pageBytes; redraw=true; }
  } else if (digitalRead(BTN_UP)==LOW) {
    lastBtn=millis();
    if      (hxOffset>=pageBytes) { hxOffset-=pageBytes; redraw=true; }
    else if (hxOffset>0)          { hxOffset=0;          redraw=true; }
  }
  if (redraw) drawHexViewer();
}

// ════════════════════════════════════════════════════════════════════════════
//  COMING SOON
// ════════════════════════════════════════════════════════════════════════════

void loopComingSoon() {
  if (millis()-lastBtn < DB) return;
  if (digitalRead(BTN_MID)==LOW || digitalRead(BTN_RST)==LOW) { lastBtn=millis(); goMenu(); }
}

// ════════════════════════════════════════════════════════════════════════════
//  SETUP & LOOP
// ════════════════════════════════════════════════════════════════════════════

void setup() {
  Serial.begin(115200);

  const int btns[] = {BTN_UP, BTN_DWN, BTN_LFT, BTN_RHT, BTN_MID, BTN_SET, BTN_RST};
  for (int i = 0; i < 7; i++) pinMode(btns[i], INPUT_PULLUP);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  tft.initR(INITR_144GREENTAB);
  tft.setRotation(0);
  tft.fillScreen(C_BG);

  curState = ST_BOOT;
  drawBootScreen();
  delay(2000);

  curState = ST_MENU;
  menuSel  = 0;
  drawMainMenu();
}

void loop() {
  switch (curState) {
    case ST_BOOT:        break;
    case ST_MENU:        loopMenu();        break;
    case ST_WIFI_MAIN:   loopWifiMain();    break;
    case ST_WIFI_SCAN:   loopWifiScan();    break;
    case ST_WIFI_PASS:   loopWifiPass();    break;
    case ST_WIFI_OTA:    loopOtaUpdate();   break;
    case ST_FILES:       loopFileManager(); break;
    case ST_FILE_ACTION: loopFileAction();  break;
    case ST_FILE_TEXT:   loopTextViewer();  break;
    case ST_FILE_HEX:    loopHexViewer();   break;
    case ST_COMING_SOON: loopComingSoon();  break;
  }
}
