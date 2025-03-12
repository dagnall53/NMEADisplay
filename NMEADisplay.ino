//*******************************************************************************

#include <NMEA0183.h>
#include <NMEA0183Msg.h>
#include <NMEA0183Messages.h>

//******  Wifi stuff

#include <WiFi.h>
#include <WiFiUdp.h>
//
#include <esp_now.h>
#include <esp_wifi.h>
#include "ESP_NOW_files.h"
#include "OTA.h"
// #include <WebServer.h> (in ota)
// #include <ESPmDNS.h>

#include "Structures.h"
#include "aux_functions.h"
#include "SineCos.h"
#include <Arduino_GFX_Library.h>
// Original version was for GFX 1.3.1 only. #include "GUITIONESP32-S3-4848S040_GFX_133.h"
#include "Esp32_4inch.h"  // defines GFX !
//*********** for keyboard*************
#include "Keyboard.h"
#include "Touch.h"
TAMC_GT911 ts = TAMC_GT911(TOUCH_SDA, TOUCH_SCL, TOUCH_INT, TOUCH_RST, TOUCH_WIDTH, TOUCH_HEIGHT);

#include <EEPROM.h>
#include "fonts.h"
#include "FreeMonoBold8pt7b.h"
#include "FreeMonoBold27pt7b.h"
#include "FreeSansBold30pt7b.h"
#include "FreeSansBold50pt7b.h"


//For SD card (see display page -98 for test)

#include <SD.h>  // pins set in GFX .h
#include "SPI.h"
#include "FS.h"
//jpeg
#include "JpegFunc.h"
#define JPEG_FILENAME_LOGO "/logo.jpg"  // logo in jpg on sd card
#define AUDIO_FILENAME_START "/StartSound.mp3"
//audio
#include "Audio.h"
//set up Audio
Audio audio;


// some wifi stuff

IPAddress udp_ap(0, 0, 0, 0);   // the IP address to send UDP data in SoftAP mode
IPAddress udp_st(0, 0, 0, 0);   // the IP address to send UDP data in Station mode
IPAddress sta_ip(0, 0, 0, 0);   // the IP address (or received from DHCP if 0.0.0.0) in Station mode
IPAddress gateway(0, 0, 0, 0);  // the IP address for Gateway in Station mode
IPAddress subnet(0, 0, 0, 0);   // the SubNet Mask in Station mode
boolean IsConnected = false;    // may be used in AP_AND_STA to flag connection success (got IP)
int NetworksFound;              // used for scan Networks result. Done at start up!
WiFiUDP Udp;
#define BufferLength 500
char nmea_1[BufferLength];    //serial
char nmea_U[BufferLength];    // NMEA buffer for UDP input port
char nmea_EXT[BufferLength];  // buffer for ESP_now received data

bool EspNowIsRunning = false;
char* pTOKEN;
bool debugpause;
//********** All boat data (instrument readings) are stored as double in a single structure:

tBoatData BoatData;  // BoatData values, int double etc


bool dataUpdated;  // flag that Nmea Data has been updated

//**   structures and helper functions for my variables



//for MP3 player based on (but modified!) https://github.com/VolosR/MakePythonLCDMP3/blob/main/MakePythonLCDMP3.ino#L119
char File_List[20][20];  //array of 20 (20 long) file names for listing..
String runtimes[20];
int file_num = 0;

// MySettings (see structures.h) are the settings for the Display:
// If the structure is changed, be sure to change the Key (first figure) so that new defaults and struct can be set.
MySettings Default_Settings = { 11, "GUESTBOAT", "12345678", "2002", true, true, true };
int Display_Page = 4;  //set last in setup(), but here for clarity?
MySettings Saved_Settings;
MySettings Current_Settings;


int MasterFont;  //global for font! Idea is to use to reset font after 'temporary' seletion of another fone
                 // to be developed ....
struct BarChart {
  int topleftx, toplefty, width, height, bordersize, value, rangemin, rangemax, visible;
  uint16_t barcolour;
};

int _null, _temp;  //null pointers

String Fontname;
int text_height = 12;      //so we can get them if we change heights etc inside functions
int text_offset = 12;      //offset is not equal to height, as subscripts print lower than 'height'
int text_char_width = 12;  // useful for monotype? only NOT USED YET! Try gfx->getTextBounds(string, x, y, &x1, &y1, &w, &h);

//****  My displays are based on 'Button' structures to define position, width height, borders and colours.
// int h, v, width, height, bordersize;  uint16_t backcol, textcol, BorderColor;
Button Fontdescriber = { 0, 0, 480, 75, 5, BLUE, WHITE, BLACK };
Button FontBox = { 0, 150, 480, 330, 5, BLUE, WHITE, BLUE };
Button WindSpeedBox = { 240 - 70, 240 - 40, 140, 80, 5, BLACK, WHITE, BLACK };
//used for single data display
Button BigSingleDisplay = { 20, 100, 440, 360, 5, BLUE, BLACK, BLACK };
//used for nmea display // was only three lines to start!
Button Threelines0 = { 20, 30, 440, 80, 5, BLUE, WHITE, BLACK };
Button Threelines1 = { 20, 130, 440, 80, 5, BLUE, WHITE, BLACK };
Button Threelines2 = { 20, 230, 440, 80, 5, BLUE, WHITE, BLACK };
Button Threelines3 = { 20, 330, 440, 80, 5, BLUE, WHITE, BLACK };
// for the quarter screens
Button topLeftquarter = { 0, 0, 235, 235, 5, BLUE, WHITE, BLACK };
Button bottomLeftquarter = { 0, 240, 235, 235, 5, BLUE, WHITE, BLACK };
Button topRightquarter = { 240, 0, 235, 235, 5, BLUE, WHITE, BLACK };
Button bottomRightquarter = { 240, 240, 235, 235, 5, BLUE, WHITE, BLACK };

Button TopLeftbutton = { 0, 0, 75, 75, 5, BLUE, WHITE, BLACK };
Button TopRightbutton = { 405, 0, 75, 75, 5, BLUE, WHITE, BLACK };
Button BottomRightbutton = { 405, 405, 75, 75, 5, BLUE, WHITE, BLACK };
Button BottomLeftbutton = { 0, 405, 75, 75, 5, BLUE, WHITE, BLACK };

// buttons for the wifi/settings pages // no need to set the bools ?
Button TOPButton = { 80, 10, 320, 35, 5, WHITE, BLACK, BLUE };
Button SecondRowButton = { 80, 50, 320, 35, 5, WHITE, BLACK, BLUE };
Button ThirdRowButton = { 80, 90, 320, 35, 5, WHITE, BLACK, BLUE };
Button FourthRowButton = { 80, 130, 320, 35, 5, WHITE, BLACK, BLUE };
Button FifthRowButton = { 80, 170, 320, 35, 5, WHITE, BLACK, BLUE };

#define sw_width 70
Button Switch1 = { 20, 180, sw_width, 35, 5, WHITE, BLACK, BLUE };
Button Switch2 = { 105, 180, sw_width, 35, 5, WHITE, BLACK, BLUE };
Button Switch3 = { 190, 180, sw_width, 35, 5, WHITE, BLACK, BLUE };
Button Switch4 = { 345, 180, 120, 35, 5, WHITE, BLACK, BLUE };

Button Terminal = { 0, 240, 470, 240, 5, WHITE, BLACK, BLUE };  // inset to try and get printing better! reset to { 0, 240, 480, 240, 5, WHITE, BLACK, BLUE };
//for selections
Button Full0Center = { 80, 55, 320, 55, 5, BLUE, WHITE, BLACK };
Button Full1Center = { 80, 125, 320, 55, 5, BLUE, WHITE, BLACK };
Button Full2Center = { 80, 195, 320, 55, 5, BLUE, WHITE, BLACK };
Button Full3Center = { 80, 265, 320, 55, 5, BLUE, WHITE, BLACK };
Button Full4Center = { 80, 335, 320, 55, 5, BLUE, WHITE, BLACK };
Button Full5Center = { 80, 405, 320, 55, 5, BLUE, WHITE, BLACK };

#define On_Off ? "ON" : "OFF"  // if 1 first case else second (0 or off)

// Draw the compass pointer at an angle in degrees
void DrawCompass(int x, int y, int rad) {
  //Work in progress!
  int Start, dot, colorbar, bigtick;
  float inner, outer;
  outer = (rad * 98) / 120;
  inner = (rad * 98) / 420;  //same as pointer
  // gfx->drawRect(x-rad,y-rad,rad*2,rad*2,BLACK);gfx->drawCircle(x,y,rad,WHITE);gfx->drawCircle(x,y,rad-1,Blue);gfx->drawCircle(x,y,Start,Blue);
  Start = rad * 0.83;     //200
  dot = rad * 0.86;       //208
  colorbar = rad * 0.91;  //220
  bigtick = rad - 1;      //239
  gfx->fillRect(x - rad, y - rad, rad * 2, rad * 2, BLACK);
  gfx->fillCircle(x, y, rad, WHITE);
  gfx->fillCircle(x, y, outer, BLUE);
  gfx->fillCircle(x, y, Start, BLACK);
  gfx->fillCircle(x, y, inner, WHITE);
  gfx->fillCircle(x, y, inner - 5, BLACK);

  //rad =240 example dot is 200 to 208   bar is 200 to 239 wind colours 200 to 220
  // colour segments
  gfx->fillArc(x, y, colorbar, Start, 270 - 45, 270, RED);
  gfx->fillArc(x, y, colorbar, Start, 270, 270 + 45, GREEN);
  //Mark 12 linesarks at 30 degrees
  for (int i = 0; i < (360 / 30); i++) { gfx->fillArc(x, y, bigtick, Start, i * 30, (i * 30) + 1, BLACK); }  //239 to 200
  for (int i = 0; i < (360 / 10); i++) { gfx->fillArc(x, y, dot, Start, i * 10, (i * 10) + 1, BLACK); }      // dots at 10 degrees
}

