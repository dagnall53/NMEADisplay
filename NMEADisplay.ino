//*******************************************************************************
/*
Compiled and tested with ESp32 V2.0.11  V3has deprecated some functions and this code will not work with V3!
GFX library for Arduino 1.5.5
Select "ESP32-S3 DEV Module"
Select PSRAM "OPI PSRAM"




*/
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
#include "FONTS/fonts.h"  // have to use reserved directory name src/ for arduino?
#include "FONTS/FreeSansBold6pt7b.h"
#include "FONTS/FreeSansBold8pt7b.h"
#include "FONTS/FreeSansBold12pt7b.h"
#include "FONTS/FreeSansBold18pt7b.h"
#include "FONTS/FreeSansBold27pt7b.h"
#include "FONTS/FreeSansBold40pt7b.h"


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

const char soft_version[] = " Version 2.0";


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
//                                                                                    LOG off NMEAlog Off
MySettings Default_Settings = { 12, "GUESTBOAT", "12345678", "2002", false, true, true,false,false };
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
Button Fontdescriber = { 0, 0, 480, 75, 5, BLUE, WHITE, BLACK };  //also used for showing the current settings
Button FontBox = { 0, 150, 480, 330, 5, BLUE, WHITE, BLUE };

Button WindDisplay = { 0, 0, 480, 480, 0, BLUE, WHITE, BLACK };  // full screen no border

//used for single data display
Button BigSingleDisplay = { 10, 110, 460, 360, 5, BLUE, WHITE, BLACK };
Button BigSingleTopRight = { 240, 0, 230, 110, 5, BLUE, WHITE, BLACK };
Button BigSingleTopLeft = { 10, 0, 230, 110, 5, BLUE, WHITE, BLACK };
//used for nmea RMC /GPS display // was only three lines to start!
Button Threelines0 = { 20, 30, 440, 80, 5, BLUE, WHITE, BLACK };
Button Threelines1 = { 20, 130, 440, 80, 5, BLUE, WHITE, BLACK };
Button Threelines2 = { 20, 230, 440, 80, 5, BLUE, WHITE, BLACK };
Button Threelines3 = { 20, 330, 440, 80, 5, BLUE, WHITE, BLACK };
// for the quarter screens
Button topLeftquarter = { 0, 0, 235, 235, 5, BLUE, WHITE, BLACK };
Button bottomLeftquarter = { 0, 240, 235, 235, 5, BLUE, WHITE, BLACK };
Button topRightquarter = { 240, 0, 235, 235, 5, BLUE, WHITE, BLACK };
Button bottomRightquarter = { 240, 240, 235, 235, 5, BLUE, WHITE, BLACK };

Button LogIndicator =  {225,465,30,15,1,BLUE,WHITE,BLUE};
Button TopLeftbutton = { 0, 0, 75, 75, 5, BLUE, WHITE, BLACK };
Button TopRightbutton = { 405, 0, 75, 75, 5, BLUE, WHITE, BLACK };
Button BottomRightbutton = { 405, 405, 75, 75, 5, BLUE, WHITE, BLACK };
Button BottomLeftbutton = { 0, 405, 75, 75, 5, BLUE, WHITE, BLACK };

// buttons for the wifi/settings pages
Button TOPButton = { 20, 10, 430, 35, 5, WHITE, BLACK, BLUE };
Button SecondRowButton = { 20, 50, 430, 35, 5, WHITE, BLACK, BLUE };
Button ThirdRowButton = { 20, 90, 430, 35, 5, WHITE, BLACK, BLUE };
Button FourthRowButton = { 20, 130, 430, 35, 5, WHITE, BLACK, BLUE };
Button FifthRowButton = { 20, 170, 430, 35, 5, WHITE, BLACK, BLUE };

#define sw_width 65
Button Switch1 = { 20, 180, sw_width, 35, 5, WHITE, BLACK, BLUE };
Button Switch2 = { 100, 180, sw_width, 35, 5, WHITE, BLACK, BLUE };
Button Switch3 = { 180, 180, sw_width, 35, 5, WHITE, BLACK, BLUE };
Button Switch5 = { 260, 180, sw_width, 35, 5, WHITE, BLACK, BLUE };
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


void WindArrow2(Button button, instData Speed, instData& Wind) {
 // Serial.printf(" ** DEBUG  speed %f    wind %f ",Speed.data,Wind.data);
  bool recent = (Wind.updated >= millis() - 3000);
  if (!Wind.displayed) { WindArrowSub(button, Speed, Wind); }
  if (Wind.greyed) { return; }
  if (!recent && !Wind.greyed) { WindArrowSub(button, Speed, Wind); }
}


