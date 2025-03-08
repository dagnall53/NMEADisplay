/*******************************************************************************

From scratch build! 
 for keyboard  trying to use elements of https://github.com/fbiego/esp32-touch-keyboard/tree/main
*/
// * Start of Arduino_GFX setting
// get TL NMEA0183 library stuff for when / if its needed
#include <NMEA0183.h>
#include <NMEA0183Msg.h>
#include <NMEA0183Messages.h>

//******  Wifi stuff

#include <WiFi.h>
#include <WiFiUdp.h>
//
#include "ESP_NOW_files.h"
#include <esp_now.h>
#include <esp_wifi.h>
// #include <WebServer.h>
// #include <ESPmDNS.h>
IPAddress udp_ap(0, 0, 0, 0);   // the IP address to send UDP data in SoftAP mode
IPAddress udp_st(0, 0, 0, 0);   // the IP address to send UDP data in Station mode
IPAddress sta_ip(0, 0, 0, 0);   // the IP address (or received from DHCP if 0.0.0.0) in Station mode
IPAddress gateway(0, 0, 0, 0);  // the IP address for Gateway in Station mode
IPAddress subnet(0, 0, 0, 0);   // the SubNet Mask in Station mode
boolean IsConnected = false;    // used when MAIN_MODE == AP_AND_STA to flag connection success (got IP)
WiFiUDP Udp;
#define BufferLength 500
//bool line_1;
char nmea_1[500];  //serial

//bool line_U = false;
char nmea_U[BufferLength];  // NMEA buffer for UDP input port
//int nmea_UpacketSize;
// *************  ESP-NOW variables and functions in ESP_NOW_files************
//bool line_EXT;
char nmea_EXT[500];
bool EspNowIsRunning = false;

char* pTOKEN;


#include <Arduino_GFX_Library.h>
// Original version was for GFX 1.3.1 only. #include "GUITIONESP32-S3-4848S040_GFX_133.h"
#include "Esp32_4inch.h"  // may need to save /documents Arduino_RGB_Display.cpp and .h to ...Documents\Arduino\libraries\GFX_Library_for_Arduino

//For SD card (see display page -98 for test)

#include <SD.h>  // pins set in GFX .h
#include "SPI.h"
#include "FS.h"
//jpeg
#include "JpegFunc.h"
#define JPEG_FILENAME_LOGO "/logo.jpg"  // logo in jpg on sd card
//audio
#include "Audio.h"
#define AUDIO_FILENAME_01 "/HotelCalifornia.mp3"
#define AUDIO_FILENAME_02 "/SoundofSilence.mp3"
#define AUDIO_FILENAME_03 "/MoonlightBay.mp3"
#define AUDIO_FILENAME_START "/StartSound.mp3"
//String file_list[20];  //for MP3 player part https://github.com/VolosR/MakePythonLCDMP3/blob/main/MakePythonLCDMP3.ino#L119
char  File_List[20][20]; //array instead..
String runtimes[20];
int file_num = 0;
//int file_index = 0;
bool started = 0;
int counter = 0;


//*********** for keyboard*************
#include "Keyboard.h"
#include "Touch.h"
TAMC_GT911 ts = TAMC_GT911(TOUCH_SDA, TOUCH_SCL, TOUCH_INT, TOUCH_RST, TOUCH_WIDTH, TOUCH_HEIGHT);
/*******************************************************************************
 * Touch settings- 
 ******************************************************************************/
// #include <TAMC_GT911.h>
// #define TOUCH_ROTATION ROTATION_NORMAL
// #define TOUCH_MAP_X1 480
// #define TOUCH_MAP_X2 0
// #define TOUCH_MAP_Y1 480
// #define TOUCH_MAP_Y2 0

// TAMC_GT911 ts = TAMC_GT911(TOUCH_SDA, TOUCH_SCL, TOUCH_INT, TOUCH_RST, TOUCH_WIDTH, TOUCH_HEIGHT);



#include <EEPROM.h>
#include "fonts.h"
#include "FreeMonoBold8pt7b.h"
#include "FreeMonoBold27pt7b.h"
#include "FreeSansBold30pt7b.h"
#include "FreeSansBold50pt7b.h"
//**   structures and helper functions for my variables

#include "Structures.h"
#include "aux_functions.h"
#include "SineCos.h"


//set up Audio
Audio audio;

tBoatData BoatData;  // BoatData values, int double etc


bool dataUpdated;  // flag that Nmea Data has been updated

// change key (first parameter) to set defaults
MySettings Default_Settings = { 6, 0, "GUESTBOAT", "12345678", "2002", true, true, true };
MySettings Saved_Settings;
MySettings Current_Settings;
struct Displaysettings {
  bool keyboard, CurrentSettings;
  bool NmeaDepth, nmeawind, nmeaspeed, GPS;
};
Displaysettings Display_default = { false, true, false, false, false, false };
Displaysettings Display_Setting;

int MasterFont;  //global for font! Idea is to use to reset font after 'temporary' seletion of another fone

struct BarChart {
  int topleftx, toplefty, width, height, bordersize, value, rangemin, rangemax, visible;
  uint16_t barcolour;
};

struct Button {
  int h, v, width, height, bordersize;
  uint16_t backcol, textcol, BorderColor;
  int Font;                  //-1 == not forced
  bool Keypressed;           //used by keypressed
  unsigned long LastDetect;  //used by keypressed
};
Button defaultbutton = { 20, 20, 75, 75, 2, BLACK, WHITE, BLUE, -1, false, 0 };

struct TouchMemory {
  int consecutive;
  unsigned long sampletime;
  uint8_t x;
  uint8_t y;
  uint8_t size;
  int X3Swipe;
  int Y3Swipe;
};
TouchMemory TouchData;

int _null, _temp;  //null pointers




String Fontname;
int text_height = 12;      //so we can get them if we change heights etc inside functions
int text_offset = 12;      //offset is not equal to height, as subscripts print lower than 'height'
int text_char_width = 12;  // useful for monotype? only NOT USED YET! Try gfx->getTextBounds(string, x, y, &x1, &y1, &w, &h);


//colour order  background, text, border
// int h, v, width, height, bordersize;
//  uint16_t backcol, textcol, BorderColor;
Button TopLeftbutton = { 0, 0, 75, 75, 5, BLUE, WHITE, BLACK };          //false,0};
Button TopRightbutton = { 405, 0, 75, 75, 5, BLUE, WHITE, BLACK };       //,false,0};
Button BottomRightbutton = { 405, 405, 75, 75, 5, BLUE, WHITE, BLACK };  //,false,0};
Button BottomLeftbutton = { 0, 405, 75, 75, 5, BLUE, WHITE, BLACK };     //false,0};