void drawCompassPointer(int x, int y, int rad, int angle, uint16_t COLOUR, bool to) {
  int x_offset, y_offset, x_arrow, y_arrow, tail1x, tail1y, tail2x, tail2y;  // The resulting offsets from the center point
  float inner, outer;
  while (angle < 0) angle += 360;  // Normalize the angle to the range 0 to 359
  while (angle > 359) angle -= 360;
  // rose radius is default at 100 and returns 100*sine.. so may need to make offsets 2x bigger for 200 size triangel ?
  //if (rad=240){inner=0.6;outer=2;}else{inner=0.3;outer=1;}
  inner = (rad * 98) / 420;  // just a tiny bit smaller!
  outer = (rad * 98) / 120;
  // these print a big triangle
  if (to) {  //Point to boat from wind
    x_offset = x + ((outer * getSine(angle)) / 100);
    y_offset = y + ((outer * getCosine(angle)) / 100);
    x_arrow = x + ((outer * getSine(angle)) / 90);
    y_arrow = y + ((outer * getCosine(angle)) / 90);
    tail1x = x + ((inner * getSine(angle + 20)) / 100);
    tail1y = y + ((inner * getCosine(angle + 20)) / 100);
    tail2x = x + ((inner * getSine(angle - 20)) / 100);
    tail2y = y + ((inner * getCosine(angle - 20)) / 100);
  } else {
    x_offset = x + ((inner * getSine(angle)) / 100);
    y_offset = y + ((inner * getCosine(angle)) / 100);
    x_arrow = x + ((inner * getSine(angle)) / 90);
    y_arrow = y + ((inner * getCosine(angle)) / 90);
    tail1x = x + ((outer * getSine(angle + 7)) / 100);
    tail1y = y + ((outer * getCosine(angle + 7)) / 100);
    tail2x = x + ((outer * getSine(angle - 7)) / 100);
    tail2y = y + ((outer * getCosine(angle - 7)) / 100);
  }

  gfx->fillTriangle(x_offset, y_offset, tail1x, tail1y, tail2x, tail2y, COLOUR);
  if (COLOUR != BLACK) {
    gfx->drawLine(x_arrow, y_arrow, (tail1x + tail2x) / 2, (tail1y + tail2y) / 2, WHITE);
  } else {
    gfx->drawLine(x_arrow, y_arrow, (tail1x + tail2x) / 2, (tail1y + tail2y) / 2, COLOUR);
  }
}

void WindArrow(int x, int y, int rad, int wind, bool to) {
  static int lastwind;
  drawCompassPointer(x, y, rad, lastwind, BLACK, to);
  drawCompassPointer(x, y, rad, wind, RED, to);
  lastwind = wind;
}

void ShowToplinesettings(MySettings A, String Text) {
  int local;
  local = MasterFont;
  setFont(0);  // SETS MasterFont, so cannot use MasterFont directly in last line!
  long rssiValue = WiFi.RSSI();
  gfx->setTextSize(1);
  GFXBoxPrintf(0, 0, 1, "%s: Serial<%s> UDP<%s> ESP<%s>", Text, A.Serial_on On_Off, A.UDP_ON On_Off, A.ESP_NOW_ON On_Off);
  GFXBoxPrintf(0, text_height, 1, " SSID<%s> PWD<%s> UDPPORT<%s>", A.ssid, A.password, A.UDP_PORT);
  gfx->fillRect(0, text_height * 2, 480, text_height, WHITE);  // manual equivalent of my function so I can print IP address!
  gfx->setTextColor(BLACK);
  gfx->setCursor(0, (text_height * 2) + (text_offset));
  // gfx->print("this is Page:<");
  // gfx->print(Display_Page);
  gfx->print("   IP:");
  sta_ip = WiFi.localIP();
  udp_st = Get_UDP_IP(sta_ip, subnet);
  gfx->print(WiFi.localIP());
  // Serial2.print("   UDP broadcasting: ");  // for if we wish to send UDP data (later! if I add autopilot controls)
  //Serial2.println(udp_st);
  gfx->print(" RSSI: ");
  gfx->print(rssiValue);

  setFont(local);
}
void ShowToplinesettings(String Text) {
  ShowToplinesettings(Current_Settings, Text);
}