void WindArrowSub(Button button, instData Speed, instData& wind) {
  //Serial.printf(" ** DEBUG WindArrowSub speed %f    wind %f \n",Speed.data,wind.data);
  bool recent = (wind.updated >= millis() - 3000);
  Phv center;
  int rad, outer, inner;
  static int lastwind, lastfont;
  center.h = button.h + button.width / 2;
  center.v = button.v + button.height / 2;
  rad = (button.height - (2 * button.bordersize)) / 2;  // height used as more likely to be squashed in height
  outer = (rad * 82) / 100;                             //% of full radius (at full height) (COMPASS has .83 as inner circle)
  inner = (rad * 28) / 100;                             //25% USe same settings as pointer
  DrawMeterPointer(center, lastwind, inner, outer, 2, button.backcol, button.backcol);
  if (wind.data != NMEA0183DoubleNA) {
    if (wind.updated >= millis() - 3000) {
      DrawMeterPointer(center, wind.data, inner, outer, 2, button.textcol, BLACK);
    } else {
      wind.greyed = true;
      DrawMeterPointer(center, wind.data, inner, outer, 2, LIGHTGREY, LIGHTGREY);
    }
  }
  lastwind = wind.data;
  wind.displayed = true;
  lastfont = MasterFont;
  if (Speed.data != NMEA0183DoubleNA) {
    if (rad <= 130) {
      UpdateDataTwoSize(8, 7, button, Speed, "%2.0fkt");
    } else {
      UpdateDataTwoSize(10, 9, button, Speed, "%2.0fkt");
    }
  }

  setFont(lastfont);
}


void DrawMeterPointer(Phv center, int wind, int inner, int outer, int linewidth, uint16_t FILLCOLOUR, uint16_t LINECOLOUR) {  // WIP
  Phv P1, P2, P3, P4, P5, P6;
  P1 = translate(center, wind - linewidth, outer);
  P2 = translate(center, wind + linewidth, outer);
  P3 = translate(center, wind - (4 * linewidth), inner);
  P4 = translate(center, wind + (4 * linewidth), inner);
  P5 = translate(center, wind, inner);
  P6 = translate(center, wind, outer);
  PTriangleFill(P1, P2, P3, FILLCOLOUR);
  PTriangleFill(P2, P3, P4, FILLCOLOUR);
  Pdrawline(P5, P6, LINECOLOUR);
}

Phv translate(Phv center, int angle, int rad) {
  Phv moved;
  moved.h = center.h + ((rad * getSine(angle)) / 100);
  moved.v = center.v + ((rad * getCosine(angle)) / 100);
  return moved;
}




void DrawCompass(Button button) {
  //xy are center in draw compass
  int x, y, rad;
  x = button.h + button.width / 2;
  y = button.v + button.height / 2;
  rad = (button.height - (2 * button.bordersize)) / 2;
  //Work in progress!
  int Rad1, Rad2, Rad3, Rad4, inner;
  // gfx->drawRect(x-rad,y-rad,rad*2,rad*2,BLACK);gfx->drawCircle(x,y,rad,WHITE);gfx->drawCircle(x,y,rad-1,Blue);gfx->drawCircle(x,y,Start,Blue);
  Rad1 = rad * 0.83;  //200
  Rad2 = rad * 0.86;  //208
  Rad3 = rad * 0.91;  //220
  Rad4 = rad * 0.94;

  inner = (rad * 28) / 100;  //28% USe same settings as pointer
  gfx->fillRect(x - rad, y - rad, rad * 2, rad * 2, button.backcol);
  gfx->fillCircle(x, y, rad, button.textcol);   //white
  gfx->fillCircle(x, y, Rad1, button.backcol);  //bluse
  gfx->fillCircle(x, y, inner - 1, button.textcol);
  gfx->fillCircle(x, y, inner - 5, button.backcol);

  //rad =240 example Rad2 is 200 to 208   bar is 200 to 239 wind colours 200 to 220
  // colour segments
  gfx->fillArc(x, y, Rad3, Rad1, 270 - 45, 270, RED);
  gfx->fillArc(x, y, Rad3, Rad1, 270, 270 + 45, GREEN);
  //Mark 12 linesarks at 30 degrees
  for (int i = 0; i < (360 / 30); i++) { gfx->fillArc(x, y, rad, Rad1, i * 30, (i * 30) + 1, BLACK); }  //239 to 200
  for (int i = 0; i < (360 / 10); i++) { gfx->fillArc(x, y, rad, Rad4, i * 10, (i * 10) + 1, BLACK); }  // dots at 10 degrees
}