// buttons for the wifi/settings pages // no need to set the bools ?
Button TOPButton = { 30, 10, 420, 35, 5, WHITE, BLACK, BLUE };
Button SSID = { 30, 50, 420, 35, 5, WHITE, BLACK, BLUE };      // was ,false};
Button PASSWORD = { 30, 90, 420, 35, 5, WHITE, BLACK, BLUE };  // was ,false};
Button UDPPORT = { 30, 130, 420, 35, 5, WHITE, BLACK, BLUE };  // was ,false};
#define sw_width 70
Button Switch1 = { 20, 180, sw_width, 35, 5, WHITE, BLACK, BLUE };   // was ,false};
Button Switch2 = { 105, 180, sw_width, 35, 5, WHITE, BLACK, BLUE };  // was ,false};
Button Switch3 = { 190, 180, sw_width, 35, 5, WHITE, BLACK, BLUE };  // was ,false};
Button Switch4 = { 345, 180, 120, 35, 5, WHITE, BLACK, BLUE };       // was ,false};
Button Spare = { 30, 170, 420, 35, 5, WHITE, BLACK, BLUE };          // was ,false};
Button Terminal = { 0, 240, 480, 240, 0, WHITE, BLACK, BLUE };       // was ,false};
//for selections
Button Full0Center = { 80, 55, 320, 55, 5, BLUE, WHITE, BLACK };   // ,false,0};
Button Full1Center = { 80, 125, 320, 55, 5, BLUE, WHITE, BLACK };  // was ,false};;
Button Full2Center = { 80, 195, 320, 55, 5, BLUE, WHITE, BLACK };  // was ,false};;
Button Full3Center = { 80, 265, 320, 55, 5, BLUE, WHITE, BLACK };  // was ,false};;
Button Full4Center = { 80, 335, 320, 55, 5, BLUE, WHITE, BLACK };  // was ,false};;
Button Full5Center = { 80, 405, 320, 55, 5, BLUE, WHITE, BLACK };  // was ,false};;

// for the quarter screens
Button topLeftquarter = { 0, 0, 235, 235, 5, BLUE, WHITE, BLACK };
Button bottomLeftquarter = { 0, 240, 235, 235, 5, BLUE, WHITE, BLACK };
Button topRightquarter = { 240, 0, 235, 235, 5, BLUE, WHITE, BLACK };
Button bottomRightquarter = { 240, 240, 235, 235, 5, BLUE, WHITE, BLACK };

#define On_Off ? "ON" : "OFF"  // if 1 first case else second (0 or off)



// Draw the compass pointer at an angle in degrees
void DrawCompass(int x, int y, int rad) {
  //Work in progress!
  int Start, dot, colorbar, bigtick;
  // gfx->drawRect(x-rad,y-rad,rad*2,rad*2,BLACK);gfx->drawCircle(x,y,rad,WHITE);gfx->drawCircle(x,y,rad-1,Blue);gfx->drawCircle(x,y,Start,Blue);
  Start = rad * 0.83;     //200
  dot = rad * 0.86;       //208
  colorbar = rad * 0.91;  //220
  bigtick = rad - 1;      //239
  gfx->fillRect(x - rad, y - rad, rad * 2, rad * 2, BLACK);
  gfx->fillCircle(x, y, rad, WHITE);
  gfx->fillCircle(x, y, rad - 1, BLUE);
  gfx->fillCircle(x, y, Start, BLACK);
  //rad =240 example dot is 200 to 208   bar is 200 to 239 wind colours 200 to 220
  // colour segments
  gfx->fillArc(x, y, colorbar, Start, 270 - 45, 270, RED);
  gfx->fillArc(x, y, colorbar, Start, 270, 270 + 45, GREEN);
  //Mark 12 linesarks at 30 degrees
  for (int i = 0; i < (360 / 30); i++) { gfx->fillArc(x, y, bigtick, Start, i * 30, (i * 30) + 1, BLACK); }  //239 to 200
  for (int i = 0; i < (360 / 10); i++) { gfx->fillArc(x, y, dot, Start, i * 10, (i * 10) + 1, BLACK); }      // dots at 10 degrees
}