void Display(int page) {  // setups for alternate pages to be selected by page.
  static int LastPageselected;
  static bool DataChanged;
  static int wifissidpointer;
  // some local variables for tests;
  char blank[] = "null";
  static int SwipeTestLR, SwipeTestUD, volume;
  static bool RunSetup;
  static unsigned int slowdown, timer2;
  static float wind, SOG, Depth;
  float temp, oldtemp;

  static int fontlocal;
  static int FileIndex, Playing;  // static to hold after selection and before pressing play!
  static int V_offset;            // used in the audio file selection to sort print area
  char Tempchar[30];
  String tempstring;
  int FS = 1;  // for font size test
  int tempint;
  if (page != LastPageselected) { RunSetup = true; }
  //generic setup stuff for ALL pages
  if (RunSetup) {
    gfx->fillScreen(BLUE);
    gfx->setTextColor(WHITE);
    setFont(0);
  }
  // add any generic stuff here

  // Now specific stuff for each page

  switch (page) {  // just show the top page
    case -200:
      if (RunSetup) { jpegDraw("/logo.jpg", jpegDrawCallback, true /* useBigEndian */,
                               0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */); }

      if (CheckButton(Full0Center)) { Display_Page = 0; }
      if (CheckButton(Full1Center)) { jpegDraw("/logo.jpg", jpegDrawCallback, true /* useBigEndian */,
                                               0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */); }
      if (CheckButton(Full1Center)) { jpegDraw("/logo1.jpg", jpegDrawCallback, true /* useBigEndian */,
                                               0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */); }
      if (CheckButton(Full2Center)) { jpegDraw("/logo4.jpg", jpegDrawCallback, false /* useBigEndian */,
                                               0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */); }
      if (CheckButton(Full3Center)) { jpegDraw("/logo2.jpg", jpegDrawCallback, true /* useBigEndian */,
                                               0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */); }
      if (CheckButton(Full4Center)) { jpegDraw("/logo4.jpg", jpegDrawCallback, true /* useBigEndian */,
                                               0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */); }
      if (CheckButton(Full4Center)) { jpegDraw("/logo4.jpg", jpegDrawCallback, true /* useBigEndian */,
                                               0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */); }
      if (CheckButton(Full4Center)) { jpegDraw("/logo4.jpg", jpegDrawCallback, true /* useBigEndian */,
                                               0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */); }
      break;
    case -101:  //a test for Swipes
      if (RunSetup) {
        gfx->fillScreen(BLACK);
        gfx->setTextColor(WHITE);
        SwipeTestLR = 0;
        SwipeTestUD = 0;
        setFont(0);
        GFXBoxPrintf(0, 250, 3, "-swipe test- ");
      }

      if (millis() >= slowdown + 10000) {
        slowdown = millis();
        gfx->fillScreen(BLACK);
        GFXBoxPrintf(0, 50, 2, "Periodic blank ");
      }

      break;
    case -99:  //a test for Screen Colours / fonts
      if (RunSetup) {
        gfx->fillScreen(BLACK);
        gfx->setTextColor(WHITE);
        fontlocal = 0;
        SwipeTestLR = 0;
        SwipeTestUD = 0;
        setFont(fontlocal);
        GFXBoxPrintf(0, 250, 3, "-TEST Colours- ");
      }

      if (millis() >= slowdown + 10000) {
        slowdown = millis();
        gfx->fillScreen(BLACK);
        fontlocal = fontlocal + 1;
        if (fontlocal > 10) { fontlocal = 0; }
        GFXBoxPrintf(0, 50, 2, "Colours check");
        GFXBoxPrintf(0, 100, FS, "Font size %i", fontlocal);
        //DisplayCheck(true);
      }



      break;

    case -102:
      if (RunSetup) {
        GFXBoxPrintf(0, 140, 2, WHITE, BLUE, " Testing new buttons\n");
        //gfx->println(" use three finger touch to renew dir listing");
      }
      if (millis() >= slowdown + 10000) {
        slowdown = millis();
        gfx->fillScreen(BLUE);  // clean up redraw buttons
      }
      if (ts.isTouched) {
        TouchCrosshair(20);
      }

      if (CheckButton(Full0Center)) { Display_Page = 0; }

      break;
    case -10:  // a test page for fonts
      if (RunSetup) {
        GFXBorderBoxPrintf(Full0Center, "-Font test -");
      }
      if (millis() > slowdown + 3500) {
        slowdown = millis();
        gfx->fillScreen(BLUE);
        //wind=wind+9; if (wind>360) {wind=0;gfx->fillScreen(BLUE);}
        fontlocal = fontlocal + 1;
        if (fontlocal > 15) { fontlocal = 0; }
        //setFont(0);
        // ShowToplinesettings("Current");
        temp = 12.3;
        Serial.print("about to set");
        Serial.print(fontlocal);
        setFont(fontlocal);
        Serial.print(" setting fontname");
        Serial.println(Fontname);
      }
      if (millis() > timer2 + 500) {
        timer2 = millis();
        temp = random(-9000, 9000);
        temp = temp / 1000;
        setFont(fontlocal);
        Fontname.toCharArray(Tempchar, 30, 0);
        setFont(3);

        GFXBorderBoxPrintf(Fontdescriber, "FONT:%i name%s", fontlocal, Tempchar);
        setFont(fontlocal);
        GFXBorderBoxPrintf(FontBox, "Test %4.2f height<%i>", temp, text_height);
      }
      if (CheckButton(Full0Center)) { Display_Page = 0; }

      if (CheckButton(BottomLeftbutton)) { fontlocal = fontlocal - 1; }
      if (CheckButton(BottomRightbutton)) { fontlocal = fontlocal + 1; }
      break;

    case -9:  // Play with the audio ..NOTE  Needs the resistors resoldered to connect to the audio chip on the Guitron (mains relay type) version!!
      if (RunSetup || DataChanged) {
        setFont(4);
        gfx->setTextColor(WHITE, BLUE);
        gfx->setTextSize(1);
        if (volume > 21) { volume = 21; };
        GFXBorderBoxPrintf(TOPButton, "Main Menu");
        GFXBorderBoxPrintf(SecondRowButton, "PLAY Audio vol %i", volume);
        GFXBorderBoxPrintf(BottomLeftbutton, "vol-");
        GFXBorderBoxPrintf(BottomRightbutton, "vol+");
        //gfx->setCursor(0, 80);

        int file_num;                                                                                           // = get_music_list(SD, "/", 0, file_list);  // see https://github.com/VolosR/MakePythonLCDMP3/blob/main/MakePythonLCDMP3.ino#L101
        file_num = get_music_list(SD, "/", 0);                                                                  // Char array  based version of  https://github.com/VolosR/MakePythonLCDMP3/blob/main/MakePythonLCDMP3.ino#L101
        V_offset = text_height + (ThirdRowButton.v + ThirdRowButton.height + (3 * ThirdRowButton.bordersize));  // start below the third button
        //  listDir(SD, "/", 0);
        int localy = 0;
        for (int i = 0; i < file_num; i++) {
          gfx->setCursor(0, V_offset + localy);
          gfx->print("   ");
          gfx->println(File_List[i]);
          Serial.println(File_List[i]);
          localy = localy + text_height;
        }
        GFXBorderBoxPrintf(ThirdRowButton, "Found %i files", file_num);
        DataChanged = false;
      }

      if (millis() > slowdown + 1000) {
        slowdown = millis();
        //DataChanged = false;
        //periodic updates.. other stuff?
        if (audio.isRunning()) {
          GFXBorderBoxPrintf(SecondRowButton, "Playing:%s", File_List[Playing]);
        } else {
          GFXBorderBoxPrintf(SecondRowButton, "PLAY Audio vol %i", volume);
        }
      }
      if ((ts.isTouched) && (ts.points[0].y >= V_offset) && (ts.points[0].y <= BottomLeftbutton.v)) {  // nb check on location on screen or it will get reset when you press one of the boxes
        //TouchCrosshair(1); // to help debugging only!
        FileIndex = ((ts.points[0].y - V_offset) / text_height) - 1;
        // Serial.printf(" pointing at %i %s\n", FileIndex, File_List[FileIndex]);
        GFXBorderBoxPrintf(ThirdRowButton, "%s", File_List[FileIndex]);
      }

      if (CheckButton(SecondRowButton)) {
        Playing = FileIndex;
        volume = 8;
        open_new_song(File_List[Playing]);
        GFXBorderBoxPrintf(SecondRowButton, "Playing:%s", File_List[Playing]);
      }

      if (CheckButton(TOPButton)) { Display_Page = 0; }
      if (CheckButton(TopLeftbutton)) { Display_Page = 0; }
      if (CheckButton(TopRightbutton)) { Display_Page = 0; }
      if (CheckButton(BottomRightbutton)) {
        volume = volume + 1;
        if (volume > 21) { volume = 21; };
        audio.setVolume(volume);
        DataChanged = true;
      }
      if (CheckButton(BottomLeftbutton)) {
        volume = volume - 1;
        if (volume < 1) { volume = 0; };
        audio.setVolume(volume);
        DataChanged = true;
      }


      break;

    case -5:  ///Wifiscan

      if (RunSetup || DataChanged) {
        setFont(4);
        GFXBorderBoxPrintf(TopRightbutton, "reScan");
        if (IsConnected) {
          GFXBorderBoxPrintf(TOPButton, "Connected<%s>", Current_Settings.ssid);
        } else {
          GFXBorderBoxPrintf(TOPButton, "look for:%s", Current_Settings.ssid);
        }
        GFXBorderBoxPrintf(SecondRowButton, "SCANNING...");
        gfx->setCursor(0, 140);  //(location of terminal. make better later!)
        Serial.println(" starting WiFi.Scan");
        //(bool async, bool show_hidden, bool passive, uint32_t max_ms_per_chan, uint8_t channel, const char * ssid, const uint8_t * bssid)

        // gfx->println(" Starting WiFi SCAN");
        // NetworksFound = WiFi.scanNetworks(false, false, true, 250, 0, nullptr, nullptr);
        GFXBorderBoxPrintf(SecondRowButton, "scan done");
        if (NetworksFound <= 0) {  // note scan error can give negative number
          NetworksFound = 0;
          GFXBorderBoxPrintf(SecondRowButton, " Use keyboard ");
        } else {
          GFXBorderBoxPrintf(SecondRowButton, " %i Networks Found", NetworksFound);
          gfx->fillRect(0, 200, 480, 280, BLUE);  // clear the place wherethe wifi wil be printed
          for (int i = 0; i < NetworksFound; ++i) {
            gfx->setCursor(0, 200 + ((i + 1) * text_height));
            // Print SSID and RSSI for each network found
            gfx->print(i + 1);
            gfx->print(": ");
            //if (WiFi.SSID(i).length() > 20) { gfx->print("..(toolong).."); }
            gfx->print(WiFi.SSID(i).substring(0, 20));
            if (WiFi.SSID(i).length() > 20) { gfx->print(".."); }
            Serial.println(WiFi.SSID(i));
            gfx->print(" (");
            gfx->print(WiFi.RSSI(i));
            gfx->println(")");
            //    gfx->println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
            delay(10);
          }
        }
        DataChanged = false;
      }

      if (millis() > slowdown + 1000) {
        slowdown = millis();
        //other stuff?
      }

      if ((ts.isTouched) && (ts.points[0].y >= 200)) {  // nb check on location on screen or it will get reset when you press one of the boxes
        //TouchCrosshair(1);
        wifissidpointer = ((ts.points[0].y - 200) / text_height) - 1;
        int str_len = WiFi.SSID(wifissidpointer).length() + 1;
        char result[str_len];
        Serial.printf(" touched at %i  equates to %i ? %s ", ts.points[0].y, wifissidpointer, WiFi.SSID(wifissidpointer));
        Serial.printf("  result str_len%i   sizeof settings.ssid%i \n", str_len, sizeof(Current_Settings.ssid));
        if (str_len <= sizeof(Current_Settings.ssid)) {                                       // check small enough for our ssid register array!
          WiFi.SSID(wifissidpointer).toCharArray(result, sizeof(Current_Settings.ssid) - 1);  // I like to keep a spare space!
          //  WiFi.SSID(wifissidpointer).toCharArray(Current_Settings.ssid,str_len);
          if (str_len == 1) {
            GFXBorderBoxPrintf(SecondRowButton, "Set via Keyboard?");
          } else {
            GFXBorderBoxPrintf(SecondRowButton, "Select<%s>?", result);
          }
        } else {
          GFXBorderBoxPrintf(SecondRowButton, "ssid too long ");
        }
      }
      if (CheckButton(TopRightbutton)) {
        GFXBorderBoxPrintf(SecondRowButton, " Starting WiFi re-SCAN ");
        NetworksFound = WiFi.scanNetworks(false, false, true, 250, 0, nullptr, nullptr);
        DataChanged = true;
      }  // do the scan again
      if (CheckButton(SecondRowButton)) {
        if ((NetworksFound >= 1) && (wifissidpointer >= 1)) {
          WiFi.SSID(wifissidpointer).toCharArray(Current_Settings.ssid, sizeof(Current_Settings.ssid) - 1);
          Serial.printf(" Update ssid to <%s>", Current_Settings.ssid);
          Display_Page = -1;
        } else {
          Serial.println(" Update ssid via keyboard");
          Display_Page = -2;
        }
      }
      if (CheckButton(TOPButton)) { Display_Page = -1; }

      break;

    case -4:
      if (RunSetup) {
        GFXBorderBoxPrintf(TOPButton, "Current <%s>", Current_Settings.UDP_PORT);
        GFXBorderBoxPrintf(Full0Center, "Set UDP PORT");
        keyboard(-1);  //reset
        keyboard(0);
        Use_Keyboard(blank, sizeof(blank));
        Use_Keyboard(Current_Settings.UDP_PORT, sizeof(Current_Settings.UDP_PORT));
      }
      Use_Keyboard(Current_Settings.UDP_PORT, sizeof(Current_Settings.UDP_PORT));
      if (CheckButton(Full0Center)) { Display_Page = -1; }
      break;

    case -3:
      if (RunSetup) {
        GFXBorderBoxPrintf(TOPButton, "Current <%s>", Current_Settings.password);
        GFXBorderBoxPrintf(Full0Center, "Set Password");
        keyboard(-1);  //reset
        Use_Keyboard(Current_Settings.password, sizeof(Current_Settings.password));
        keyboard(0);
      }
      Use_Keyboard(Current_Settings.password, sizeof(Current_Settings.password));
      if (CheckButton(Full0Center)) { Display_Page = -1; }
      break;

    case -2:
      if (RunSetup) {

        GFXBorderBoxPrintf(Full0Center, "Set SSID");
        GFXBorderBoxPrintf(TopRightbutton, "Scan");
        AddTitleBorderBox(TopRightbutton, "WiFi");
        keyboard(-1);  //reset
        Use_Keyboard(Current_Settings.ssid, sizeof(Current_Settings.ssid));
        keyboard(0);
        //Terminal for show ssids?
      }

      Use_Keyboard(Current_Settings.ssid, sizeof(Current_Settings.ssid));
      if (CheckButton(Full0Center)) { Display_Page = -1; }
      if (CheckButton(TopRightbutton)) { Display_Page = -5; }
      break;

    case -1:  // this is the main WIFI settings page
      if (RunSetup || DataChanged) {
        gfx->fillScreen(BLACK);
        gfx->setTextColor(BLACK);
        gfx->setTextSize(1);
        ShowToplinesettings(Saved_Settings, "SAVED");
        setFont(4);
        gfx->setCursor(180, 180);
        GFXBorderBoxPrintf(SecondRowButton, "Set SSID <%s>", Current_Settings.ssid);
        GFXBorderBoxPrintf(ThirdRowButton, "Set Password <%s>", Current_Settings.password);
        GFXBorderBoxPrintf(FourthRowButton, "Set UDP Port <%s>", Current_Settings.UDP_PORT);
        GFXBorderBoxPrintf(Switch1, Current_Settings.Serial_on On_Off);  //A.Serial_on On_Off,  A.UDP_ON On_Off, A.ESP_NOW_ON On_Off
        AddTitleBorderBox(Switch1, "Serial");
        GFXBorderBoxPrintf(Switch2, Current_Settings.UDP_ON On_Off);
        AddTitleBorderBox(Switch2, "UDP");
        GFXBorderBoxPrintf(Switch3, Current_Settings.ESP_NOW_ON On_Off);
        AddTitleBorderBox(Switch3, "ESP-Now");
        GFXBorderBoxPrintf(Switch4, "UPDATE");
        AddTitleBorderBox(Switch4, "EEPROM");
        GFXBorderBoxPrintf(Terminal, "- NMEA DATA -");
        AddTitleBorderBox(Terminal, "TERMINAL");
        setFont(0);
        DataChanged = false;
        //while (ts.sTouched{yield(); Serial.println("yeilding -1");}
      }
      if (millis() > slowdown + 1000) {
        slowdown = millis();

        //print recieved data in terminal here? - or in UpdateLinef()
      }

      if (CheckButton(Terminal)) {
        debugpause = !debugpause;
        if (!debugpause) {
          AddTitleBorderBox(Terminal, "TERMINAL");
        } else {
          AddTitleBorderBox(Terminal, "-Paused-");
        }
      }
      //runsetup to repopulate the text in the boxes!
      if (CheckButton(Switch1)) {
        Current_Settings.Serial_on = !Current_Settings.Serial_on;
        DataChanged = true;
      };
      if (CheckButton(Switch2)) {
        Current_Settings.UDP_ON = !Current_Settings.UDP_ON;
        DataChanged = true;
      };
      if (CheckButton(Switch3)) {
        Current_Settings.ESP_NOW_ON = !Current_Settings.ESP_NOW_ON;
        DataChanged = true;
      };
      if (CheckButton(Switch4)) {
        Display_Page = 0;
        ;
      };


      if (CheckButton(TOPButton)) { Display_Page = 0; }
      if (CheckButton(Full0Center)) { Display_Page = 0; }
      if (CheckButton(SecondRowButton)) { Display_Page = -5; };
      if (CheckButton(ThirdRowButton)) { Display_Page = -3; };
      if (CheckButton(FourthRowButton)) { Display_Page = -4; };

      break;
    case 0:  // main settings
      if (RunSetup) {

        ShowToplinesettings("Now");
        setFont(3);
        // GFXBorderBoxPrintf(TopLeftbutton, "Page-");
        // GFXBorderBoxPrintf(TopRightbutton, "Page+");
        setFont(3);
        GFXBorderBoxPrintf(Full0Center, "-Top page-");
        GFXBorderBoxPrintf(Full1Center, "Check SD /Audio");
        GFXBorderBoxPrintf(Full2Center, "Settings  WIFI etc");
        GFXBorderBoxPrintf(Full3Center, "See NMEA");
        GFXBorderBoxPrintf(Full4Center, "Check Fonts");
        GFXBorderBoxPrintf(Full5Center, "Save / Reset to Quad NMEA");
      }
      if (millis() > slowdown + 500) {
        slowdown = millis();
        // ShowToplinesettings("Current");
        //ShowToplinesettings("Current");
      }
      if (CheckButton(Full0Center)) { Display_Page = 0; }
      if (CheckButton(Full1Center)) { Display_Page = -9; }
      if (CheckButton(Full2Center)) { Display_Page = -1; }
      if (CheckButton(Full3Center)) { Display_Page = 4; }
      if (CheckButton(Full4Center)) { Display_Page = -10; }
      if (CheckButton(Full5Center)) {
        Display_Page = 4;
        EEPROM_WRITE(Current_Settings);
        delay(50);
        ESP.restart();
      }

      break;




    case 4:  // Quad display
      if (RunSetup) {
        setFont(5);
        //GFXBoxPrintf(0,480-(text_height*2),  1,BLACK,BLUE, "quad display");
        gfx->fillScreen(BLACK);
        DrawCompass(360, 120, 120);
        GFXBorderBoxPrintf(topLeftquarter, "STW");
        AddTitleBorderBox(topLeftquarter, "STW");
        // AddTitleBorderBox(topLeftquarter, "SPEED");
        GFXBorderBoxPrintf(bottomLeftquarter, "DEPTH");
        AddTitleBorderBox(bottomLeftquarter, "DEPTH");
        // AddTitleBorderBox(bottomLeftquarter, "DEPTH");
        //GFXBorderBoxPrintf(topRightquarter  "c%3.2F",wind*1.1);
        GFXBorderBoxPrintf(bottomRightquarter, "SOG");
        AddTitleBorderBox(bottomRightquarter, "SOG");
        //AddTitleBorderBox(bottomRightquarter, "SOG");

        delay(500);
      }
      if (millis() > slowdown + 300) {
        slowdown = millis();
        WindArrow(360, 120, 120, BoatData.WindDirectionT, false);
        UpdateCentered(topLeftquarter, "%3.0fkts", BoatData.STW);
        UpdateCentered(bottomLeftquarter, "%4.1fm", BoatData.WaterDepth);  // add units?
                                                                           //UpdateCentered(topRightquarter  "c%3.2F",wind*1.1);
        UpdateCentered(bottomRightquarter, "%3.0FKts", BoatData.SOG);
      }
      // if (CheckButton(Full0Center)) { Display_Page = 0; }  // first so it has priority!
      // if (CheckButton(Full1Center)) { Display_Page = 0; }  //easier to hit two
      //        TouchCrosshair(20); quarters select big screens
      if (CheckButton(topLeftquarter)) { Display_Page = 6; }
      if (CheckButton(bottomLeftquarter)) { Display_Page = 7; }
      if (CheckButton(topRightquarter)) { Display_Page = 5; }
      if (CheckButton(bottomRightquarter)) { Display_Page = 8; }


      // if (CheckButton(TopLeftbutton)) { Display_Page = Display_Page - 1; }
      // if (CheckButton(TopRightbutton)) { Display_Page = 1; }  //loop to page 1


      break;

    case 5:  // wind instrument
      if (RunSetup) {
        setFont(8);
        DrawCompass(240, 240, 240);
      }
      if (millis() > slowdown + 500) {
        slowdown = millis();
        GFXBorderBoxPrintf(WindSpeedBox, "%2.0fkts", BoatData.WindSpeedK);
        WindArrow(240, 240, 240, BoatData.WindDirectionT, false);
        //Box in middle for wind dir / speed
        //int h, int v, int width, int height, int textsize, int bordersize, uint16_t backgroundcol, uint16_t textcol, uint16_t BorderColor, const char* fmt, ...) {  //Print in a box.(h,v,width,height,textsize,bordersize,backgroundcol,textcol,BorderColor, const char* fmt, ...)
      }

      //        TouchCrosshair(20); quarters select big screens
      if (CheckButton(topLeftquarter)) { Display_Page = 4; }
      if (CheckButton(bottomLeftquarter)) { Display_Page = 4; }
      if (CheckButton(topRightquarter)) { Display_Page = 0; }
      if (CheckButton(bottomRightquarter)) { Display_Page = 4; }

      // if (CheckButton(TopLeftbutton)) { Display_Page = Display_Page - 1; }
      // if (CheckButton(TopRightbutton)) { Display_Page = 4; }  //loop to page 1


      break;
    case 6:
      if (RunSetup) {
        setFont(10);
        //GFXBoxPrintf(0,480-(text_height*2),  2, "PAGE 2 -SPEED");

        GFXBorderBoxPrintf(BigSingleDisplay, "STW");
        AddTitleBorderBox(20, 100, BLACK, "STW");
      }
      if (millis() > slowdown + 200) {
        //gfx->fillScreen(BLUE);
        slowdown = millis();
        // ShowToplinesettings("Current");

        UpdateCentered(BigSingleDisplay, "%4.2fkts", BoatData.STW);
      }
      //  if (CheckButton(Full0Center)) { Display_Page = 4; }
      //        TouchCrosshair(20); quarters select big screens
      if (CheckButton(topLeftquarter)) { Display_Page = 9; }
      if (CheckButton(bottomLeftquarter)) { Display_Page = 4; }
      if (CheckButton(topRightquarter)) { Display_Page = 4; }
      if (CheckButton(bottomRightquarter)) { Display_Page = 4; }

      break;
    case 7:
      if (RunSetup) {
        setFont(10);
        //GFXBoxPrintf(0,480-(text_height*2),  2, "PAGE 2 -SPEED");
        GFXBorderBoxPrintf(BigSingleDisplay, "DEPTH");
        AddTitleBorderBox(20, 100, BLACK, "DEPTH");
      }
      if (millis() > slowdown + 200) {
        //gfx->fillScreen(BLUE);
        slowdown = millis();
        UpdateCentered(BigSingleDisplay, "%4.1fm", BoatData.WaterDepth);
      }
      if (CheckButton(Full0Center)) { Display_Page = 4; }
      //        TouchCrosshair(20); quarters select big screens
      if (CheckButton(topLeftquarter)) { Display_Page = 4; }
      if (CheckButton(bottomLeftquarter)) { Display_Page = 11; }
      if (CheckButton(topRightquarter)) { Display_Page = 4; }
      if (CheckButton(bottomRightquarter)) { Display_Page = 4; }

      break;
    case 8:
      if (RunSetup) {
        setFont(10);

        GFXBorderBoxPrintf(BigSingleDisplay, "SOG");
        AddTitleBorderBox(20, 100, BLACK, "SOG");
      }
      if (millis() > slowdown + 200) {
        //gfx->fillScreen(BLUE);
        slowdown = millis();
        UpdateCentered(BigSingleDisplay, "%3.1Fkts", BoatData.SOG);
      }
      //        TouchCrosshair(20); quarters select big screens
      if (CheckButton(topLeftquarter)) { Display_Page = 4; }
      if (CheckButton(bottomLeftquarter)) { Display_Page = 4; }
      if (CheckButton(topRightquarter)) { Display_Page = 4; }
      if (CheckButton(bottomRightquarter)) { Display_Page = 12; }

      break;

    case 9:  // GPS page
      if (RunSetup) {
        setFont(8);
        //GFXBorderBoxPrintf(BigSingleDisplay, "");

        AddTitleBorderBox(20, 130, BLACK, "GPS Time");
        AddTitleBorderBox(20, 230, BLACK, "LATITUDE DD.ddd");
        AddTitleBorderBox(20, 330, BLACK, "LONGITUDE DD.ddd");
      }
      if (millis() > slowdown + 200) {
        slowdown = millis();
        UpdateCentered(Threelines0, "%.0f Satellites in view", BoatData.SatsInView);
        if (BoatData.GPSTime != NMEA0183DoubleNA) { UpdateCentered(Threelines1, "%02i:%02i:%02i",
                                                                   int(BoatData.GPSTime) / 3600, (int(BoatData.GPSTime) % 3600) / 60, (int(BoatData.GPSTime) % 3600) % 60); }
        if (BoatData.Latitude != NMEA0183DoubleNA) { UpdateCentered(Threelines2, "%f", BoatData.Latitude); }
        if (BoatData.Longitude != NMEA0183DoubleNA) { UpdateCentered(Threelines3, "%f", BoatData.Longitude); }
      }
      //if (CheckButton(Full0Center)) { Display_Page = 4; }
      if (CheckButton(topLeftquarter)) { Display_Page = 10; }
      if (CheckButton(bottomLeftquarter)) { Display_Page = 4; }  //Loop to the main settings page
      if (CheckButton(topRightquarter)) { Display_Page = 4; }
      if (CheckButton(bottomRightquarter)) { Display_Page = 4; }  // test! check default loops back to 0

      break;

    default:
      if (RunSetup) {
        setFont(3);  //GFXBorderBoxPrintf(int h, int v, int width, int height, int textsize, int bordersize,
        //uint16_t backgroundcol, uint16_t textcol, uint16_t BorderColor, const char* fmt, ...) {  //Print in a box.(h,v,width,height,textsize,bordersize,backgroundcol,textcol,BorderColor, const char* fmt, ...)
        GFXBorderBoxPrintf(Full0Center, "-Top page-");
        GFXBorderBoxPrintf(TopLeftbutton, "Page-");
        GFXBorderBoxPrintf(TopRightbutton, "Page+");

        GFXBorderBoxPrintf(Full1Center, "Check SD");
        GFXBorderBoxPrintf(Full2Center, "Set WIFI");
        GFXBorderBoxPrintf(Full3Center, "See NMEA");
        GFXBorderBoxPrintf(Full4Center, "Check Fonts");
      }
      if (millis() > slowdown + 1000) {
        slowdown = millis();
      }
      Display_Page = 0;

      break;
  }
  LastPageselected = page;
  RunSetup = false;
}