void ShowToplinesettings(MySettings A, String Text) {
  int local;
  local = MasterFont;
  setFont(0);  // SETS MasterFont, so cannot use MasterFont directly in last line and have to save it!
  long rssiValue = WiFi.RSSI();
  gfx->setTextSize(1);
  gfx->setTextColor(Fontdescriber.textcol);
  Fontdescriber.PrintLine = 0;
  UpdateLinef(Fontdescriber, "%s: Serial<%s> UDP<%s> ESP<%s>", Text, A.Serial_on On_Off, A.UDP_ON On_Off, A.ESP_NOW_ON On_Off);
  //GFXBoxPrintf(0, 0, 1, "%s: Serial<%s> UDP<%s> ESP<%s>", Text, A.Serial_on On_Off, A.UDP_ON On_Off, A.ESP_NOW_ON On_Off);
  UpdateLinef(Fontdescriber, " SSID<%s> PWD<%s> UDPPORT<%s>", A.ssid, A.password, A.UDP_PORT);
  sta_ip = WiFi.localIP();
  UpdateLinef(Fontdescriber, " IP: %i.%i.%i.%i   RSSI %i", sta_ip[0], sta_ip[1], sta_ip[2], sta_ip[3], rssiValue);
  setFont(local);
}
void ShowToplinesettings(String Text) {
  ShowToplinesettings(Current_Settings, Text);
}