void drawCompassPointer(int x, int y, int rad, int angle, uint16_t COLOUR, bool to) {
  int x_offset, y_offset, tail1x, tail1y, tail2x, tail2y;  // The resulting offsets from the center point
                                                           // Normalize the angle to the range 0 to 359
  float inner, outer;


  while (angle < 0) angle += 360;
  while (angle > 359) angle -= 360;
  // rose radius is default at 100 and returns 100*sine.. so may need to make offsets 2x bigger for 200 size triangel ?
  //if (rad=240){inner=0.6;outer=2;}else{inner=0.3;outer=1;}
  inner = (rad * 98) / 400;  // just a tiny bit smaller!
  outer = (rad * 98) / 120;

  // these print a big triangle
  if (to) {  //Point to boat from wind
    x_offset = x + ((outer * getSine(angle)) / 100);
    y_offset = y + ((outer * getCosine(angle)) / 100);
    tail1x = x + ((inner * getSine(angle + 20)) / 100);
    tail1y = y + ((inner * getCosine(angle + 20)) / 100);
    tail2x = x + ((inner * getSine(angle - 20)) / 100);
    tail2y = y + ((inner * getCosine(angle - 20)) / 100);
  } else {
    x_offset = x + ((inner * getSine(angle)) / 100);
    y_offset = y + ((inner * getCosine(angle)) / 100);
    tail1x = x + ((outer * getSine(angle + 7)) / 100);
    tail1y = y + ((outer * getCosine(angle + 7)) / 100);
    tail2x = x + ((outer * getSine(angle - 7)) / 100);
    tail2y = y + ((outer * getCosine(angle - 7)) / 100);
  }
  // gfx->drawLine(x_offset, y_offset,(tail1x+tail2x)/2, (tail1y+tail2y)/2,BLACK);
  gfx->fillTriangle(x_offset, y_offset, tail1x, tail1y, tail2x, tail2y, COLOUR);
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
  gfx->print(" Page:<");
  gfx->print(A.DisplayPage);
  gfx->print(">    IP:");
  sta_ip = WiFi.localIP();
  udp_st = Get_UDP_IP(sta_ip, subnet);
  gfx->print(WiFi.localIP());
  // Serial2.print("   UDP broadcasting: ");  // for if we wish to send UDP data (later! if I add autopilot controls)
  //Serial2.println(udp_st);
  gfx->print(" RSSI: ");
  gfx->print(rssiValue);
  IsConnected = true;
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
  static int FileIndex; // static to hold after selection and before pressing play!
  char Tempchar[30];
  String tempstring;
  int FS = 1;  // for font size test
  int tempint;
  if (page != LastPageselected) { RunSetup = true; }
  //generic stuff  for ALL pages
  if (RunSetup) {
    gfx->fillScreen(BLUE);
    gfx->setTextColor(WHITE);
    setFont(0);
  }
  // add any generic stuff here like check for left right?

  // Now specific stuff for each page

  switch (page) {
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

      // if (Swipe2(SwipeTestLR, 1, 10, true, SwipeTestUD, 0, 10, false, false)) {
      //   Serial.printf(" LR updated %i UD updated %i \n", SwipeTestLR, SwipeTestUD);  // swipe page using (current) TouchData information with LR (true) swipe
      //   GFXBoxPrintf(0, 0, 1, "SwipeTestLR (%i)", SwipeTestLR);
      //   GFXBoxPrintf(0, 480 - text_height, 1, "SwipeTestUD (%i)", SwipeTestUD);
      // };

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
      if (CheckButton(TopLeftbutton)) {
        volume = volume + 1;
        if (volume > 21) { volume = 21; };
        Serial.println("LEFT");
        GFXBoxPrintf(80, 0, 2, WHITE, BLUE, " LEFT   %i\n", volume);
        audio.setVolume(volume);
      }
      if (CheckButton(TopRightbutton)) {
        volume = volume - 1;
        if (volume < 1) { volume = 0; };
        Serial.println("RIGHT");
        GFXBoxPrintf(80, 0, 2, WHITE, BLUE, " RIGHT  %i\n", volume);
        audio.setVolume(volume);
      }
      if (!audio.isRunning()) { audio.connecttoFS(SD, AUDIO_FILENAME_02); }
      if (CheckButton(Full0Center)) { Current_Settings.DisplayPage = 0; }

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
        GFXBorderBoxPrintf(0, 0, 480, 75, 5, BLUE, WHITE, BLACK, "FONT:%i name%s", fontlocal, Tempchar);
        setFont(fontlocal);
        GFXBorderBoxPrintf(0, 150, 480, 330, 5, BLUE, WHITE, BLUE, "Test %4.2f height<%i>", temp, text_height);
      }
      if (CheckButton(Full0Center)) { Current_Settings.DisplayPage = 0; }

      if (CheckButton(BottomLeftbutton)) { fontlocal = fontlocal - 1; }
      if (CheckButton(BottomRightbutton)) { fontlocal = fontlocal + 1; }
      break;

    case -9:  // Play with the audio ..NOTE  Needs the resistors resoldered to connect to the audio chip on the Guitron (mains relay type) version!! 
      if (RunSetup || DataChanged) {
        setFont(4);        
        gfx->setTextSize(1); 
        if (volume > 21) { volume = 21; };
        GFXBorderBoxPrintf(TOPButton, "SD Contents / Run Audio vol%i", volume);
        gfx->setTextColor(WHITE, BLUE);
        GFXBorderBoxPrintf(BottomLeftbutton, "vol-");
        GFXBorderBoxPrintf(BottomRightbutton, "vol+");
        gfx->setCursor(0, 80);
                                  
        int file_num;// = get_music_list(SD, "/", 0, file_list);  // see https://github.com/VolosR/MakePythonLCDMP3/blob/main/MakePythonLCDMP3.ino#L101
        file_num = get_music_list(SD, "/", 0);  // Char array  based version of  https://github.com/VolosR/MakePythonLCDMP3/blob/main/MakePythonLCDMP3.ino#L101
       
        //  listDir(SD, "/", 0); 
        int localy = 0;
        for (int i = 0; i < file_num; i++) {
          gfx->setCursor(0, 118 + localy);
          gfx->printf("  %i = ",i);gfx->println(File_List[i]);
          Serial.println(File_List[i]);
          localy = localy + text_height;
        }
        GFXBorderBoxPrintf(SSID, "Found %i files", file_num);
        DataChanged = false;
      }
      if (CheckButton(TOPButton)) {Serial.printf("selected %i  %s \n",FileIndex,File_List[FileIndex]);
         open_new_song(File_List[FileIndex]);}

      if ((ts.isTouched) && (ts.points[0].y >= 118)) {  // nb check on location on screenor it will get reset when you press one of the boxes
        TouchCrosshair(1);
        FileIndex = ((ts.points[0].y - 118) / text_height) - 1;
        Serial.printf(" pointing at %i %s\n",FileIndex,File_List[FileIndex]);
        GFXBorderBoxPrintf(SSID,"Select %i %s?",FileIndex,File_List[FileIndex]);  // lots of trouble hewe with the string ! needs to be proper char string arrays?
        
      }
      // if (!audio.isRunning()) {  open_new_song(file_list[file_index]);audio.connecttoFS(SD, AUDIO_FILENAME_02); }
  
      if (CheckButton(Full0Center)) { Current_Settings.DisplayPage = 0; }
      if (CheckButton(TopLeftbutton)) { Current_Settings.DisplayPage = 0; }
      if (CheckButton(TopRightbutton)) { Current_Settings.DisplayPage = 0; }
      if (CheckButton(BottomRightbutton)) {
        volume = volume + 1;
        if (volume > 21) { volume = 21; };
        audio.setVolume(volume);
        //Serial.printf("Right %i and ", volume);
        //Serial.println(volume);
        DataChanged = true;
      }
      if (CheckButton(BottomLeftbutton)) {
        volume = volume - 1;
        if (volume < 1) { volume = 0; };
        // Serial.printf("LEFT %i  and ", volume);
        //Serial.println(volume);
        audio.setVolume(volume);
        DataChanged = true;
      }


      break;

    case -5:  ///Wifiscan
      if (RunSetup) {
        setFont(4);
        GFXBorderBoxPrintf(TOPButton, "Current <%s>", Current_Settings.ssid);
        GFXBorderBoxPrintf(SSID, "SCANNING...");
        gfx->setCursor(0, 140);  //(location of terminal. make better later!)
        int n = WiFi.scanNetworks();
        GFXBorderBoxPrintf(SSID, "scan done");
        if (n == 0) {
          GFXBorderBoxPrintf(SSID, "no networks found");
        } else {
          GFXBorderBoxPrintf(SSID, " %i networks found", n);
          for (int i = 0; i < n; ++i) {
            gfx->setCursor(0, 200 + ((i + 1) * text_height));
            // Print SSID and RSSI for each network found
            gfx->print(i + 1);
            gfx->print(": ");
            gfx->print(WiFi.SSID(i));
            gfx->print(" (");
            gfx->print(WiFi.RSSI(i));
            gfx->println(")");
            //    gfx->println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
            delay(10);
          }
        }
      }
      if (millis() > slowdown + 1000) {
        slowdown = millis();
        //other stuff?
      }
      if ((ts.isTouched) && (ts.points[0].y >= 200)) {  // nb check on location on screenor it will get reset when you press one of the boxes
        //TouchCrosshair(1);
        wifissidpointer = ((ts.points[0].y - 200) / text_height) - 1;
        int str_len = WiFi.SSID(wifissidpointer).length() + 1;
        char result[str_len];
        if (str_len << sizeof(Current_Settings.ssid)) {  //-1 check small enough for our ssid register!
          WiFi.SSID(wifissidpointer).toCharArray(result, 15);
          //  WiFi.SSID(wifissidpointer).toCharArray(Current_Settings.ssid,str_len);
          Serial.printf(" touched at %i  equates to %i ? %s \n", ts.points[0].y, tempint, WiFi.SSID(wifissidpointer));
          GFXBorderBoxPrintf(SSID, "use<%s>?", result);
        } else {
          GFXBorderBoxPrintf(SSID, "ssid too long ");
        }
      }
      if (CheckButton(SSID)) {
        WiFi.SSID(wifissidpointer).toCharArray(Current_Settings.ssid, 15);
        Serial.printf(" Updating ssid to <%s>", Current_Settings.ssid);
        Current_Settings.DisplayPage = -1;
      }
      if (CheckButton(TOPButton)) { Current_Settings.DisplayPage = -2; }
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

      if (CheckButton(Full0Center)) { Current_Settings.DisplayPage = -1; }
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

      if (CheckButton(Full0Center)) { Current_Settings.DisplayPage = -1; }
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
      if (CheckButton(Full0Center)) { Current_Settings.DisplayPage = -1; }
      if (CheckButton(TopRightbutton)) { Current_Settings.DisplayPage = -5; }

      break;

    case -1:  // this is the main WIFI settings page
      if (RunSetup || DataChanged) {
        gfx->fillScreen(BLACK);
        gfx->setTextColor(WHITE, BLACK);
        gfx->setTextSize(1);
        ShowToplinesettings(Saved_Settings, "SAVED");
        setFont(4);
        gfx->setCursor(180, 180);
        GFXBorderBoxPrintf(SSID, "Set SSID <%s>", Current_Settings.ssid);
        GFXBorderBoxPrintf(PASSWORD, "Set Password <%s>", Current_Settings.password);
        GFXBorderBoxPrintf(UDPPORT, "Set UDP Port <%s>", Current_Settings.UDP_PORT);
        GFXBorderBoxPrintf(Switch1, Current_Settings.Serial_on On_Off);  //A.Serial_on On_Off,  A.UDP_ON On_Off, A.ESP_NOW_ON On_Off
        AddTitleBorderBox(Switch1, "Serial");
        GFXBorderBoxPrintf(Switch2, Current_Settings.UDP_ON On_Off);
        AddTitleBorderBox(Switch2, "UDP");
        GFXBorderBoxPrintf(Switch3, Current_Settings.ESP_NOW_ON On_Off);
        AddTitleBorderBox(Switch3, "ESP-Now");
        GFXBorderBoxPrintf(Switch4, "SAVE");
        AddTitleBorderBox(Switch4, "EEPROM");
        GFXBorderBoxPrintf(Terminal, "- NMEA DATA -");
        AddTitleBorderBox(Terminal, "TERMINAL");
        setFont(0);
        DataChanged = false;
        //while (ts.sTouched{yield(); Serial.println("yeilding -1");}
      }
      if (millis() > slowdown + 1000) {
        slowdown = millis();

        //print recieved data in terminal here - or in useNMEA()
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
        Current_Settings.DisplayPage = 0;
        ;
      };
      if (CheckButton(TOPButton)) { Current_Settings.DisplayPage = 0; }
      if (CheckButton(Full0Center)) { Current_Settings.DisplayPage = 0; }
      if (CheckButton(SSID)) { Current_Settings.DisplayPage = -5; };
      if (CheckButton(PASSWORD)) { Current_Settings.DisplayPage = -3; };
      if (CheckButton(UDPPORT)) { Current_Settings.DisplayPage = -4; };
      break;
    case 0:
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
      if (CheckButton(Full0Center)) { Current_Settings.DisplayPage = 0; }
      if (CheckButton(Full1Center)) { Current_Settings.DisplayPage = -9; }
      if (CheckButton(Full2Center)) { Current_Settings.DisplayPage = -1; }
      if (CheckButton(Full3Center)) { Current_Settings.DisplayPage = 4; }
      if (CheckButton(Full4Center)) { Current_Settings.DisplayPage = -10; }
      if (CheckButton(Full5Center)) {
        Current_Settings.DisplayPage = 4;
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
        UpdateCentered(topLeftquarter, "%4.2fkts", BoatData.STW);
        UpdateCentered(bottomLeftquarter, "%4.1fm", BoatData.WaterDepth);  // add units?
                                                                           //UpdateCentered(topRightquarter  "c%3.2F",wind*1.1);
        UpdateCentered(bottomRightquarter, "%3.2FKts", BoatData.SOG);
      }
      // if (CheckButton(Full0Center)) { Current_Settings.DisplayPage = 0; }  // first so it has priority!
      // if (CheckButton(Full1Center)) { Current_Settings.DisplayPage = 0; }  //easier to hit two
      //        TouchCrosshair(20); quarters select big screens
      if (CheckButton(topLeftquarter)) { Current_Settings.DisplayPage = 6; }
      if (CheckButton(bottomLeftquarter)) { Current_Settings.DisplayPage = 7; }
      if (CheckButton(topRightquarter)) { Current_Settings.DisplayPage = 5; }
      if (CheckButton(bottomRightquarter)) { Current_Settings.DisplayPage = 8; }


      // if (CheckButton(TopLeftbutton)) { Current_Settings.DisplayPage = Current_Settings.DisplayPage - 1; }
      // if (CheckButton(TopRightbutton)) { Current_Settings.DisplayPage = 1; }  //loop to page 1


      break;

    case 5:  // wind instrument
      if (RunSetup) {
        setFont(6);
        DrawCompass(240, 240, 240);
      }
      if (millis() > slowdown + 500) {
        slowdown = millis();
        WindArrow(240, 240, 240, BoatData.WindDirectionT, false);
        //Box in middle for wind dir / speed
        //int h, int v, int width, int height, int textsize, int bordersize, uint16_t backgroundcol, uint16_t textcol, uint16_t BorderColor, const char* fmt, ...) {  //Print in a box.(h,v,width,height,textsize,bordersize,backgroundcol,textcol,BorderColor, const char* fmt, ...)
        GFXBorderBoxPrintf(240 - 70, 240 - 40, 140, 80, 5, BLACK, WHITE, BLACK, "%2.0fkts", BoatData.WindSpeedK);
      }

      //        TouchCrosshair(20); quarters select big screens
      if (CheckButton(topLeftquarter)) { Current_Settings.DisplayPage = 4; }
      if (CheckButton(bottomLeftquarter)) { Current_Settings.DisplayPage = 4; }
      if (CheckButton(topRightquarter)) { Current_Settings.DisplayPage = 0; }
      if (CheckButton(bottomRightquarter)) { Current_Settings.DisplayPage = 4; }

      // if (CheckButton(TopLeftbutton)) { Current_Settings.DisplayPage = Current_Settings.DisplayPage - 1; }
      // if (CheckButton(TopRightbutton)) { Current_Settings.DisplayPage = 4; }  //loop to page 1


      break;
    case 6:
      if (RunSetup) {
        setFont(10);
        //GFXBoxPrintf(0,480-(text_height*2),  2, "PAGE 2 -SPEED");
        GFXBorderBoxPrintf(20, 100, 440, 360, 5, BLUE, BLACK, BLACK, "STW");
        AddTitleBorderBox(20, 100, BLACK, "STW");
      }
      if (millis() > slowdown + 200) {
        //gfx->fillScreen(BLUE);
        slowdown = millis();
        // ShowToplinesettings("Current");

        UpdateCentered(20, 100, 440, 360, 5, BLUE, BLACK, BLACK, "%4.2fkts", BoatData.STW);
      }
      //  if (CheckButton(Full0Center)) { Current_Settings.DisplayPage = 4; }
      //        TouchCrosshair(20); quarters select big screens
      if (CheckButton(topLeftquarter)) { Current_Settings.DisplayPage = 9; }
      if (CheckButton(bottomLeftquarter)) { Current_Settings.DisplayPage = 4; }
      if (CheckButton(topRightquarter)) { Current_Settings.DisplayPage = 4; }
      if (CheckButton(bottomRightquarter)) { Current_Settings.DisplayPage = 4; }

      break;
    case 7:
      if (RunSetup) {
        setFont(10);
        //GFXBoxPrintf(0,480-(text_height*2),  2, "PAGE 2 -SPEED");
        GFXBorderBoxPrintf(20, 100, 440, 360, 5, BLUE, BLACK, BLACK, "DEPTH");
        AddTitleBorderBox(20, 100, BLACK, "DEPTH");
      }
      if (millis() > slowdown + 200) {
        //gfx->fillScreen(BLUE);
        slowdown = millis();
        UpdateCentered(20, 100, 440, 360, 5, BLUE, BLACK, BLACK, "%4.1fm", BoatData.WaterDepth);
      }
      if (CheckButton(Full0Center)) { Current_Settings.DisplayPage = 4; }
      //        TouchCrosshair(20); quarters select big screens
      if (CheckButton(topLeftquarter)) { Current_Settings.DisplayPage = 4; }
      if (CheckButton(bottomLeftquarter)) { Current_Settings.DisplayPage = 11; }
      if (CheckButton(topRightquarter)) { Current_Settings.DisplayPage = 4; }
      if (CheckButton(bottomRightquarter)) { Current_Settings.DisplayPage = 4; }

      break;
    case 8:
      if (RunSetup) {
        setFont(10);

        GFXBorderBoxPrintf(20, 100, 440, 360, 5, BLUE, BLACK, BLACK, "SOG");
        AddTitleBorderBox(20, 100, BLACK, "SOG");
      }
      if (millis() > slowdown + 200) {
        //gfx->fillScreen(BLUE);
        slowdown = millis();
        UpdateCentered(20, 100, 440, 360, 5, BLUE, BLACK, BLACK, "%3.1Fkts", BoatData.SOG);
      }
      //        TouchCrosshair(20); quarters select big screens
      if (CheckButton(topLeftquarter)) { Current_Settings.DisplayPage = 4; }
      if (CheckButton(bottomLeftquarter)) { Current_Settings.DisplayPage = 4; }
      if (CheckButton(topRightquarter)) { Current_Settings.DisplayPage = 4; }
      if (CheckButton(bottomRightquarter)) { Current_Settings.DisplayPage = 12; }

      break;

    case 9:
      if (RunSetup) {
        setFont(8);
        GFXBorderBoxPrintf(20, 100, 440, 360, 5, BLUE, BLACK, BLACK, "");

        AddTitleBorderBox(20, 130, BLACK, "GPS Time");
        AddTitleBorderBox(20, 230, BLACK, "LATITUDE DD.ddd");
        AddTitleBorderBox(20, 330, BLACK, "LONGITUDE DD.ddd");
      }
      if (millis() > slowdown + 200) {
        slowdown = millis();
        if (BoatData.GPSTime != NMEA0183DoubleNA) { UpdateCentered(20, 130, 440, 80, 5, BLUE, WHITE, BLACK, "%02i:%02i:%02i",
                                                                   int(BoatData.GPSTime) / 3600, (int(BoatData.GPSTime) % 3600) / 60, (int(BoatData.GPSTime) % 3600) % 60); }
        if (BoatData.Latitude != NMEA0183DoubleNA) { UpdateCentered(20, 230, 440, 80, 5, BLUE, WHITE, BLACK, "%f", BoatData.Latitude); }
        if (BoatData.Longitude != NMEA0183DoubleNA) { UpdateCentered(20, 330, 440, 80, 5, BLUE, WHITE, BLACK, "%f", BoatData.Longitude); }
      }
      //if (CheckButton(Full0Center)) { Current_Settings.DisplayPage = 4; }
      if (CheckButton(topLeftquarter)) { Current_Settings.DisplayPage = 10; }
      if (CheckButton(bottomLeftquarter)) { Current_Settings.DisplayPage = 4; }  //Loop to the main settings page
      if (CheckButton(topRightquarter)) { Current_Settings.DisplayPage = 4; }
      if (CheckButton(bottomRightquarter)) { Current_Settings.DisplayPage = 4; }  // test! check default loops back to 0

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
      Current_Settings.DisplayPage = 0;

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
#ifdef GFX_BL
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);
#endif
  gfx->setCursor(0, 50);
  setFont(0);
  gfx->setTextColor(WHITE);
  EEPROM_READ();  // setup and read Saved_Settings (saved variables)
  Current_Settings = Saved_Settings;
  ConnectWiFiusingCurrentSettings();

  if (WiFi.status() == WL_CONNECTED) {
    gfx->println(" Connected ! ");
    Serial.print("** Connected.: ");
  }



  gfx->println(" Using :");
  gfx->print(WiFi.SSID());
  gfx->print(" ");
  gfx->println(WiFi.localIP());
  Serial.print(" *Running with:  ssid<");
  Serial.print(WiFi.SSID());
  Serial.print(">  Ip:");
  Serial.println(WiFi.localIP());
  Start_ESP_EXT();  //  Sets esp_now links to the current WiFi.channel etc.
  SD_Setup();
  Audio_setup();
  keyboard(-1);  //reset keyboard display update settings
  gfx->println(F("***START Screen***"));
  Udp.begin(atoi(Current_Settings.UDP_PORT));
  delay(100);    // .1 seconds
  Display(100);  // trigger default
  // while(1){ // trying to avoid RTOS
  // ts.read();
  // Display(Current_Settings.DisplayPage);  //EventTiming("STOP");
  // CheckAndUseInputs();
  // audio.loop();   //
  // vTaskDelay(1);
  // }
}