void setFont(int fontinput) {
  MasterFont = fontinput;
  gfx->setTextSize(1);
  switch (fontinput) {  //select font and automatically set height/offset based on character '['
    // set the heights and offset to print [ in boxes. Heights in pixels are NOT the point heights!
    case 0:  // SMALL 8pt
      Fontname = "FreeMono8pt7b";
      gfx->setFont(&FreeMono8pt7b);
      text_height = (FreeMono8pt7bGlyphs[0x3D].height) + 1;
      text_offset = -(FreeMono8pt7bGlyphs[0x3D].yOffset);
      text_char_width = 12;
      break;
    case 1:  // standard 12pt
      Fontname = "FreeMono12pt7b";
      gfx->setFont(&FreeMono12pt7b);
      text_height = (FreeMono12pt7bGlyphs[0x3D].height) + 1;
      text_offset = -(FreeMono12pt7bGlyphs[0x3D].yOffset);
      text_char_width = 12;

      break;
    case 2:  //standard 18pt
      Fontname = "FreeMono18pt7b";
      gfx->setFont(&FreeMono18pt7b);
      text_height = (FreeMono18pt7bGlyphs[0x3D].height) + 1;
      text_offset = -(FreeMono18pt7bGlyphs[0x3D].yOffset);
      text_char_width = 12;

      break;
    case 3:  //BOLD 8pt
      Fontname = "FreeMonoBOLD8pt7b";
      gfx->setFont(&FreeMonoBold8pt7b);
      text_height = (FreeMonoBold8pt7bGlyphs[0x3D].height) + 1;
      text_offset = -(FreeMonoBold8pt7bGlyphs[0x3D].yOffset);
      text_char_width = 12;

      break;
    case 4:  //BOLD 12pt
      Fontname = "FreeMonoBOLD12pt7b";
      gfx->setFont(&FreeMonoBold12pt7b);
      text_height = (FreeMonoBold12pt7bGlyphs[0x3D].height) + 1;
      text_offset = -(FreeMonoBold12pt7bGlyphs[0x3D].yOffset);
      text_char_width = 12;

      break;
    case 5:  //BOLD 18 pt
      Fontname = "FreeMonoBold18pt7b";
      gfx->setFont(&FreeMonoBold18pt7b);
      text_height = (FreeMonoBold18pt7bGlyphs[0x3D].height) + 1;
      text_offset = -(FreeMonoBold18pt7bGlyphs[0x3D].yOffset);
      text_char_width = 12;
      break;
    case 6:  //BOLD 27 pt
      Fontname = "FreeMonoBold27pt7b";
      gfx->setFont(&FreeMonoBold27pt7b);
      text_height = (FreeMonoBold27pt7bGlyphs[0x3D].height) + 1;
      text_offset = -(FreeMonoBold27pt7bGlyphs[0x3D].yOffset);
      text_char_width = 12;
      break;
    case 7:  //SANS BOLD 10 pt
      Fontname = "FreeSansBold10pt7b";
      gfx->setFont(&FreeSansBold10pt7b);
      text_height = (FreeSansBold10pt7bGlyphs[0x38].height) + 1;
      text_offset = -(FreeSansBold10pt7bGlyphs[0x38].yOffset);
      text_char_width = 12;
      break;
    case 8:  //SANS BOLD 18 pt
      Fontname = "FreeSansBold18pt7b";
      gfx->setFont(&FreeSansBold18pt7b);
      text_height = (FreeSansBold18pt7bGlyphs[0x38].height) + 1;
      text_offset = -(FreeSansBold18pt7bGlyphs[0x38].yOffset);
      text_char_width = 12;
      break;
    case 9:  //sans BOLD 30 pt
      Fontname = "FreeSansBold30pt7b";
      gfx->setFont(&FreeSansBold30pt7b);
      text_height = (FreeSansBold30pt7bGlyphs[0x38].height) + 1;
      text_offset = -(FreeSansBold30pt7bGlyphs[0x38].yOffset);
      text_char_width = 12;
      break;
    case 10:  //sans BOLD 50 pt
      Fontname = "FreeSansBold50pt7b";
      gfx->setFont(&FreeSansBold50pt7b);
      text_height = (FreeSansBold50pt7bGlyphs[0x38].height) + 1;
      text_offset = -(FreeSansBold50pt7bGlyphs[0x38].yOffset);
      text_char_width = 12;
      break;

    default:
      Fontname = "FreeMono8pt7b";
      gfx->setFont(&FreeMono8pt7b);
      text_height = (FreeMono8pt7bGlyphs[0x3D].height) + 1;
      text_offset = -(FreeMono8pt7bGlyphs[0x3D].yOffset);
      text_char_width = 12;
      MasterFont = 0;

      break;
  }
}