void Display(int page) {  // setups for alternate pages to be selected by page.

        static double startposlat,startposlon;
        double LatD,LongD; //deltas
        int magnification,h,v;



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
  static instData LocalCopy,LocalCopy2;

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

  switch (page) {  // just show the logos on the sd card top page
    case -200:
      if (RunSetup) {
        jpegDraw("/logo.jpg", jpegDrawCallback, true /* useBigEndian */,
                 0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
        GFXBorderBoxPrintf(Full0Center, "Jpg tests -Return to Menu-");
        GFXBorderBoxPrintf(Full1Center, "logo.jpg");
        GFXBorderBoxPrintf(Full2Center, "logo1.jpg");
        GFXBorderBoxPrintf(Full3Center, "logo2.jpg");
        GFXBorderBoxPrintf(Full4Center, "logo3.jpg");
        GFXBorderBoxPrintf(Full5Center, "logo4.jpg");
      }

      if (CheckButton(Full0Center)) { Display_Page = 0; }
      if (CheckButton(Full1Center)) {
        jpegDraw("/logo.jpg", jpegDrawCallback, true /* useBigEndian */,
                 0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
        GFXBorderBoxPrintf(Full0Center, "logo");
      }
      if (CheckButton(Full2Center)) {
        jpegDraw("/logo1.jpg", jpegDrawCallback, true /* useBigEndian */,
                 0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
        GFXBorderBoxPrintf(Full0Center, "logo1");
      }
      if (CheckButton(Full3Center)) {
        jpegDraw("/logo2.jpg", jpegDrawCallback, false /* useBigEndian */,
                 0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
        GFXBorderBoxPrintf(Full0Center, "logo2");
      }
      if (CheckButton(Full4Center)) {
        jpegDraw("/logo3.jpg", jpegDrawCallback, true /* useBigEndian */,
                 0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
        GFXBorderBoxPrintf(Full0Center, "logo3");
      }
      if (CheckButton(Full5Center)) {
        jpegDraw("/logo4.jpg", jpegDrawCallback, true /* useBigEndian */,
                 0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
        GFXBorderBoxPrintf(Full0Center, "logo4");
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
        GFXBorderBoxPrintf(Fontdescriber, "-TEST Colours- ");
      }

      if (millis() >= slowdown + 10000) {
        slowdown = millis();
        gfx->fillScreen(BLACK);
        fontlocal = fontlocal + 1;
        if (fontlocal > 10) { fontlocal = 0; }
        GFXBorderBoxPrintf(Fontdescriber, "Font size %i", fontlocal);
        //DisplayCheck(true);
      }



      break;

    case -20:  // Secondary "extra settings"
      if (RunSetup) {

        ShowToplinesettings("Now");
        setFont(3);
        // GFXBorderBoxPrintf(TopLeftbutton, "Page-");
        // GFXBorderBoxPrintf(TopRightbutton, "Page+");
        setFont(3);
        GFXBorderBoxPrintf(Full0Center, "-Test JPegs-");
        GFXBorderBoxPrintf(Full1Center, "Check SD /Audio");
        GFXBorderBoxPrintf(Full2Center, "Check Fonts");
        // GFXBorderBoxPrintf(Full2Center, "Settings  WIFI etc");
        // GFXBorderBoxPrintf(Full3Center, "See NMEA");

        GFXBorderBoxPrintf(Full5Center, "Main Menu");
      }
      if (millis() > slowdown + 500) {
        slowdown = millis();
      }
      if (CheckButton(Full0Center)) { Display_Page = -200; }
      if (CheckButton(Full1Center)) { Display_Page = -9; }
      if (CheckButton(Full2Center)) { Display_Page = -10; }
      //  if (CheckButton(Full3Center)) { Display_Page = 4; }
      //   if (CheckButton(Full4Center)) { Display_Page = -10; }
      if (CheckButton(Full5Center)) { Display_Page = 0; }

      break;

    case -10:  // a test page for fonts
      if (RunSetup) {
        GFXBorderBoxPrintf(Full0Center, "-Font test -");
      }
      if (millis() > slowdown + 5000) {
        slowdown = millis();
        gfx->fillScreen(BLUE);
        // fontlocal = fontlocal + 1;
        // if (fontlocal > 15) { fontlocal = 0; } // just use the buttons to change!
        temp = 12.3;
        setFont(fontlocal);
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

        int file_num;
        //Ver 1.2 I have moved the music to a subdirectory in anticipation of adding SD logging
        //   if (fs.chdir("/music")) {
        //   Serial.println("Changed directory to /music");
        // } else {
        //   Serial.println("Failed to change directory");
        // }                                                                                           // = get_music_list(SD, "/", 0, file_list);  // see https://github.com/VolosR/MakePythonLCDMP3/blob/main/MakePythonLCDMP3.ino#L101
        file_num = get_music_list(SD, "/music", 0);                                                             // Char array  based version of  https://github.com/VolosR/MakePythonLCDMP3/blob/main/MakePythonLCDMP3.ino#L101
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
        open_new_song("/music/", File_List[Playing]);
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
      if (CheckButton(TOPButton)) { Display_Page = -1; }
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
      //if (CheckButton(Full0Center)) { Display_Page = -1; }
      if (CheckButton(TOPButton)) { Display_Page = -1; }

      break;

    case -2:
      if (RunSetup) {
        GFXBorderBoxPrintf(Full0Center, "Set SSID");
        GFXBorderBoxPrintf(TopRightbutton, "Scan");
        AddTitleBorderBox(0,TopRightbutton, "WiFi");
        keyboard(-1);  //reset
        Use_Keyboard(Current_Settings.ssid, sizeof(Current_Settings.ssid));
        keyboard(0);
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
        AddTitleBorderBox(0,Switch1, "Serial");
        GFXBorderBoxPrintf(Switch2, Current_Settings.UDP_ON On_Off);
        AddTitleBorderBox(0,Switch2, "UDP");
        GFXBorderBoxPrintf(Switch3, Current_Settings.ESP_NOW_ON On_Off);
        AddTitleBorderBox(0,Switch3, "ESP-Now");
        GFXBorderBoxPrintf(Switch5, Current_Settings.Log_ON On_Off);
        AddTitleBorderBox(0,Switch5, "Log");
        GFXBorderBoxPrintf(Switch4, "UPDATE");
        AddTitleBorderBox(0,Switch4, "EEPROM");
        GFXBorderBoxPrintf(Terminal, "- NMEA DATA -");
        AddTitleBorderBox(0,Terminal, "TERMINAL");
        setFont(0);
        DataChanged = false;
        //while (ts.sTouched{yield(); Serial.println("yeilding -1");}
      }
      if (millis() > slowdown + 1000) {
        slowdown = millis();
      }

      if (CheckButton(Terminal)) {
        debugpause = !debugpause;
        if (!debugpause) {
          AddTitleBorderBox(0,Terminal, "TERMINAL");
        } else {
          AddTitleBorderBox(0,Terminal, "-Paused-");
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
      if (CheckButton(Switch5)) {
        Current_Settings.Log_ON = !Current_Settings.Log_ON;
        if (Current_Settings.Log_ON) { Startlogfile(); }
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
        GFXBorderBoxPrintf(Full0Center, "-Extra settings-");
        GFXBorderBoxPrintf(Full1Center, "Settings  WIFI etc");
        GFXBorderBoxPrintf(Full2Center, "See NMEA");
      //  GFXBorderBoxPrintf(Full4Center, "Check Fonts");
        GFXBorderBoxPrintf(Full5Center, "Save / Reset to Quad NMEA");
      }
      if (millis() > slowdown + 500) {
        slowdown = millis();
      }
      if (CheckButton(Full0Center)) { Display_Page = -20; }
      if (CheckButton(Full1Center)) { Display_Page = -1; }
      if (CheckButton(Full2Center)) { Display_Page = 4; }
     
      if (CheckButton(Full5Center)) {
        Display_Page = 4;
        EEPROM_WRITE(Current_Settings);
        delay(50);
        ESP.restart();
      }

      break;




    case 4:  // Quad display
      if (RunSetup) {
        setFont(10);
        gfx->fillScreen(BLACK);
        // DrawCompass(360, 120, 120);
        DrawCompass(topRightquarter);
        AddTitleBorderBox(0,topRightquarter, "WIND");
        GFXBorderBoxPrintf(topLeftquarter, "");
        AddTitleBorderBox(0,topLeftquarter, "STW");
        GFXBorderBoxPrintf(bottomLeftquarter, "");
        AddTitleBorderBox(0,bottomLeftquarter, "DEPTH");
        GFXBorderBoxPrintf(bottomRightquarter, "");
        AddTitleBorderBox(0,bottomRightquarter, "SOG");
        delay(500);
      }
      if (millis() > slowdown + 300) {
        slowdown = millis();
        setFont(10);  // note: all the 'updates' now check for new data else return immediately
      }
      WindArrow2(topRightquarter, BoatData.WindSpeedK, BoatData.WindAngle);
      UpdateDataTwoSize(12, 11, topLeftquarter, BoatData.STW, "%4.1fkts");
      UpdateDataTwoSize(12, 11, bottomLeftquarter, BoatData.WaterDepth, "%4.1fm");
      UpdateDataTwoSize(12, 11, bottomRightquarter, BoatData.SOG, "%4.1fkts");
      // }
      if (CheckButton(topLeftquarter)) { Display_Page = 6; }      //stw
      if (CheckButton(bottomLeftquarter)) { Display_Page = 7; }   //depth
      if (CheckButton(topRightquarter)) { Display_Page = 5; }     // Wind
      if (CheckButton(bottomRightquarter)) { Display_Page = 8; }  //SOG
      break;

    case 5:  // wind instrument
      if (RunSetup) {
        setFont(10);
        GFXBorderBoxPrintf(BigSingleDisplay, "");
        GFXBorderBoxPrintf(BigSingleTopRight, "");
        DrawCompass(BigSingleDisplay);
      }
      if (millis() > slowdown + 500) {
        slowdown = millis();
      }
      LocalCopy = BoatData.WindAngle;  //Duplicate wind angle so it can be shown again in a second box
      WindArrow2(BigSingleDisplay, BoatData.WindSpeedK, BoatData.WindAngle);
      UpdateDataTwoSize(10, 9, BigSingleTopRight, LocalCopy, "%3.0f deg");
   
      if (CheckButton(topLeftquarter)) { Display_Page = 4; }
      if (CheckButton(bottomLeftquarter)) { Display_Page = 4; }
      if (CheckButton(topRightquarter)) { Display_Page = 0; }
      if (CheckButton(bottomRightquarter)) { Display_Page = 4; }
      break;
    case 6:  //Speed Through WATER GRAPH
      if (RunSetup) {
        setFont(10);
        GFXBorderBoxPrintf(BigSingleTopRight, "");
        GFXBorderBoxPrintf(BigSingleTopLeft, "");
        AddTitleBorderBox(0,BigSingleTopLeft, "SOG");
        AddTitleBorderBox(0,BigSingleTopRight, "STW");
        GFXBorderBoxPrintf(BigSingleDisplay, "");
        AddTitleBorderBox(0,BigSingleDisplay, "STW Graph");
        LocalCopy = BoatData.STW;
        DrawGraph(true, BigSingleDisplay, LocalCopy, 0, 10);
      }
      if (millis() > slowdown + 500) {
        slowdown = millis();
      }
      LocalCopy = BoatData.STW;
      DrawGraph(false, BigSingleDisplay, LocalCopy, 0, 10);
      UpdateDataTwoSize(11, 10, BigSingleTopLeft, BoatData.SOG, "%2.1fkt");
      UpdateDataTwoSize(11, 10, BigSingleTopRight, BoatData.STW, "%2.1fkt");


      //  if (CheckButton(Full0Center)) { Display_Page = 4; }
      //        TouchCrosshair(20); quarters select big screens
      if (CheckButton(BigSingleTopLeft)) { Display_Page = 8; }
      if (CheckButton(bottomLeftquarter)) { Display_Page = 9; }
      if (CheckButton(bottomRightquarter)) { Display_Page = 4; }

      break;
    case 7:  // Depth
      if (RunSetup) {
        setFont(10);
        GFXBorderBoxPrintf(BigSingleTopRight, "");
        AddTitleBorderBox(0,BigSingleTopRight, "Depth");
        GFXBorderBoxPrintf(BigSingleDisplay, "");
        AddTitleBorderBox(0,BigSingleDisplay, "Fathmometer 30m");
        LocalCopy = BoatData.WaterDepth;  //WaterDepth, "%4.1f m");
        DrawGraph(true, BigSingleDisplay, LocalCopy, 30, 0);
      }
      if (millis() > slowdown + 500) {
        slowdown = millis();
      }
      LocalCopy = BoatData.WaterDepth;  //WaterDepth, "%4.1f m");
      DrawGraph(false, BigSingleDisplay, LocalCopy, 30, 0);
      UpdateDataTwoSize(11, 10, BigSingleTopRight, BoatData.WaterDepth, "%4.1fm");

      if (CheckButton(BigSingleTopRight)) { Display_Page = 4; }
      //        TouchCrosshair(20); quarters select big screens
      if (CheckButton(topLeftquarter)) { Display_Page = 4; }
      if (CheckButton(BigSingleDisplay)) { Display_Page = 11; }


      break;
    case 8:  //SOG  graph
      if (RunSetup) {
        setFont(10);
        GFXBorderBoxPrintf(BigSingleTopRight, "");
        GFXBorderBoxPrintf(BigSingleTopLeft, "");
        AddTitleBorderBox(0,BigSingleTopLeft, "SOG");
        AddTitleBorderBox(0,BigSingleTopRight, "STW");
        GFXBorderBoxPrintf(BigSingleDisplay, "");
        AddTitleBorderBox(0,BigSingleDisplay, "SOG Graph");
        LocalCopy = BoatData.SOG;
        DrawGraph(true, BigSingleDisplay, LocalCopy, 0, 10);
      }
      if (millis() > slowdown + 500) {
        slowdown = millis();
      }
      LocalCopy = BoatData.SOG;
      DrawGraph(false, BigSingleDisplay, LocalCopy, 0, 10);
      UpdateDataTwoSize(11, 10, BigSingleTopRight, BoatData.STW, "%2.1fkt");
      UpdateDataTwoSize(11, 10, BigSingleTopLeft, BoatData.SOG, "%2.1fkt");

      //if (CheckButton(Full0Center)) { Display_Page = 4; }
      //        TouchCrosshair(20); quarters select big screens
      //if (CheckButton(topLeftquarter)) { Display_Page = 4; }   
      if (CheckButton(bottomLeftquarter)) { Display_Page = 9; }
      if (CheckButton(BigSingleTopRight)) { Display_Page = 6; }
      if (CheckButton(bottomRightquarter)) { Display_Page = 4; }
   
      


      break;

    case 9:  // GPS page
      if (RunSetup) {
        setFont(10);
        GFXBorderBoxPrintf(BigSingleDisplay, "");
        GFXBorderBoxPrintf(BigSingleTopLeft," Click for graphic");
      }
      if (millis() > slowdown + 1000) {
        slowdown = millis();
        // do this one once a second.. I have not yet got simplified functions testing if previously displayed and greyed yet
        gfx->setTextColor(BigSingleDisplay.textcol);
        BigSingleDisplay.PrintLine = 0;
        UpdateLinef(BigSingleDisplay, "%.0f Satellites in view", BoatData.SatsInView);
        if (BoatData.GPSTime != NMEA0183DoubleNA) {
          UpdateLinef(BigSingleDisplay, "");
          UpdateLinef(BigSingleDisplay, "Date: %06i ", int(BoatData.GPSDate));
          UpdateLinef(BigSingleDisplay, "");
          UpdateLinef(BigSingleDisplay, "TIME: %02i:%02i:%02i",
                      int(BoatData.GPSTime) / 3600, (int(BoatData.GPSTime) % 3600) / 60, (int(BoatData.GPSTime) % 3600) % 60);
        }
        if (BoatData.Latitude != NMEA0183DoubleNA) {
          UpdateLinef(BigSingleDisplay, "");
          UpdateLinef(BigSingleDisplay, "LAT: %f", BoatData.Latitude);
          UpdateLinef(BigSingleDisplay, "");
          UpdateLinef(BigSingleDisplay, "LON: %f", BoatData.Longitude);
        }
      }
      if (CheckButton(BigSingleTopLeft)) { Display_Page = 10; }
      if (CheckButton(bottomLeftquarter)) { Display_Page = 4; }  //Loop to the main settings page
      if (CheckButton(topRightquarter)) { Display_Page = 4; }
      if (CheckButton(bottomRightquarter)) { Display_Page = 4; }  // test! check default loops back to 0

      break;
case 10:  // GPS page 2
      if (RunSetup) {
        setFont(8);
        GFXBorderBoxPrintf(BigSingleDisplay, "");
         GFXBorderBoxPrintf(BigSingleTopLeft,"");
         GFXBorderBoxPrintf(BigSingleTopRight," Main Menu ");
      }
      if (millis() > slowdown + 1000) {
        slowdown = millis();
        // do this one once a second.. I have not yet got simplified functions testing if previously displayed and greyed yet
        gfx->setTextColor(BigSingleDisplay.textcol);
        BigSingleTopLeft.PrintLine = 0;
        UpdateLinef(BigSingleTopLeft, "%.0f Satellites in view", BoatData.SatsInView);
        if (BoatData.GPSTime != NMEA0183DoubleNA) {
          //UpdateLinef(BigSingleTopLeft, "");
          UpdateLinef(BigSingleTopLeft, "Date: %06i ", int(BoatData.GPSDate));
         // UpdateLinef(BigSingleTopLeft, "");
          UpdateLinef(BigSingleTopLeft, "TIME: %02i:%02i:%02i",
                      int(BoatData.GPSTime) / 3600, (int(BoatData.GPSTime) % 3600) / 60, (int(BoatData.GPSTime) % 3600) % 60);
        }
        if (BoatData.Latitude != NMEA0183DoubleNA) {

          UpdateLinef(BigSingleTopLeft, "LAT: %f", BoatData.Latitude);
          UpdateLinef(BigSingleTopLeft, "LON: %f", BoatData.Longitude);
        
        // would like to put equivalent of anchor watch / plot here/ change to a sensible function later! 
        
        static double startposlat,startposlon;
        double LatD,LongD,magnification; //deltas
        int h,v;
        magnification = 111111;  //approx 1pixel per meter? 1 degree is roughly 111111 m
        AddTitleBorderBox(0,BigSingleDisplay, "Magnification:%3.0f pixel/m",magnification/111111);
        if (startposlon==0){startposlat=BoatData.Latitude;startposlon=BoatData.Longitude;}
        h=BigSingleDisplay.h+((BigSingleDisplay.width)/2);
        v=BigSingleDisplay.v+((BigSingleDisplay.height)/2);
        LatD = h+ int((BoatData.Latitude-startposlat)* magnification);
        LongD= v+ int((BoatData.Longitude-startposlon)* magnification); 
       //set limits!! ?
        gfx->fillCircle(LongD, LatD, 4, BigSingleDisplay.textcol);

       } 
      }
      if (CheckButton(topLeftquarter)) { Display_Page = 9; }
      if (CheckButton(BigSingleTopRight)) { Display_Page = 4; }
    // TBD , reset position?  
      if (CheckButton(BigSingleDisplay)) {
        if (BoatData.Latitude != NMEA0183DoubleNA) {        
          startposlat=BoatData.Latitude;
          startposlon=BoatData.Longitude;
          Serial.printf(" reset  %f   %f \n",startposlat,startposlon);
        GFXBorderBoxPrintf(BigSingleDisplay, "");}
      }  // test! check default loops back to 0

      break;



    case 11:  // Depth different range
      if (RunSetup) {
        setFont(10);
        GFXBorderBoxPrintf(BigSingleTopRight, "");
        AddTitleBorderBox(0,BigSingleTopRight, "Depth");
        GFXBorderBoxPrintf(BigSingleDisplay, "");
        AddTitleBorderBox(0,BigSingleDisplay, "Fathmometer 10m");
        LocalCopy = BoatData.WaterDepth;  //WaterDepth, "%4.1f m");
      DrawGraph(true, BigSingleDisplay, LocalCopy, 10, 0);
      }
      if (millis() > slowdown + 500) {
        slowdown = millis();
      }
      LocalCopy = BoatData.WaterDepth;  //WaterDepth, "%4.1f m");
      DrawGraph(false, BigSingleDisplay, LocalCopy, 10, 0);
      UpdateDataTwoSize(11, 10, BigSingleTopRight, BoatData.WaterDepth, "%4.1fm");

      
      //        TouchCrosshair(20); quarters select big screens
      if (CheckButton(topLeftquarter)) { Display_Page = 4; }
      if (CheckButton(BigSingleDisplay)) { Display_Page = 7; }
      if (CheckButton(topRightquarter)) { Display_Page = 4; }
      

      break;

    default:
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
    case 7:  //SANS BOLD 6 pt
      Fontname = "FreeSansBold6pt7b";
      gfx->setFont(&FreeSansBold6pt7b);
      text_height = (FreeSansBold6pt7bGlyphs[0x38].height) + 1;
      text_offset = -(FreeSansBold6pt7bGlyphs[0x38].yOffset);
      text_char_width = 12;
      break;
    case 8:  //SANS BOLD 8 pt
      Fontname = "FreeSansBold8pt7b";
      gfx->setFont(&FreeSansBold8pt7b);
      text_height = (FreeSansBold8pt7bGlyphs[0x38].height) + 1;
      text_offset = -(FreeSansBold8pt7bGlyphs[0x38].yOffset);
      text_char_width = 12;
      break;
    case 9:  //SANS BOLD 12 pt
      Fontname = "FreeSansBold12pt7b";
      gfx->setFont(&FreeSansBold12pt7b);
      text_height = (FreeSansBold12pt7bGlyphs[0x38].height) + 1;
      text_offset = -(FreeSansBold12pt7bGlyphs[0x38].yOffset);
      text_char_width = 12;
      break;
    case 10:  //SANS BOLD 18 pt
      Fontname = "FreeSansBold18pt7b";
      gfx->setFont(&FreeSansBold18pt7b);
      text_height = (FreeSansBold18pt7bGlyphs[0x38].height) + 1;
      text_offset = -(FreeSansBold18pt7bGlyphs[0x38].yOffset);
      text_char_width = 12;
      break;
    case 11:  //sans BOLD 27 pt
      Fontname = "FreeSansBold27pt7b";
      gfx->setFont(&FreeSansBold27pt7b);
      text_height = (FreeSansBold27pt7bGlyphs[0x38].height) + 1;
      text_offset = -(FreeSansBold27pt7bGlyphs[0x38].yOffset);
      text_char_width = 12;
      break;
    case 12:  //sans BOLD 40 pt
      Fontname = "FreeSansBold40pt7b";
      gfx->setFont(&FreeSansBold40pt7b);
      text_height = (FreeSansBold40pt7bGlyphs[0x38].height) + 1;
      text_offset = -(FreeSansBold40pt7bGlyphs[0x38].yOffset);
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
  Serial.println(soft_version);
  //Serial.println(" IDF will throw errors here as one pin is -1!");
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
  gfx->setTextColor(WHITE);
  gfx->setTextBound(40, 40, 440, 380);
#ifdef GFX_BL  // I have no idea why this digital write seems to be done twice- perhaps to reset some screens?
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
  gfx->setTextBound(0, 0, 480, 480);
  gfx->setTextColor(WHITE);
  // gfx->setTextBound(0, 0, 480, 480); //reset or it wil cause issues later in other prints?
  //Startlogfile();
  Display(Display_Page);
}

void loop() {
  static unsigned long LogInterval;
  static bool flash;
  static unsigned long flashinterval;
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
  if ((millis() >= flashinterval)) {
    flashinterval = millis() + 500;
    if (!Current_Settings.Log_ON){AddTitleBorderBox(0,LogIndicator,"    ");}
    else{   
    flash=!flash;
    if (flash) {AddTitleBorderBox(0,LogIndicator," LOG");}else{AddTitleBorderBox(0,LogIndicator," ON ");}
    }
  }

  if (!Current_Settings.Log_ON){AddTitleBorderBox(0,LogIndicator,"   ");}
  if ((Current_Settings.Log_ON) && (millis() >= LogInterval)) {
    LogInterval = millis() + 5000;

    LOG("TIME: %02i:%02i:%02i ,%4.2f ,%4.2f ,%4.2f ,%3.1f ,%4.0f ,%f ,%f \r\n",
        int(BoatData.GPSTime) / 3600, (int(BoatData.GPSTime) % 3600) / 60, (int(BoatData.GPSTime) % 3600) % 60,
        BoatData.STW.data, BoatData.SOG.data, BoatData.WaterDepth.data, BoatData.WindSpeedK.data,
        BoatData.WindAngle.data, BoatData.Latitude, BoatData.Longitude);
  }
  //EventTiming(" loop time touch sample display");
  //vTaskDelay(1);  // Audio is distorted without this?? used in https://github.com/schreibfaul1/ESP32-audioI2S/blob/master/examples/plays%20all%20files%20in%20a%20directory/plays_all_files_in_a_directory.ino
  // //.... (audio.isRunning()){   delay(100);gfx->println("Playing Ships bells"); Serial.println("Waiting for bells to finish!");}
}


// useful when debugging to see where touch is
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
void UseNMEA(char* buf, int type) {
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
extern bool hasSD;
void SD_Setup() {
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI);
  if (!SD.begin(SD_CS)) {
    Serial.println("Card Mount Failed");
    return;
  } else {
    hasSD = true;
    // flash logo
    jpegDraw(JPEG_FILENAME_LOGO, jpegDrawCallback, true /* useBigEndian */,
             0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
    // Serial.printf("Time used: %lums\n", millis() - start);
    //gfx->setTextColor(BLUE);  //if displaying the logo, write in Blue in a box!
    //  gfx->setTextBound(80, 80, 400, 400); // a test
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  gfx->println("");  // or it starts outside text bound??
  gfx->print("  SD Card Type: ");
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

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  gfx->printf("SD Card Size: %lluMB\n", cardSize);
  // Serial.println("*** SD card contents  (to three levels) ***");

  //listDir(SD, "/", 3);
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

//************ Music stuff  Purely to explore if the ESP32 has any spare capacity while doing display etc!
//***

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


void open_new_song(String dir, String filename) {
  Serial.print(" Open _NEW song..");
  Serial.println(filename);
  //music_info.name = filename.substring(1, filename.indexOf("."));
  //Serial.print(" audio input..");
  //Serial.println(music_info.name);
  String FullName;
  FullName = dir + filename;
  //Serial.print("** Audio selected<");Serial.print(FullName);Serial.println(">");
  audio.connecttoFS(SD, FullName.c_str());
  //music_info.runtime = audio.getAudioCurrentTime();
  //music_info.length = audio.getAudioFileDuration();
  //music_info.volume = audio.getVolume();
  //music_info.status = 1;
  //music_info.m = music_info.length / 60;
  //music_info.s = music_info.length % 60;
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