void loop() {
  yield();
  //
  //DisplayCheck(false);
  //EventTiming("START");
  delay(1);
  ts.read();
  CheckAndUseInputs();
  Display(Current_Settings.DisplayPage);  //EventTiming("STOP");
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


void DisplayCheck(bool invertcheck) {  //used to test colours are as expected
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






//*********** Touch stuff ****************

int swipedir(int sampleA, int sampleB, int range) {
  int tristate;
  tristate = 0;
  if (SwipedFarEnough(sampleA, sampleB, range)) {
    if ((sampleB - sampleA) > 0) {
      tristate = 1;
    } else {
      tristate = -1;
    }
  }
  return tristate;
}

bool SwipedFarEnough(int sampleA, int sampleB, int range) {  // separated so I can debug!
  int A = sampleB - sampleA;
  return (abs(A) >= range);
}

bool SwipedFarEnough(TouchMemory sampleA, TouchMemory sampleB, int range) {
  int H, V;
  H = sampleB.x - sampleA.x;
  V = sampleB.y - sampleA.y;
  //Serial.printf(" SFE [ H(%i)  V(%i),  %i ]",abs(sampleB.x - sampleA.x),abs(sampleB.y - sampleA.y),range );
  return ((abs(H) >= range) || abs(V) >= range);
}

// bool CheckButton(Button button){
//   static unsigned long LastDetect;
//   static bool keynoted;
//   ///bool XYinBox(int touchx,int touchy, int h,int v,int width,int height){
//     return (XYinBox(ts.points[0].x, ts.points[0].y,button.h,button.v,button.width,button.height) );
//   }
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


bool IncrementInRange(int _increment, int& Variable, int Varmin, int Varmax, bool Cycle_Round) {  // use wwith swipedir (-1.0,+1)
  int _var;
  //bool altered = true;
  if (_increment == 0) { return false; }
  _var = Variable + _increment;
  if (Cycle_Round) {
    if (_var >= Varmax + 1) { _var = Varmin; }
    if (_var <= Varmin - 1) { _var = Varmax; }
  } else {  // limit at max min
    if (_var >= Varmax + 1) { _var = Varmax; }
    if (_var <= Varmin - 1) { _var = Varmin; }
  }
  Variable = _var;
  return true;
}

bool Swipe2(int& _LRvariable, int LRvarmin, int LRvarmax, bool LRCycle_Round,
            int& _UDvariable, int UDvarmin, int UDvarmax, bool UDCycle_Round, bool TopScreenOnly) {
  static TouchMemory Startingpoint, Touch_snapshot;
  static bool SwipeFound, SwipeStart, MidSwipeSampled;
  static unsigned long SwipeStartTime, SwipeFoundSent_atTime, timenow, SwipeCleared_atTime;
  int Hincrement, Vincrement;
#define minDist_Pixels 30
#define minSwipe_TestTime_ms 150

  //int Looking_at;  // horizontal/Vertical  0,1
  bool returnvalue = false;
  timenow = millis();

  if (SwipeFound && (timenow >= (SwipeFoundSent_atTime + 150))) {  // limits repetitions
    SwipeFound = false;
    MidSwipeSampled = false;  // reset so can look for new swipe
    SwipeStart = false;       //
    Serial.printf("   Timed reset after 'Found'   flags:[%i] [%i]) \n", SwipeStart, SwipeFound);
    return false;
  }

  if (SwipeStart && (timenow >= (SwipeStartTime + 1000))) {  // you have 3 sec from start to swipe. else resets start point.
    SwipeStart = false;
    MidSwipeSampled = false;
    SwipeFound = false;  // no swipe within 1500 of start - reset.
    Serial.printf("   RESET on Start timing flags [%i] [%i]) \n", SwipeStart, SwipeFound);
    return false;
  }

  if (SwipeFound) { return false; }     // stop doing anything if we have Swipe sensed already (will wait for timed reset)
  if (!ts.isTouched) { return false; }  // is never triggered!
  //************* ts must be touched to get here....*****************
  if (TopScreenOnly && (ts.points[0].y >= 180)) { return false; }
  //Capture Snapshot of touch point: in case in changes during tests
  //if (timenow <= SwipeCleared_atTime+ 200) {return false;}  // force a gap after a swipe clear. halts function!!
  if ((timenow - Touch_snapshot.sampletime) <= 20) { return false; }  // 20ms min update

  Touch_snapshot.sampletime = timenow;
  Touch_snapshot.x = ts.points[0].x;
  Touch_snapshot.y = ts.points[0].y;
  Touch_snapshot.size = ts.points[0].size;

  if (!SwipeStart) {  // capture First cross hair after reset ?
    Startingpoint = Touch_snapshot;
    // Serial.printf(" Starting at x%i y%i\n", Startingpoint.x, Startingpoint.y);
    //TouchCrosshair(10, WHITE);
    SwipeStart = true;
    SwipeFound = false;
    MidSwipeSampled = false;
    SwipeStartTime = timenow;  // so we can reset on time if we do not 'swipe'
    return false;              // we have the start point, so just return false.
  }
  //test
  //Ignore this first movement- its just to ensure swiping properly. I had sensied this, but its not consistent!
  if (SwipeStart && !MidSwipeSampled && SwipedFarEnough(Startingpoint, Touch_snapshot, minDist_Pixels)) {
    Startingpoint = Touch_snapshot;
    MidSwipeSampled = true;
    //TouchCrosshair(20, BLUE);
    return false;
  }
  if (SwipeStart && MidSwipeSampled && SwipedFarEnough(Startingpoint, Touch_snapshot, (minDist_Pixels))) {
    Hincrement = swipedir(Startingpoint.x, Touch_snapshot.x, minDist_Pixels);
    Vincrement = swipedir(Touch_snapshot.y, Startingpoint.y, minDist_Pixels);
    // only return true if we increment (change) either of the selected attributes
    // Tried evaluating seperately at mid point.. but values were NOT consistent Do H and V separately as one may not be consistent!
    // Serial.printf(" swipe evaluate  start-mid(H%i  V%i)   mid-now(H%i V%i) ",Hincrement[0],Vincrement[0],Hincrement[1],Vincrement[1]);
    if (IncrementInRange(Hincrement, _LRvariable, LRvarmin, LRvarmax, LRCycle_Round)) { returnvalue = true; }
    if (IncrementInRange(Vincrement, _UDvariable, UDvarmin, UDvarmax, UDCycle_Round)) { returnvalue = true; }

    SwipeFoundSent_atTime = timenow;
    SwipeStart = false;
    MidSwipeSampled = false;
    SwipeFound = true;  // we have gone far enough to test swipe, so start again..
                        // if (returnvalue){TouchCrosshair(20, GREEN);} else {TouchCrosshair(20, RED);}

  }  // SWIPEStart recorded, now looking for swipe > min distance (swipeFound)//
  return returnvalue;
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
boolean CompStruct(MySettings A, MySettings B) {  // does not check ssid and password
  bool same = false;
  // have to check each variable individually
  if (A.EpromKEY == B.EpromKEY) { same = true; }
  if (A.UDP_PORT == B.UDP_PORT) { same = true; }
  if (A.UDP_ON == B.UDP_ON) { same = true; }
  if (A.ESP_NOW_ON == B.ESP_NOW_ON) { same = true; }
  if (A.Serial_on == B.Serial_on) { same = true; }
  return same;
}

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

// void GFXBoxPrintf(int h, int v, const char* fmt, ...) {
//  GFXBoxPrintf(h, v, 1, fmt);
// }
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
//void GFXBoxPrintf(int h, int v, const char* fmt, ...) {  //complete object type suitable for holding the information needed by the macros va_start, va_copy, va_arg, and va_end.
//  GFXBoxPrintf(h, v,1,fmt);
//}


void GFXPrintf(int h, int v, const char* fmt, ...) {  //complete object type suitable for holding the information needed by the macros va_start, va_copy, va_arg, and va_end.
  char msg[300] = { '\0' };                           // used in message buildup
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, 128, fmt, args);
  va_end(args);
  int len = strlen(msg);
  Writeat(h, v, msg);
}

// more general versions including box width size Box draws border OUTside topleft position by 'bordersize'
//USED BY GFXBorderBoxPrintf
void WriteinBorderBox(int h, int v, int width, int height, int bordersize,
                      uint16_t backgroundcol, uint16_t textcol, uint16_t BorderColor, const char* TEXT) {  //Write text in filled box of text height at h,v (using fontoffset to use TOP LEFT of text convention)
  int16_t x, y, TBx1, TBy1;
  uint16_t TBw, TBh;
  // gfx->setTextSize(textsize);
  x = h + bordersize;
  y = v + bordersize + (text_offset);                        //initial Top Left positioning for print text
  gfx->getTextBounds(TEXT, x, y, &TBx1, &TBy1, &TBw, &TBh);  // do not forget '& ! Pointer not value!!!
  //Serial.printf(" Text bounds planned print (x%i y%i)  W:%i H:%i TB_X %i  TB_Y %i \n",x,y, TBw,TBh,TBx1,TBy1);
  //gfx->fillRect(TBx1 , TBy1 , TBw , TBh , WHITE); delay(100); // visulize what the data is!
  // move to center is  add (width-2*bordersize-TBw)/2 ?
  //move vertical is add (height -2*bordersize-TBh)/2
  gfx->fillRect(h, v, width, height, BorderColor);
  gfx->fillRect(h + bordersize, v + bordersize, width - (2 * bordersize), height - (2 * bordersize), backgroundcol);
  gfx->setTextColor(textcol);

  // offset up/down by OFFSET (!) for GFXFONTS that start at Bottom left. Standard fonts start at TOP LEFT
  x = h + bordersize;
  y = v + bordersize + (text_offset);
  x = x + ((width - TBw - (2 * bordersize)) / 2);   //try horizontal centering
  y = y + ((height - TBh - (2 * bordersize)) / 2);  //try vertical centering
  gfx->setCursor(x, y);
  gfx->print(TEXT);
  //reset font ...
  gfx->setTextSize(1);
}

//complete object type suitable for holding the information needed by the macros va_start, va_copy, va_arg, and va_end.
/* same as struct Button { int h, v, width, height, bordersize;
uint16_t backcol,textcol,barcol;
}; but button misses out textsize
*/

void AddTitleBorderBox(Button button, const char* fmt, ...) {  // add a title to the box
  int Font_Before;
  //Serial.println("Font at start is %i",MasterFont);
  Font_Before = MasterFont;
  setFont(0);
  static char Title[300] = { '\0' };
  int16_t x, y, TBx1, TBy1;
  uint16_t TBw, TBh;  // used in message buildup
  va_list args;
  va_start(args, fmt);
  vsnprintf(Title, 128, fmt, args);
  va_end(args);
  int len = strlen(Title);
  gfx->getTextBounds(Title, button.h, button.v, &TBx1, &TBy1, &TBw, &TBh);
  gfx->setTextColor(WHITE, button.BorderColor);
  if ((button.v - TBh) >= 0) {
    gfx->setCursor(button.h, button.v);
  } else {
    gfx->setCursor(button.h, button.v + TBh);
  }
  gfx->print(Title);
  setFont(Font_Before);  //Serial.println("Font selected is %i",MasterFont);
}

void AddTitleBorderBox(int h, int v, uint16_t BorderColor, const char* fmt, ...) {  // add a title to the box
  int Font_Before;
  //Serial.println("Font at start is %i",MasterFont);
  Font_Before = MasterFont;
  setFont(0);
  static char Title[300] = { '\0' };
  int16_t x, y, TBx1, TBy1;
  uint16_t TBw, TBh;  // used in message buildup
  va_list args;
  va_start(args, fmt);
  vsnprintf(Title, 128, fmt, args);
  va_end(args);
  int len = strlen(Title);
  gfx->getTextBounds(Title, h, v, &TBx1, &TBy1, &TBw, &TBh);
  gfx->setTextColor(WHITE, BorderColor);
  if ((v - TBh) >= 0) {
    gfx->setCursor(h, v);
  } else {
    gfx->setCursor(h, v + TBh);
  }
  gfx->print(Title);
  setFont(Font_Before);  //Serial.println("Font selected is %i",MasterFont);
}

// template <class ... Args>
// test ideas from  https://stackoverflow.com/questions/36881533/passing-va-list-to-other-functions
// void GFXBorderBoxPrintf(Button button,const char* fmt, Args ... args){
// or perhaps void GFXBorderBoxPrintf(Button button,const char* fmt, Args ... args)GFXBorderBoxPrintf(button,1,fmt,__VA_ARGS__)
//    GFXBorderBoxPrintf(button,1,fmt, args...);}
// void GFXBorderBoxPrintf(Button button,const char* fmt, ...){
//   GFXBorderBoxPrintf(button,1,fmt,__VA_ARGS__);}


void GFXBorderBoxPrintf(Button button, const char* fmt, ...) {
  static char msg[300] = { '\0' };  // used in message buildup
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, 128, fmt, args);
  va_end(args);
  int len = strlen(msg);
  WriteinBorderBox(button.h, button.v, button.width, button.height, button.bordersize, button.backcol, button.textcol, button.BorderColor, msg);
}

// void GFXBorderBoxPrintf(Button button, int textsize, const char* fmt, ...) {
//   static char msg[300] = { '\0' };  // used in message buildup
//   va_list args;
//   va_start(args, fmt);
//   vsnprintf(msg, 128, fmt, args);
//   va_end(args);
//   int len = strlen(msg);
//   WriteinBorderBox(button.h, button.v, button.width, button.height, button.bordersize, button.backcol, button.textcol, button.BorderColor, msg);
// }

void GFXBorderBoxPrintf(int h, int v, int width, int height, int bordersize,
                        uint16_t backgroundcol, uint16_t textcol, uint16_t BorderColor, const char* fmt, ...) {  //Print in a box.(h,v,width,height,textsize,bordersize,backgroundcol,textcol,BorderColor, const char* fmt, ...)
  static char msg[300] = { '\0' };                                                                               // used in message buildup
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, 128, fmt, args);
  va_end(args);
  int len = strlen(msg);
  WriteinBorderBox(h, v, width, height, bordersize, backgroundcol, textcol, BorderColor, msg);
}