void setup() {
  _null = 0;
  _temp = 0;
  Serial.begin(115200);
  Serial.println("Test for NMEA Display ");
  Serial.println(" IDF will throw errors here as one pin is -1!");
  ts.begin();
  ts.setRotation(ROTATION_INVERTED);
#ifdef GFX_BL
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);
#endif
  // Init Display
  gfx->begin();
  //if GFX> 1.3.1 try and do this as the invert colours write 21h or 20h to 0Dh has been lost from the structure!
  gfx->invertDisplay(false);
  gfx->fillScreen(BLUE);
  gfx->setCursor(0, 110);

#ifdef GFX_BL
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);
#endif
  setFont(4);
  SD_Setup();
  Audio_setup();
  EEPROM_READ();  // setup and read Saved_Settings (saved variables)
  Current_Settings = Saved_Settings;
  ConnectWiFiusingCurrentSettings();
  SetupOTA();
  Start_ESP_EXT();  //  Sets esp_now links to the current WiFi.channel etc.
  keyboard(-1);     //reset keyboard display update settings
  gfx->println(F("***START Screen***"));
  Udp.begin(atoi(Current_Settings.UDP_PORT));
  delay(1500);       // 1.5 seconds
  Display_Page = 4;  // select first page to show or use non defined page to start with default
  gfx->setTextColor(WHITE);
  // gfx->setTextBound(0, 0, 480, 480); //reset or it wil cause issues later in other prints?
  Display(Display_Page);
}

void loop() {
  yield();
  server.handleClient();  // for OTA;
  //
  //DisplayCheck(false);
  //EventTiming("START");
  delay(1);
  ts.read();
  CheckAndUseInputs();
  Display(Display_Page);  //EventTiming("STOP");
  EXTHeartbeat();
  audio.loop();
  //EventTiming(" loop time touch sample display");
  //vTaskDelay(1);  // Audio is distorted without this?? used in https://github.com/schreibfaul1/ESP32-audioI2S/blob/master/examples/plays%20all%20files%20in%20a%20directory/plays_all_files_in_a_directory.ino
  // //.... (audio.isRunning()){   delay(100);gfx->println("Playing Ships bells"); Serial.println("Waiting for bells to finish!");}
}


void TouchCrosshair(int size) {
  for (int i = 0; i < (ts.touches); i++) {
    TouchCrosshair(i, size, WHITE);
  }
}
void TouchCrosshair(int point, int size, uint16_t colour) {
  gfx->setCursor(ts.points[point].x, ts.points[point].y);
  // gfx->printf("%i", point);
  gfx->drawFastVLine(ts.points[point].x, ts.points[point].y - size, 2 * size, colour);
  gfx->drawFastHLine(ts.points[point].x - size, ts.points[point].y, 2 * size, colour);
}

void DisplayCheck(bool invertcheck) {  //used to test colours are as expected while checking new dispolays
  static unsigned long timedInterval;
  static unsigned long updatetiming;
  static unsigned long LoopTime;
  static int Color;

  static bool ips;
  LoopTime = millis() - updatetiming;
  updatetiming = millis();
  if (millis() >= timedInterval) {
    //***** Timed updates *******
    timedInterval = millis() + 300;
    //Serial.printf("Loop Timing %ims",LoopTime);
    setFont(3);
    ScreenShow(Current_Settings, "Current");


    if (invertcheck) {
      //Serial.println(" Timed display check");
      Color = Color + 1;
      if (Color > 5) {
        Color = 0;
        ips = !ips;
        gfx->invertDisplay(ips);
      }
      gfx->fillRect(300, text_height, 180, text_height * 2, BLACK);
      gfx->setTextColor(WHITE);
      if (ips) {
        Writeat(310, text_height, "INVERTED");
      } else {
        Writeat(310, text_height, " Normal ");
      }

      switch (Color) {
        //gfx->fillRect(0, 00, 480, 20,BLACK);gfx->setTextColor(WHITE);Writeat(0,0, "WHITE");
        case 0:
          gfx->fillRect(300, 60, 180, 60, WHITE);
          gfx->setTextColor(BLACK);
          Writeat(310, 62, "WHITE");
          break;
        case 1:
          gfx->fillRect(300, 60, 180, 60, BLACK);
          ;
          gfx->setTextColor(WHITE);
          Writeat(310, 62, "BLACK");
          break;
        case 2:
          gfx->fillRect(300, 60, 180, 60, RED);
          ;
          gfx->setTextColor(BLACK);
          Writeat(310, 62, "RED");
          break;
        case 3:
          gfx->fillRect(300, 60, 180, 60, GREEN);
          ;
          gfx->setTextColor(BLACK);
          Writeat(310, 62, "GREEN");
          break;
        case 4:
          gfx->fillRect(300, 60, 180, 60, BLUE);
          ;
          gfx->setTextColor(BLACK);
          Writeat(310, 62, "BLUE");
          break;
      }
    }
  }
}


bool CheckButton(Button& button) {  // trigger on release. ?needs index (s) to remember which button!
  //trigger on release! does not sense !isTouched ..  use Keypressed in each button struct to keep track!
  if (ts.isTouched && !button.Keypressed && (millis() - button.LastDetect >= 250)) {
    if (XYinBox(ts.points[0].x, ts.points[0].y, button.h, button.v, button.width, button.height)) {
      //Serial.printf(" Checkbutton size%i state %i %i \n",ts.points[0].size,ts.isTouched,XYinBox(ts.points[0].x, ts.points[0].y,button.h,button.v,button.width,button.height));
      button.Keypressed = true;
      button.LastDetect = millis();
    }
    return false;
  }
  if (button.Keypressed && (millis() - button.LastDetect >= 250)) {
    //Serial.printf(" Checkbutton released from  %i %i\n",button.h,button.v);
    button.Keypressed = false;
    return true;
  }
  return false;
}


void TouchValueShow(int offset, bool debug) {  // offset display down in pixels
  //ts.read();
  if (ts.isTouched) {
    for (int i = 0; i < (ts.touches); i++) {
      if (debug) {
        Serial.printf("Touch [%i] X:%i Y:%i size:%i \n", i + 1, ts.points[i].x, ts.points[i].y, ts.points[i].size);
        GFXBoxPrintf(0, ((i * 10) + offset), "Touch [%i] X:%i Y:%i size:%i ", i + 1, ts.points[i].x, ts.points[i].y, ts.points[i].size);
      }
    }
  }
}

//*********** EEPROM functions *********
void EEPROM_WRITE(MySettings A) {
  // save my current settings
  // ALWAYS Write the Default display page!  may change this later and save separately?!!
  Serial.printf("SAVING EEPROM key:%i \n", A.EpromKEY);
  EEPROM.put(1, A.EpromKEY);  // separate and duplicated so it can be checked by itsself first in case structures change
  EEPROM.put(10, A);
  EEPROM.commit();
  delay(50);
}
void EEPROM_READ() {
  int key;
  EEPROM.begin(512);
  Serial.println("READING EEPROM");
  gfx->println(" EEPROM READING ");
  EEPROM.get(1, key);
  Serial.printf(" read %i  default %i \n", key, Default_Settings.EpromKEY);
  if (key == Default_Settings.EpromKEY) {
    EEPROM.get(10, Saved_Settings);
    Serial.println("Using EEPROM settings");
    gfx->println("Using EEPROM settings");
  } else {
    Saved_Settings = Default_Settings;
    gfx->println("Using DEFAULTS");
    Serial.println("Using DEFAULTS");
    EEPROM_WRITE(Default_Settings);
  }
}

//************** display housekeeping ************
void ScreenShow(MySettings A, String Text) {
  long rssiValue = WiFi.RSSI();
  GFXBoxPrintf(0, 0, 1, "%s: Seron<%s>UDPon<%s> ESPon<%s>", Text, A.Serial_on On_Off, A.UDP_ON On_Off, A.ESP_NOW_ON On_Off);
  GFXBoxPrintf(0, text_height, 1, "Wifi:  SSID<%s> PWD<%s> UDPPORT<%s>  rssi<%i> ", A.ssid, A.password, A.UDP_PORT, rssiValue);
}

void dataline(MySettings A, String Text) {
  ScreenShow(A, Text);
  Serial.printf("%d Dataline display %s: Ser<%d> UDPPORT<%d> UDP<%d>  ESP<%d> \n ", A.EpromKEY, Text, A.Serial_on, A.UDP_PORT, A.UDP_ON, A.ESP_NOW_ON);
  Serial.print("SSID <");
  Serial.print(A.ssid);
  Serial.print(">  Password <");
  Serial.print(A.password);
  Serial.println("> ");
}
boolean CompStruct(MySettings A, MySettings B) {  // Does NOT compare the display page!
  bool same = false;
  // have to check each variable individually
  if (A.EpromKEY == B.EpromKEY) { same = true; }
  if (A.UDP_PORT == B.UDP_PORT) { same = true; }
  if (A.UDP_ON == B.UDP_ON) { same = true; }
  if (A.ESP_NOW_ON == B.ESP_NOW_ON) { same = true; }
  if (A.Serial_on == B.Serial_on) { same = true; }
  if (A.ssid == B.ssid) { same = true; }
  if (A.password == B.password) { same = true; }
  return same;
}

//*************** Not tidied up yet!! *************************
void Writeat(int h, int v, const char* text) {  //Write text at h,v (using fontoffset to use TOP LEFT of text convention)
  gfx->setCursor(h, v + (text_offset));         // offset up/down for GFXFONTS that start at Bottom left. Standard fonts start at TOP LEFT
  gfx->print(text);
  gfx->setCursor(h, v + (text_offset));
}
// void Writeat(int h, int v, const char* text) {
//   Writeat(h, v, text);
// }
// try out void getTextBounds(const char *string, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h);
void WriteinBox(int h, int v, int size, const char* TEXT, uint16_t TextColor, uint16_t BackColor) {  //Write text in filled box of text height at h,v (using fontoffset to use TOP LEFT of text convention)
  int width, height;

  gfx->fillRect(h, v, 480, text_height * size, BackColor);
  gfx->setTextColor(TextColor);
  gfx->setTextSize(size);
  Writeat(h, v, TEXT);  // text offset is dealt with in write at
  gfx->setCursor(h, v + (text_offset));
}
void WriteinBox(int h, int v, int size, const char* TEXT) {  //Write text in filled box of text height at h,v (using fontoffset to use TOP LEFT of text convention)
  gfx->fillRect(h, v, 480, text_height * size, WHITE);
  gfx->setTextColor(BLACK);
  gfx->setTextSize(size);
  Writeat(h, v, TEXT);  // text offset is dealt with in write at
  gfx->setCursor(h, v + (text_offset));
}