void UpdateCentered(Button button, const char* fmt, ...) {  // Centers text in space
  static char msg[300] = { '\0' };
  //Serial.printf("h %i v %i TEXT %i  Background %i \n",button.h,button.v, button.textcol,button.backcol);
  int textsize = 1;
  // calculate new offsets to just center on original box - minimum redraw of blank
  int16_t x, y, TBx1, TBy1;
  uint16_t TBw, TBh;
  gfx->setTextSize(textsize);  // used in message buildup
  va_list args;
  va_start(args, fmt);

  vsnprintf(msg, 128, fmt, args);
  va_end(args);
  int len = strlen(msg);
  gfx->getTextBounds(msg, button.h, button.v, &TBx1, &TBy1, &TBw, &TBh);  // do not forget '& ! Pointer not value!!!
  x = button.h + button.bordersize;
  y = button.v + button.bordersize + (text_offset * textsize);
  x = x + ((button.width - TBw - (2 * button.bordersize)) / 2) / textsize;                                                    //try horizontal centering
  y = y + ((button.height - TBh - (2 * button.bordersize)) / 2) / textsize;                                                   //try vertical centering
  gfx->fillRect(button.h + button.bordersize, y - TBh - 1, button.width - (2 * button.bordersize), TBh + 4, button.backcol);  // just (plus a little bit ) where the text will be
  gfx->setTextColor(button.textcol);
  gfx->setCursor(x, y);
  gfx->print(msg);
  gfx->setTextSize(1);
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

void UpdateCentered(int h, int v, int width, int height, int bordersize,
                    uint16_t backgroundcol, uint16_t textcol, uint16_t BorderColor, const char* fmt, ...) {  //Print in a box.(h,v,width,height,textsize,bordersize,backgroundcol,textcol,BorderColor, const char* fmt, ...)
  static char msg[300] = { '\0' };
  // calculate new offsets to just center on original box - minimum redraw of blank
  int16_t x, y, TBx1, TBy1;
  uint16_t TBw, TBh;
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, 128, fmt, args);
  va_end(args);
  int len = strlen(msg);
  gfx->getTextBounds(msg, h, v, &TBx1, &TBy1, &TBw, &TBh);  // do not forget '& ! Pointer not value!!!
  x = h + bordersize;
  y = v + bordersize + (text_offset);
  x = x + ((width - TBw - (2 * bordersize)) / 2);   //try horizontal centering
  y = y + ((height - TBh - (2 * bordersize)) / 2);  //try vertical centering
                                                    // gfx->fillRect(h + bordersize, v + bordersize, width - (2 * bordersize), height - (2 * bordersize), backgroundcol);  // full inner rectangle blanking
  gfx->fillRect(h + bordersize, y - TBh - 1, width - (2 * bordersize), TBh + 4, backgroundcol);
  gfx->setTextColor(textcol);
  gfx->setCursor(x, y);
  gfx->print(msg);
  gfx->setTextSize(1);
}