void GFXBoxPrintf(int h, int v, int size, uint16_t TextColor, uint16_t BackColor, const char* fmt, ...) {  //complete object type suitable for holding the information needed by the macros va_start, va_copy, va_arg, and va_end.
  char msg[300] = { '\0' };                                                                                // used in message buildup
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, 128, fmt, args);
  va_end(args);
  int len = strlen(msg);
  WriteinBox(h, v, size, msg, TextColor, BackColor);
  gfx->setCursor(h, v + (text_offset));
}


void GFXBoxPrintf(int h, int v, const char* fmt, ...) {  //complete object type suitable for holding the information needed by the macros va_start, va_copy, va_arg, and va_end.
  char msg[300] = { '\0' };                              // used in message buildup
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, 128, fmt, args);
  va_end(args);
  int len = strlen(msg);
  WriteinBox(h, v, 1, msg);
}
void GFXBoxPrintf(int h, int v, int size, const char* fmt, ...) {  //complete object type suitable for holding the information needed by the macros va_start, va_copy, va_arg, and va_end.
  static char msg[300] = { '\0' };                                 // used in message buildup
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, 128, fmt, args);
  va_end(args);
  int len = strlen(msg);
  WriteinBox(h, v, size, msg);  // includes font size
}


void GFXPrintf(int h, int v, const char* fmt, ...) {  //complete object type suitable for holding the information needed by the macros va_start, va_copy, va_arg, and va_end.
  char msg[300] = { '\0' };                           // used in message buildup
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, 128, fmt, args);
  va_end(args);
  int len = strlen(msg);
  Writeat(h, v, msg);
}


void GetGPSPos(char* str, char* NMEAgpspos, uint8_t sign) {
  unsigned short int u = 0, d = 0;
  unsigned int minutes;
  unsigned char pos, i, j;
  for (pos = 0; pos < strlen(NMEAgpspos) && NMEAgpspos[pos] != '.'; pos++)
    ;
  for (i = 0; i < pos - 2; i++) {
    u *= 10;
    u += NMEAgpspos[i] - '0';
  }
  d = (NMEAgpspos[pos - 2] - '0') * 10;
  d += (NMEAgpspos[pos - 1] - '0');
  for (i = pos + 1, j = 0; i < strlen(NMEAgpspos) && j < 4; i++, j++)  //Only 4 chars
  {
    d *= 10;
    d += NMEAgpspos[i] - '0';
  }
  minutes = d / 60;
  gfx->printf(str, "%d.%04d", (sign ? -1 : 1) * u, minutes);
}

int yforLine(Button button, int printline) {
  int y = button.v + button.bordersize;
  y = y + printline * (text_height + 2);
  return y;
}

void UpdateLinef(Button& button, const char* fmt, ...) {  // Types sequential lines in the button space '&' for button to store printline?
  //static int button.PrintLine; // place in button so its static for each button!
  int LinesOfType, characters;
  static bool screenfull;
  char limitedWidth[80];
  int16_t x, y, TBx1, TBy1;
  uint16_t TBw, TBh;
  int typingspaceH, typingspaceW;
  if (screenfull && debugpause) { return; }
  typingspaceH = button.height - (2 * button.bordersize);
  typingspaceW = button.width - (2 * button.bordersize);
  LinesOfType = typingspaceH / (text_height + 2);  //assumes textsize 1?
  static char msg[300] = { '\0' };
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, 128, fmt, args);
  va_end(args);
  int len = strlen(msg);
  x = button.h + button.bordersize;  // shift inwards for border
  gfx->setTextBound(button.h + button.bordersize, button.v + button.bordersize, typingspaceW, typingspaceH);
  gfx->setTextWrap(true);
  y=yforLine(button, button.PrintLine); //Needed for the text bounds?
  gfx->getTextBounds(msg, x, y, &TBx1, &TBy1, &TBw, &TBh);
 // if (TBh/text_height) {  // calculate how many lines?? 
 //   Serial.printf(" x %i y%i &TBx1 %i, &TBy1 %i, &TBw %i, &TBh %i \n",x,y, TBx1, TBy1, TBw, TBh);
    //tbh is always the full height of the box! 
 // not needed as clear on full screen is simpler  gfx->fillRect(x, yforLine(button, button.PrintLine), typingspaceW, TBh+2, LIGHTGREY);
  
  gfx->setCursor(x, yforLine(button, button.PrintLine) + text_offset + 1);  // puts cursor on a specific line with 2 pixels of V spacing
  gfx->print(msg);   
  //Serial.printf(" lines  tbh %i textheight %i  >lines are %i  \n",TBh,text_height,TBh/text_height);                                                       // lines should be blanked by previous filRect
  button.PrintLine = button.PrintLine + (TBh/(text_height+2))+1;
  if (button.PrintLine >= (LinesOfType)) {
    screenfull = true;
    if (!debugpause) {
      button.PrintLine = 0;
      gfx->fillRect(button.h + button.bordersize, button.v + button.bordersize, typingspaceW, typingspaceH, WHITE);
      screenfull = false;
    }
  }
  gfx->setTextBound(0, 0, 480, 480);
}



//********* Send Advice function - useful for messages can be switched on/off here for debugging
void sendAdvice(String message) {  // just a general purpose advice send that makes sure it sends at 115200

  Serial.print(message);
}
void sendAdvicef(const char* fmt, ...) {  //complete object type suitable for holding the information needed by the macros va_start, va_copy, va_arg, and va_end.
  static char msg[300] = { '\0' };        // used in message buildup
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, 128, fmt, args);
  va_end(args);
  // add checksum?
  int len = strlen(msg);
  sendAdvice(msg);
  delay(10);  // let it send!
}

//************ TIMING FUNCTIONS FOR TESTING PURPOSES ONLY ******************
//Note this is also an example of how useful Function overloading can be!!
void EventTiming(String input) {
  EventTiming(input, 1);  // 1 should be ignored as this is start or stop! but will also give immediate print!
}

void EventTiming(String input, int number) {  // Event timing, Usage START, STOP , 'Descrption text'   Number waits for the Nth call before serial.printing results (Description text + results).
  static unsigned long Start_time;
  static unsigned long timedInterval;
  static unsigned long _MaxInterval;
  static unsigned long SUMTotal;
  static int calls = 0;
  static int reads = 0;
  long NOW = micros();
  if (input == "START") {
    Start_time = NOW;
    return;
  }
  if (input == "STOP") {
    timedInterval = NOW - Start_time;
    SUMTotal = SUMTotal + timedInterval;
    if (timedInterval >= _MaxInterval) { _MaxInterval = timedInterval; }
    reads++;
    return;
  }
  calls++;
  if (calls < number) { return; }
  if (reads >= 2) {

    if (calls >= 2) {
      Serial.print("\r\n TIMING ");
      Serial.print(input);
      Serial.print(" Using (");
      Serial.print(reads);
      Serial.print(") Samples");
      Serial.print(" last: ");
      Serial.print(timedInterval);
      Serial.print("us Average : ");
      Serial.print(SUMTotal / reads);
      Serial.print("us  Max : ");
      Serial.print(_MaxInterval);
      Serial.println("uS");
    } else {
      Serial.print("\r\n TIMED ");
      Serial.print(input);
      Serial.print(" was :");
      Serial.print(timedInterval);
      Serial.println("uS");
    }
    _MaxInterval = 0;
    SUMTotal = 0;
    reads = 0;
    calls = 0;
  }
}


//SD and image functions  include #include "JpegFunc.h"
static int jpegDrawCallback(JPEGDRAW* pDraw) {
  // Serial.printf("Draw pos = %d,%d. size = %d x %d\n", pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight);
  gfx->draw16bitBeRGBBitmap(pDraw->x, pDraw->y, pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);
  return 1;
}