void UpdateLinef(Button button, const char* fmt, ...) {  // Types sequential lines in the button space
  static int Printline;
  int LinesOfType, characters;
  int16_t x, y;
  char limitedWidth[80];
  int typingspaceH, typingspaceW;
  typingspaceH = button.height - 2 * button.bordersize;
  typingspaceW = button.width - 2 * button.bordersize;
  LinesOfType = typingspaceH / (text_height + 2);
  static char msg[300] = { '\0' };
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, 128, fmt, args);
  va_end(args);
  int len = strlen(msg);
  x = button.h + button.bordersize;
  y = button.v + button.bordersize + (text_offset);
  gfx->setCursor(x, y + (Printline * (text_height + 2)));
  gfx->print(msg);  // lines should already have CRLF!
  Printline = Printline + 1;
  if (len >= 40) { Printline = Printline + 1; }
  if (Printline >= (LinesOfType)) {
    Printline = 0;
    gfx->fillRect(x, y - (text_offset), typingspaceW, typingspaceH, WHITE);
  }
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
    unsigned long start = millis();
    jpegDraw(JPEG_FILENAME_LOGO, jpegDrawCallback, true /* useBigEndian */,
             0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
    Serial.printf("Time used: %lums\n", millis() - start);
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
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
  if (audio.connecttoFS(SD, AUDIO_FILENAME_START)) {
    gfx->setCursor(10, 100);
    delay(200);
    if (audio.isRunning()) { gfx->println("Playing Ships bells"); }
    while (audio.isRunning()) {
      audio.loop();
      vTaskDelay(1);
    }
  } else {
    gfx->setCursor(10, 100);
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
    if (Current_Settings.DisplayPage == -1) {
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
void ConnectWiFiusingCurrentSettings() {
  uint32_t StartTime = millis();
  gfx->println(" Using EEPROM settings");
  WiFi.setHostname("NMEA_Display");
  WiFi.mode(WIFI_AP_STA);
  gfx->println(" Starting WiFi");
  //MDNS.begin("NMEA_Display");
  WiFi.begin(Current_Settings.ssid, Current_Settings.password);
  while ((WiFi.status() != WL_CONNECTED) && (millis() <= StartTime + 10000)) {  //Give it 10 seconds
    delay(500);
    gfx->print(".");
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

void ScannetworkstoGFX(int x) {
  gfx->setCursor(0, x);  //(location of terminal. make better later!)
  int n = WiFi.scanNetworks();
  gfx->println("scan done");
  if (n == 0) {
    gfx->println("no networks found");
  } else {
    gfx->print(n);
    gfx->println(" networks found");
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      gfx->print(i + 1);
      gfx->print(": ");
      gfx->print(WiFi.SSID(i));
      gfx->print(" (");
      gfx->print(WiFi.RSSI(i));
      gfx->println(")");
      //    gfx->println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
      delay(10);
    }
  }
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

int get_music_list(fs::FS& fs, const char* dirname, uint8_t levels ) { // uses char* File_List[30] ?? how to pass the name here ?? 
  Serial.printf("Listing directory: %s\n", dirname);
  int i = 0;
  char temp[20];
  File root = fs.open(dirname);
  if (!root){Serial.println("Failed to open directory");return i;}
  if (!root.isDirectory()){Serial.println("Not a directory");return i;}
  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
    } else {
      strcpy(temp,file.name());
     // if (temp.endsWith(".wav")) {
    strcpy(File_List[i],temp); //, sizeof(File_List[i]));  // char array version of '=' !! with limit on size
        i++;
      //} else if (temp.endsWith(".mp3")) {
    //strcpy(File_List[i],temp); // sizeof(File_List[i]));
      //  i++;
      //}
    }
    file = root.openNextFile();
  }
  return i;
}

// void open_new_song(char filename) {
//    Serial.print(" Open _NEW song..");Serial.println (filename ); 
//   music_info.name = filename;
//   Serial.print(" audio input..");Serial.println (music_info.name );
//   audio.connecttoFS(SD, filename);  // Does this need the '/' before the filename?? 
//   music_info.runtime = audio.getAudioCurrentTime();
//   music_info.length = audio.getAudioFileDuration();
//   music_info.volume = audio.getVolume();
//   music_info.status = 1;
//   music_info.m = music_info.length / 60;
//   music_info.s = music_info.length % 60;
// }


// int get_music_list(fs::FS& fs, const char* dirname, uint8_t levels, String wavlist[30]) {
//   Serial.printf("Listing directory: %s\n", dirname);
//   int i = 0;
//   File root = fs.open(dirname);
//   if (!root) {
//     Serial.println("Failed to open directory");
//     return i;
//   }
//   if (!root.isDirectory()) {
//     Serial.println("Not a directory");
//     return i;
//   }
//   File file = root.openNextFile();
//   while (file) {
//     if (file.isDirectory()) {
//     } else {
//       String temp = file.name();
//       if (temp.endsWith(".wav")) {
//         wavlist[i] = temp;
//         i++;
//       } else if (temp.endsWith(".mp3")) {
//         wavlist[i] = temp;
//         i++;
//       }
//     }
//     file = root.openNextFile();
//   }
//   return i;
// }
void open_new_song(String filename) {
   Serial.print(" Open _NEW song..");Serial.println (filename ); 
  music_info.name = filename.substring(1, filename.indexOf("."));
  Serial.print(" audio input..");Serial.println (music_info.name );
  audio.connecttoFS(SD, filename.c_str());
  music_info.runtime = audio.getAudioCurrentTime();
  music_info.length = audio.getAudioFileDuration();
  music_info.volume = audio.getVolume();
  music_info.status = 1;
  music_info.m = music_info.length / 60;
  music_info.s = music_info.length % 60;
}