void listDir(fs::FS& fs, const char* dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\n", dirname);
  gfx->printf("Listing directory: %s\n", dirname);
  File root = fs.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      gfx->print("  DIR : ");
      gfx->println(file.name());
      if (levels) {
        listDir(fs, file.path(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      gfx->print("  FILE : ");
      gfx->println(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void SD_Setup() {

  SPI.begin(SD_SCK, SD_MISO, SD_MOSI);
  if (!SD.begin(SD_CS)) {
    Serial.println("Card Mount Failed");
    return;
  } else {
    // avoid the flash screen for now...
    // unsigned long start = millis();
    jpegDraw(JPEG_FILENAME_LOGO, jpegDrawCallback, true /* useBigEndian */,
             0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
    // Serial.printf("Time used: %lums\n", millis() - start);
    gfx->setTextColor(BLUE);  //if displaying the logo, write in Blue in a box!
    //  gfx->setTextBound(80, 80, 400, 400); // a test
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  gfx->println("");  // or it starts outside text bound??
  gfx->print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
    gfx->println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
    gfx->println("SCSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
    gfx->println("SDHC");
  } else {
    Serial.println("UNKNOWN");
    gfx->println("Unknown");
  }

  // uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  // Serial.printf("SD Card Size: %lluMB\n", cardSize);
  // Serial.println("*** SD card contents  (to three levels) ***");

  //listDir(SD, "/", 3);
}

//*****   AUDIO ....


void Audio_setup() {
  Serial.println("Audio setup ");
  delay(200);
  audio.setPinout(I2S_BCLK, I2S_LRCK, I2S_DOUT);
  audio.setVolume(15);  // 0...21
  if (audio.connecttoFS(SD, "/StartSound.mp3")) {
    delay(10);
    if (audio.isRunning()) { gfx->println("Playing Ships bells"); }
    while (audio.isRunning()) {
      audio.loop();
      vTaskDelay(1);
    }
  } else {
    gfx->print(" Failed  audio setup ");
    Serial.println("Audio setup FAILED");
  };
  // gfx->println("Exiting Setup");
  // audio.connecttoFS(SD, AUDIO_FILENAME_02); // would start some music -- not needed but fun !
}

void UseNMEA(char* buf, int type) {  //&sets pointer aso I can modify Line_Ready in this function
                                     //if (Line_Ready){
  if (buf[0] != 0) {
    // print serial version if on the wifi page terminal window page.
    if ((Display_Page == -1)) {  //debugpause in UpdateLinef
      if (type == 2) {
        gfx->setTextColor(BLUE);
        UpdateLinef(Terminal, "UDP:%s", buf);
      }
      if (type == 3) {
        gfx->setTextColor(RED);
        UpdateLinef(Terminal, "ESP:%s", buf);
      }
      gfx->setTextColor(BLACK);  // reset to black now!
      if (type == 1) { UpdateLinef(Terminal, "Ser:%s", buf); }
    }
    // now decode it for the displays to use
    pTOKEN = buf;                                               // pToken is used in processPacket to separate out the Data Fields
    if (processPacket(buf, BoatData)) { dataUpdated = true; };  // NOTE processPacket will search for CR! so do not remove it and then do page updates if true ?
    buf[0] = 0;                                                 //clear buf  when finished!
    return;
  }
}
void wifiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.println("WiFi connected");
      gfx->println(" Connected ! ");
      Serial.print("** Connected.: ");
      IsConnected = true;
      gfx->println(" Using :");
      gfx->print(WiFi.SSID());
      Serial.print(" *Running with:  ssid<");
      Serial.print(WiFi.SSID());
      Serial.println(">");
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      IsConnected = false;
      Serial.println("WiFi disconnected");
      Serial.print("WiFi lost Reason: ");
      Serial.println(info.wifi_sta_disconnected.reason);
      Serial.println("Trying to Reconnect");
      WiFi.begin(Current_Settings.ssid, Current_Settings.password);
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.print("The ESP32 has received an IP address from the network");
      gfx->print("IP: ");
      gfx->println(WiFi.localIP());
      Serial.println(WiFi.localIP());
      break;
  }
}







void ConnectWiFiusingCurrentSettings() {
  uint32_t StartTime = millis();
  gfx->println("Setting up WiFi");
  WiFi.disconnect(true);
  WiFi.onEvent(wifiEvent);  // Register the event handler
  WiFi.softAP("NMEADISPLAY", "12345678");
  delay(5);  // start an AP ?.. does this ure the WiFI scan bug?
  WiFi.mode(WIFI_AP_STA);
  //MDNS.begin("NMEA_Display");
  // (sucessful!) experiment.. do the WIfI/scan(i) and they get independently stored??
  NetworksFound = WiFi.scanNetworks(false, false, true, 250, 0, nullptr, nullptr);
  WiFi.disconnect(false);
  delay(100);
  gfx->printf(" Scan found <%i> networks\n", NetworksFound);
  Serial.printf(" Scan found <%i> networks", NetworksFound);
  bool found = false;
  for (int i = 0; i < NetworksFound; ++i) {
    if (WiFi.SSID(i) = Current_Settings.ssid) { found = true; }
  }
  if (found) { gfx->printf("Found <%s> network!\n", Current_Settings.ssid); }
  WiFi.begin(Current_Settings.ssid, Current_Settings.password);                 //standard wifi start
  while ((WiFi.status() != WL_CONNECTED) && (millis() <= StartTime + 10000)) {  //wait while it tries..10 seconds max
    delay(500);
    gfx->print(".");
    Serial.print(".");
  }
}

void CheckAndUseInputs() {  //multiinput capable, will check sources in sequence
  if (Current_Settings.ESP_NOW_ON) {
    if (nmea_EXT[0] != 0) { UseNMEA(nmea_EXT, 3); }
    // line_EXT = false;
  }

  if (Current_Settings.Serial_on) {
    if (Test_Serial_1()) { UseNMEA(nmea_1, 1); }
    //line_1=false;
  }
  if (Current_Settings.UDP_ON) {
    if (Test_U()) { UseNMEA(nmea_U, 2); }
  }
}
bool Test_Serial_1() {  // UART0 port P1
  static bool LineReading_1 = false;
  static int Skip_1 = 1;
  static int i_1;
  static bool line_1;  //has found a full line!
  byte b;
  if (!line_1) {                  // ONLY get characters if we are NOT still processing the last line message!
    while (Serial.available()) {  // get the character
      b = Serial.read();
      if (LineReading_1 == false) {
        nmea_1[0] = b;
        i_1 = 1;
        LineReading_1 = true;
      }  // Place first character of line in buffer location [0]
      else {
        nmea_1[i_1] = b;
        i_1 = i_1 + 1;
        if (b == 0x0A) {       //0A is LF
          nmea_1[i_1] = 0x00;  // put end in buffer.
          LineReading_1 = false;
          line_1 = true;
          return true;
        }
        if (i_1 > 150) {
          LineReading_1 = false;
          i_1 = 0;
          line_1 = false;
          return false;
        }
      }
    }
  }
  line_1 = false;
  return false;
}
bool Test_U() {  // check if udp packet (UDP is sent in lines..) has arrived
  static int Skip_U = 1;
  // if (!line_U) {  // only process if we have dealt with the last line.
  nmea_U[0] = 0x00;
  int packetSize = Udp.parsePacket();
  if (packetSize) {  // Deal with UDP packet
    if (packetSize >= (BufferLength)) {
      Udp.flush();
      return false;
    }  // Simply discard if too long
    int len = Udp.read(nmea_U, BufferLength);
    byte b = nmea_U[0];
    nmea_U[len] = 0;
    // nmea_UpacketSize = packetSize;
    //Serial.print(nmea_U);
    //line_U = true;
    return true;
  }  // udp PACKET DEALT WITH
     // }
  return false;
}
IPAddress Get_UDP_IP(IPAddress ip, IPAddress mk) {
  uint16_t ip_h = ((ip[0] << 8) | ip[1]);  // high 2 bytes
  uint16_t ip_l = ((ip[2] << 8) | ip[3]);  // low 2 bytes
  uint16_t mk_h = ((mk[0] << 8) | mk[1]);  // high 2 bytes
  uint16_t mk_l = ((mk[2] << 8) | mk[3]);  // low 2 bytes
  // reverse the mask
  mk_h = ~mk_h;
  mk_l = ~mk_l;
  // bitwise OR the net IP with the reversed mask
  ip_h = ip_h | mk_h;
  ip_l = ip_l | mk_l;
  // ip to return the result
  ip[0] = highByte(ip_h);
  ip[1] = lowByte(ip_h);
  ip[2] = highByte(ip_l);
  ip[3] = lowByte(ip_l);
  return ip;
}

void UDPSEND(const char* buf) {                              // this is the one that saves repetitious code!
  Udp.beginPacket(udp_st, atoi(Current_Settings.UDP_PORT));  //if connected and 'AP and STA' else use alternate udp_AP?
  Udp.print(buf);
  Udp.endPacket();

  // Udp.beginPacket(udp_ap, udpport);
  // Udp.print(buf);
  // Udp.endPacket();
}

// void ScannetworkstoGFX(int x) {
//   gfx->setCursor(0, x);  //(location of terminal. make better later!)
//   int n = WiFi.scanNetworks(false, false, false, 50, 0, nullptr, nullptr);
//   gfx->println("scan done");
//   if (n <= 0) {
//     gfx->println("no networks found");
//   } else {
//     gfx->print(n);
//     gfx->println(" networks found");
//     for (int i = 0; i < n; ++i) {
//       // Print SSID and RSSI for each network found
//       gfx->print(i + 1);
//       gfx->print(": ");
//       gfx->print(WiFi.SSID(i));
//       gfx->print(" (");
//       gfx->print(WiFi.RSSI(i));
//       gfx->println(")");
//       //    gfx->println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
//       delay(10);
//     }
//   }
// }
//************ Music stuff  Purely to explore if the ESP32 has any spare capacity while doing display etc!
//***
struct Music_info {
  String name;
  int length;
  int runtime;
  int volume;
  int status;
  int mute_volume;
  int m;
  int s;
} music_info = { "", 0, 0, 0, 0, 0, 0, 0 };

int get_music_list(fs::FS& fs, const char* dirname, uint8_t levels) {  // uses char* File_List[30] ?? how to pass the name here ??
  //Serial.printf("Listing directory: %s\n", dirname);
  bool ismusic;
  int i = 0;
  char temp[20];
  File root = fs.open(dirname);
  if (!root) {
    // Serial.println("Failed to open directory");
    return i;
  }
  if (!root.isDirectory()) {
    //  Serial.println("Not a directory");
    return i;
  }
  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
    } else {
      strcpy(temp, file.name());
      // if (temp.endsWith(".wav")) {
      // Serial.printf("    temp %s  strstr %i \n",temp,strstr(temp,".mp3"));//strstr(temp,".wav"),strstr(temp,".wav"),( strstr(temp,".wav") || strstr(temp,".wav")));
      if (strstr(temp, ".mp3")) {
        strcpy(File_List[i], temp);  //, sizeof(File_List[i]));  // char array version of '=' !! with limit on size
        i++;
      }
      //} else if (temp.endsWith(".mp3")) {
      //strcpy(File_List[i],temp); // sizeof(File_List[i]));
      //  i++;
      //}
    }
    file = root.openNextFile();
  }
  return i;
}


void open_new_song(String filename) {
  Serial.print(" Open _NEW song..");
  Serial.println(filename);
  music_info.name = filename.substring(1, filename.indexOf("."));
  Serial.print(" audio input..");
  Serial.println(music_info.name);
  audio.connecttoFS(SD, filename.c_str());
  music_info.runtime = audio.getAudioCurrentTime();
  music_info.length = audio.getAudioFileDuration();
  music_info.volume = audio.getVolume();
  music_info.status = 1;
  music_info.m = music_info.length / 60;
  music_info.s = music_info.length % 60;
}
