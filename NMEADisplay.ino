
/*************************************************************
  This sketch implements a simple serial receive terminal
  program for monitoring  messages

TTGO comments 22/0/25
Does not use hardware scroling
DO NOT FORGET TO CHANGE DISPLAY SELECT in tft-espi User_Setup_Select.h !
Or use UserSetup 

 *************************************************************/
//#include <ESPEssentials.h>
#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager

#include <WiFi.h>
#include <WiFiUdp.h>
#include <TFT_eSPI.h>  // Hardware-specific library  DO NOT FORGET TO CHANGE DISPLAY SELECT in User_Setup_Select.h !
#include <SPI.h>

#include <esp_now.h>
#include <esp_wifi.h>
#include <WebServer.h>
#include "ESP_NOW_files.h"

// include library to read and write from flash memory
#include <EEPROM.h>

// define the number of bytes you want to access
#define EEPROM_SIZE 512

TFT_eSPI tft = TFT_eSPI();  // Invoke custom library

// The scrolling area must be a integral multiple of TEXT_HEIGHT
#define TEXT_HEIGHT 16     // Height of text in pixels  to be printed and scrolled
#define BOT_FIXED_AREA 1   // Number of Text Lines in bottom fixed area
#define TOP_FIXED_AREA 16  // Number of pixels lines in top fixed area

#define YMAX 135  // (size of) Bottom of screen in pixels
#define XMAX 240  // (size of) width of screen in pixels
#define Project "DMon"
#define BUTTON1PIN 35
#define BUTTON2PIN 0
bool ButtonPressed = false;
bool ModeUpdateFinished = true;
int Button_pressed;

#define BAUD_RATE 115200
//A structure EpromKEY,UDP_PORT,Mode,UDP_ON,Serial_on ESP_NOW_ON   to save stuff for eeprom and modes
struct MySettings {
  int EpromKEY;  // to allow check for clean EEprom and no data stored
  int UDP_PORT;
  int Mode;
  bool UDP_ON;
  bool Serial_on;
  bool ESP_NOW_ON;
  uint8_t ListTextSize;  // fontsize for listed text (2 is readable, 1 gives more lines)// all off
  char ssid[16];
  char password[16];
};

MySettings Default_Settings = { 127, 2002, 1, false, true, true, 2, "N2K0183-proto", "12345678" };
MySettings Saved_Settings;
MySettings Current_Settings;
char UDPPORT[10];  // to pass value from wifimanager
int UDP_PORT;      // see Default_Settings for default

// The initial y coordinate of the top of the scrolling area
uint16_t yStart = TOP_FIXED_AREA;
// yArea must be a integral multiple of TEXT_HEIGHT
uint16_t yBottom = YMAX - (BOT_FIXED_AREA * TEXT_HEIGHT);
uint8_t NumberoftextLines = (YMAX - TOP_FIXED_AREA - (BOT_FIXED_AREA * TEXT_HEIGHT)) / TEXT_HEIGHT;
// The initial y coordinate of the top main text line
uint16_t yDraw = yStart + TEXT_HEIGHT;

// Keep track of the drawing x coordinate
uint16_t xPos = 0;
uint8_t WriteLine;  // line of text (0.. )
// For the byte we read from the serial port
byte data = 0;

// A few test variables used during debugging
bool change_colour = 1;
bool selected = 1;

// We have to blank the top line each time the display is scrolled, but this takes up to 13 milliseconds
// for a full width line, meanwhile the serial buffer may be filling... and overflowing
// We can speed up scrolling of short text lines by just blanking the character we drew
int blank[19];  // We keep all the strings pixel lengths to optimise the speed of the top line blanking



WiFiUDP Udp;

//WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
WiFiManager wifiManager;
WiFiManagerParameter custom_field;  // global param ( for non blocking w params )


#define USEWM


//*********** Button stuff *****************
void IRAM_ATTR toggleButton1() {
  ButtonPressed = true;
  if (digitalRead(BUTTON2PIN) == LOW) {
    Button_pressed = 3;
  } else {
    Button_pressed = 1;
  }
}

void IRAM_ATTR toggleButton2() {
  ButtonPressed = true;
  if (digitalRead(BUTTON1PIN) == LOW) {
    Button_pressed = 3;
  } else {
    Button_pressed = 2;
  }
}

#define BufferLength 500
bool line_1;
char nmea_1[500];

bool line_U = false;
char nmea_U[BufferLength];  // NMEA buffer for UDP input port

// *************  ESP-NOW variables and functions in ESP_NOW_files************
bool line_EXT;
char nmea_EXT[500];
bool EspNowIsRunning = false;
// byte peerAddress[6];
// const byte peerAddress_def[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };  // all receive
// esp_now_peer_info_t peerInfo;
// bool EspNowIsRunning = false;
// unsigned long Last_EXT_Sent;


void StartTFT() {
  // Setup the TFT display
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  //tft.fillRect(0, 0, XMAX, 16, TFT_BLUE);
  tft.setCursor(0, 0, 2);
}

void connectwithsettings() {
  uint32_t StartTime =millis();
  tft.print(" Using EEPROM settings");
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(Current_Settings.ssid, Current_Settings.password);
  while ((WiFi.status() != WL_CONNECTED)&& (millis() <= StartTime+ 10000)) { //Give it 10 seconds 
    delay(500);
    tft.print(".");
  }
}

void connectwithWM() {
  bool res;
  Serial.println("---Running WIFI Manager---");
  tft.println(" Running WIFI Manager");
  tft.println(" Connect to WifiManager AP");
  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  res = wifiManager.autoConnect("NMEA_DISPLAY", "12345678");  // password protected ap
  if (!res) {
    tft.println(" WM Failed to connect");
    Serial.println("Failed to connect");
    // ESP.restart(); //?
  } else {
    //if you get here you have connected to the WiFi
    tft.println(" WM Connected ! ");
    Serial.println("WM connected.:)");
  }
}

void setup() {
  Serial.begin(BAUD_RATE);
  StartTFT();
  EEPROM_READ();  // setup and read saved variables into Saved_Settings
  //dataline(Saved_Settings, "Saved");
  //dataline(Default_Settings, "Default");
  Current_Settings = Saved_Settings;
  if (Current_Settings.EpromKEY != 127) {
    Current_Settings = Default_Settings;
    EEPROM_WRITE();
  }
  dataline(Current_Settings, "Current");


  pinMode(BUTTON1PIN, INPUT);
  pinMode(BUTTON2PIN, INPUT);
  attachInterrupt(BUTTON1PIN, toggleButton1, FALLING);
  attachInterrupt(BUTTON2PIN, toggleButton2, FALLING);


  //how to connect ?
  tft.setCursor(0, 0, 2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print(" STARTING CONNECT");
  Serial.print(" STARTING CONNECT");
  #ifdef USEWM
    //*********** use WifiManager ******************
  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter customUDPport("UDP_Port", "UDP port", "Number", 10);
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //add all your parameters here
  wifiManager.addParameter(&customUDPport);
  connectwithWM();
  #else 
  connectwithsettings();
  #endif

  if (WiFi.status() == WL_CONNECTED){    tft.println(" Connected ! ");
    Serial.println("connected.:)");}
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.fillRect(0, 0, XMAX, 16, TFT_BLUE);
  tft.setCursor(0, 0, 2);
  //tft.printf("%s:", Project);
  tft.print(WiFi.SSID());
  tft.print(" ");
  tft.println(WiFi.localIP());
  Serial.printf(" *Running with:  ssid<%s> psk<%s> \n", WiFi.SSID(), WiFi.psk());
  strcpy(Current_Settings.ssid, WiFi.SSID().c_str());
  strcpy(Current_Settings.password, WiFi.psk().c_str());
  // get any values from WiFi Manager ...
  strcpy(UDPPORT, customUDPport.getValue());
  if (atoi(UDPPORT) != 0) {  // Number is the default, so converts to zero only changed if we set a real value in WiFi Manager
    Serial.printf("Updating UDP Port (%d)  from WiFi Manager?",atoi(UDPPORT));
    Current_Settings.UDP_PORT = atoi(UDPPORT);  // UPDATE and convert char array to int
    EEPROM_WRITE();                             //Changed - Update eeprom
  } else {
    Serial.println("Keeping stored UDP port");
  }

  line_1 = false;
  WriteLine = 1;

  Udp.begin(Current_Settings.UDP_PORT);
  Serial.println("Setting ESP-NOW");
  tft.println("\nStarting ESP_now");
  if (Start_ESP_EXT()) {
    Serial.println("   Success to add peer");
    tft.println("   Success to add peer");
  } else {
    Serial.println("   Failed to add peer");
    tft.println("   Failed to add peer");
  }
  if (EspNowIsRunning) {
    Serial.println("   ESP-NOW Init Success");
    tft.println("ESP-NOW Init Success");
  } else {
    Serial.println("   ESP-NOW Init Failed");
    tft.println("ESP-NOW Init Failed");
  }

  Serial.print("   Mac Address: ");
  Serial.println(WiFi.macAddress());
  tft.println(WiFi.macAddress());
  //Show current housekeeping settings
  dataline(Current_Settings, "SET");
  // Change colour for  text zone
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

void ModeFunctions() {
  #ifdef USEWM
  wifiManager.process();
  #endif
  if ((ButtonPressed != 0) && ModeUpdateFinished) {
    ModeUpdate(Button_pressed);
    ButtonPressed = false;
  };
}

void ModeUpdate(uint8_t button) {
  if (ButtonPressed) {
    ModeUpdateFinished = false;  // token to prevent multiple calls
    WriteLine = 0;
    DrawON_line(1, "Button pressed", 2, TFT_BLUE);
    Serial.printf(" Mode change <%d>", button);
    if (button == 3) {
      // V simple configure to allow both buttons to trigger EEPROM save and  WiFiManager reset
      Serial.println("Reset EEPROM");
      DrawON_line(1, "Reset EEprom: Hold for WIFi reset", 2, TFT_BLUE);
      Current_Settings = Default_Settings;
      EEPROM_WRITE();

      delay(3000);  // wait before testing pins again reset wifi manager delay hold
      if ((digitalRead(BUTTON1PIN) == LOW) && (digitalRead(BUTTON2PIN) == LOW)) {
        DrawON_line(3, "Button Held", 2, TFT_BLUE);
        delay(1000);
        DrawON_line(4, "Erasing Config, restarting", 2, TFT_BLUE);
        delay(1000);
        wifiManager.resetSettings();
        ESP.restart();
      }
    } else {

      if (button == 1) { Current_Settings.Mode = Current_Settings.Mode + 1; }
      if (button == 2) { Current_Settings.Mode = Current_Settings.Mode - 1; }

      if (Current_Settings.Mode > 10) { Current_Settings.Mode = 10; }
      if (Current_Settings.Mode < 0) { Current_Settings.Mode = 0; }
      //change display
      //tft.printf("Button %d Pressed! Current_Settings.Mode: %d",button,Current_Settings.Mode);
      if (Current_Settings.Mode == 0) {
        Current_Settings.Serial_on = true;
        Current_Settings.UDP_ON = false;
        Current_Settings.ESP_NOW_ON = false;
        Current_Settings.ListTextSize = 2;
      }
      if (Current_Settings.Mode == 1) {
        Current_Settings.Serial_on = true;
        Current_Settings.UDP_ON = true;
        Current_Settings.ESP_NOW_ON = false;
        Current_Settings.ListTextSize = 1;
      }
      if (Current_Settings.Mode == 2) {
        Current_Settings.Serial_on = true;
        Current_Settings.UDP_ON = false;
        Current_Settings.ESP_NOW_ON = true;
        Current_Settings.ListTextSize = 1;
      }
      if (Current_Settings.Mode == 3) {
        Current_Settings.Serial_on = true;
        Current_Settings.UDP_ON = true;
        Current_Settings.ESP_NOW_ON = true;
        Current_Settings.ListTextSize = 1;
      }
      if (Current_Settings.Mode == 4) {
        Current_Settings.Serial_on = true;
        Current_Settings.UDP_ON = true;
        Current_Settings.ESP_NOW_ON = false;
        Current_Settings.ListTextSize = 2;
      }
      if (Current_Settings.Mode == 5) {
        Current_Settings.Serial_on = true;
        Current_Settings.UDP_ON = false;
        Current_Settings.ESP_NOW_ON = true;
        Current_Settings.ListTextSize = 2;
      }
      if (Current_Settings.Mode == 6) {
        Current_Settings.Serial_on = true;
        Current_Settings.UDP_ON = true;
        Current_Settings.ESP_NOW_ON = true;
        Current_Settings.ListTextSize = 2;
      }
      if (Current_Settings.Mode == 10) {
        Current_Settings.Serial_on = true;
        Current_Settings.UDP_ON = true;
        Current_Settings.ESP_NOW_ON = true;
        Current_Settings.ListTextSize = 2;
      }

      // fill in bottom line of setup
      dataline(Current_Settings, "ModeUpdate");
      ButtonPressed = false;
      EEPROM_WRITE();  // nothing fancy, just save it
    }                  //end of +- mode changes
    ModeUpdateFinished = true;
  }
}

void dataline(MySettings A, String Text) {
  //tft.fillRect(0, TOP_FIXED_AREA, XMAX, YMAX - TOP_FIXED_AREA, TFT_BLACK);
  tft.fillRect(0, YMAX - TEXT_HEIGHT, XMAX, TEXT_HEIGHT, TFT_BLACK);
  tft.setCursor(0, yBottom);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.printf("%d, %d SER<%d>", A.EpromKEY, A.Mode, A.Serial_on);
  tft.setTextColor(TFT_BLUE, TFT_BLACK);
  tft.printf("UDP<%d><%d> ", A.UDP_PORT, A.UDP_ON);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.printf("ESP<%d>", A.ESP_NOW_ON);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);  // reset to initial state
  Serial.printf("%d Dataline display %s: Mode<%d> Ser<%d> UDPPORT<%d> UDP<%d>  ESP<%d> ", A.EpromKEY, Text, A.Mode, A.Serial_on, A.UDP_PORT, A.UDP_ON, A.ESP_NOW_ON);
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
  if (A.Mode == B.Mode) { same = true; }
  if (A.ListTextSize == B.ListTextSize) { same = true; }
  return same;
}

void DrawON_line(int Line, String text, uint8_t font, uint32_t TEXT_Colour) {
  int32_t ypos = Line * TEXT_HEIGHT;
  tft.setTextColor(TEXT_Colour, TFT_BLACK);
  tft.fillRect(0, ypos, XMAX, TEXT_HEIGHT, TFT_BLACK);
  tft.drawString(text, 0, ypos, font);
}

void ShowData(bool& Line_Ready, char* buf, uint8_t font, uint32_t TEXT_Colour) {  //&sets pointer so I can modify Line_Ready in this function
  //if (Line_Ready){
  if (buf[0] != 0) {
    WriteLine = WriteLine + 1;
    if (WriteLine * font > (NumberoftextLines * 2)) { WriteLine = 1; }
    int32_t ypos = 8 + (WriteLine * font * (TEXT_HEIGHT / 2));
    Line_Ready = false;
    tft.setTextColor(TEXT_Colour, TFT_BLACK);
    tft.fillRect(0, ypos, XMAX, font * (TEXT_HEIGHT / 2), TFT_BLACK);
    tft.drawString(buf, 0, ypos, font);
    Line_Ready = false;
    if (TEXT_Colour == TFT_GREEN) {
      Serial.printf("esp_now :%s", buf);
      buf[0] = 0;
      return;
    }
    if (TEXT_Colour == TFT_BLUE) {
      Serial.printf("UDP     :%s", buf);
      buf[0] = 0;
      return;
    }
    if (TEXT_Colour == TFT_WHITE) {
      Serial.printf("Serial  :%s", buf);
      buf[0] = 0;
      return;
    }
  }
}


void TestInputsOutputs() {
  if (Current_Settings.ESP_NOW_ON) { ShowData(line_EXT, nmea_EXT, Current_Settings.ListTextSize, TFT_GREEN); }
  if (Current_Settings.Serial_on) {
    Test_Serial_1();
    ShowData(line_1, nmea_1, Current_Settings.ListTextSize, TFT_WHITE);
  }
  if (Current_Settings.UDP_ON) {
    Test_U();
    ShowData(line_U, nmea_U, Current_Settings.ListTextSize, TFT_BLUE);
  }
}

void loop(void) {
  //ESPEssentials::handle();
  //EventTiming("START");
  EXTHeartbeat();
  TestInputsOutputs();
  ModeFunctions();
}


//****  ESP-Now functions
// void Start_ESP_EXT() {  // start espnow and run Test_EspNOW() when data is seen
//   Serial.println("Setting ESP-NOW");
//   tft.println("\nStarting ESP_now");
//   memcpy(peerInfo.peer_addr, peerAddress, 6);
//   peerInfo.encrypt = false;
//   peerInfo.channel = 0;  // why the comment?
//   if (esp_now_init() == ESP_OK) {
//     EspNowIsRunning = true;
//     Serial.println("   ESP-NOW Init Success");
//     tft.println("ESP-NOW Init Success");
//   } else {
//     EspNowIsRunning = false;
//     Serial.println("   ESP-NOW Init Failed");
//     tft.println("ESP-NOW Init Failed");
//   }
//   esp_now_register_recv_cb(Test_EspNOW);
//   if (esp_now_add_peer(&peerInfo) == ESP_OK) {
//     Serial.println("   Success to add peer");
//     tft.println("   Success to add peer");
//   } else {
//     Serial.println("   Failed to add peer");
//     tft.println("   Failed to add peer");
//   }
//   Serial.print("   Mac Address: ");
//   Serial.println(WiFi.macAddress());
//   tft.println(WiFi.macAddress());
//   delay(2000);  // allow time to read it!
//   Serial.flush();
// }
// //EXT send functions "esp_now"
// void Test_EspNOW(const uint8_t* mac, const uint8_t* incomingData, int len) {
//   EspNowIsRunning = true;
//   //if (!line_EXT) { // wait until previous capture has been processed?
//   memcpy(&nmea_EXT, incomingData, sizeof(nmea_EXT));
//   //Serial.printf( "esp_now INTERRUPT :%s",nmea_EXT);
//   line_EXT = true;  //}
// }
// void testHeartbeat() {
//   static unsigned long lastMessageSent;
//   if (!EspNowIsRunning) { return; }
//   if (millis()>=lastMessageSent+1000) {  // 1 sec.
//     lastMessageSent = millis();               // but we also update Last_EXT_Sent in the EXT send..
//      Serial.printf("TEST: esp_sending? running(%s) ch%d \r\n", EspNowIsRunning ? "ON" : "OFF", WiFi.channel());  // or other prints of status!
//     EXTSENDf("_%s_:ch%d_\r\n", Project, WiFi.channel());  // BUT We must send with a \r\n!!
//   }
// }
// void EXTSEND(const char* buf) {  // same format as TCP send etc..  83 size for NMEA 250 max for ESP-NOW link  NOT BIG ENOUGH FOR SOME RAW!!
//   if (!EspNowIsRunning) { return; }
//   char myData[248];                       // max size of esp_now_send messages
//   strcpy(myData, buf);                    // to avoid overflowing in this function..?
//   strcat(myData, "\r\n");                 // Adding CRLF wass eqivalent of Println.. and we now "print" everywhere
//   if (strlen(myData) >= 247) { return; }  // // sendAdvice("Message too big for ESP_EXT link");}but test before going further!
//   if (strlen(myData) <= 5) { return; }    // should never be less than 5 !! Just to try to catch extra CRLF's ?
//   // sendAdvicef(" esp-now Sending: (%s) Wifich<%i>  peer_ch<%i> ", myData, WiFi.channel(), peerInfo.channel);
//   //sendAdvicef(" esp-now Sending: (length %i) Wifich<%i>  peer_ch<%i> ", strlen(myData), WiFi.channel(), peerInfo.channel);
//   Last_EXT_Sent = millis();
//   esp_err_t result = esp_now_send(peerInfo.peer_addr, (uint8_t*)&myData, sizeof(myData));
//   //if (result == ESP_OK) {sendAdvicef(" esp-now Sent %i in msg max [%i] long",strlen(myData), sizeof(myData));}
//   // if (result != ESP_OK) {EspNowIsRunning=false;}   // but then we need something in Loop to re-establish the link ??
// }

// void EXTSENDf(const char* fmt, ...) {  //complete object type suitable for holding the information needed by the macros va_start, va_copy, va_arg, and va_end.
//   if (!EspNowIsRunning) { return; }
//   static char msg[300] = { '\0' };  // used in message buildup
//   va_list args;
//   va_start(args, fmt);
//   vsnprintf(msg, 128, fmt, args);
//   va_end(args);
//   // add checksum?
//   int len = strlen(msg);
//   EXTSEND(msg);
// }


void Test_Serial_1() {  // UART0 port P1
  static bool LineReading_1 = false;
  static int Skip_1 = 1;
  static int i_1;
  byte b;
  if (!line_1) {                  // ONLY get characters if we are NOT still processing the last message!
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
          return;
        }
        if (i_1 > 150) {
          LineReading_1 = false;
          i_1 = 0;
          return;
        }
      }
    }
  }
}

void Test_U() {  // check if udp packet  has arrived
  static int Skip_U = 1;
  if (!line_U) {  // only process if we have dealt with the last line.
    nmea_U[0] = 0x00;
    int packetSize = Udp.parsePacket();
    if (packetSize) {  // Deal with UDP packet
      if (packetSize >= (BufferLength + 4)) {
        Udp.flush();
        return;
      }  // Simply discard if too long
      int len = Udp.read(nmea_U, BufferLength);
      byte b = nmea_U[0];
      nmea_U[len] = 0;
      line_U = true;
    }  // udp PACKET DEALT WITH
  }
}
// void ScanForSSID(char* look_for, int ScanChannel) {  // defined as separate function for debug webpage to avoid any conflicts with the ScanandConnect..
//   int n = WiFi.scanNetworks(false, false, false, 50, ScanChannel, nullptr, nullptr);
//   String st = "&nbsp &nbsp &nbsp Scanning WiFi Ch(" + String(ScanChannel) + ")  Found ";
//   if (ScanChannel == WiFi.channel()) { st = st + ":<b>(ME)</b>(" + String(ssidAP) + ") +"; }
//   if (n != 0) {
//     for (int i = 0; i < n; ++i) {
//       st = st + " : " + String(WiFi.SSID(i)) + " (Strength :" + String(WiFi.RSSI(i)) + ")  ";
//     }
//   } else {
//     st = st + ": NONE";
//   }
//   st = st + "<br>";
//   webserver.sendContent(st);
// }

// void ScanAndConnect(char* look_for, int ScanChannel) {
//   ScanChannelFound = 0;
//   unsigned long Updated = millis();
//   int n = WiFi.scanNetworks(false, false, false, 50, ScanChannel, nullptr, nullptr);
//   for (int i = 0; i < n; ++i) {
//     if (String(look_for) == String(WiFi.SSID(i))) {
//       ScanChannelFound = WiFi.channel(i);
//       //WiFi.begin(ssidST,passST, ScanChannelFound);
//       if ( String(passST) == "NULL" ) { WiFi.begin(ssidST, NULL, ScanChannelFound); }
//       else { WiFi.begin(ssidST, passST, ScanChannelFound); }
//     }
//   }
//   // if (ScanChannel != 0) { return ; }     // only repports the full scan
//   if (OutOfSetup) { SetSerialFor_115200(); }
//   if(ScanChannelFound) { Serial.printf("   FOUND <%s> Ch<%d> in %ums\r\n", String(look_for), ScanChannelFound, (millis() - Updated)); }
//   else { Serial.printf("   not found <%s> Ch<%d> in %ums\r\n", String(look_for), ScanChannel, (millis() - Updated)); }
//   if (OutOfSetup) { SetSerialFor_DefaultBaud(); }
// }
//************ TIMING FUNCTIONS FOR TESTING PURPOSES ONLY ******************
//Note this is also an example of how useful Function overloading can be!!
// void EventTiming(String input ){
//   EventTiming(input,1); // 1 should be ignored as this is start or stop! but will also give immediate print!
// }

// void EventTiming(String input, int number){// Event timing, Usage START, STOP , 'Descrption text'   Number waits for the Nth call before serial.printing results (Description text + results).
//   static unsigned long Start_time;
//   static unsigned long timedInterval;
//   static unsigned long _MaxInterval;
//   static unsigned long SUMTotal;
//   static int calls = 0;  static int reads = 0;
//   long NOW=micros();
//   if (input == "START") { Start_time = NOW; return ;}
//   if (input == "STOP") {timedInterval= NOW- Start_time; SUMTotal=SUMTotal+timedInterval;
//                        if (timedInterval >= _MaxInterval){_MaxInterval=timedInterval;}
//                        reads++;
//                        return; }
//   calls++;
//   if (calls < number){return;}
//   if (reads>=2){
//     //if (OutOfSetup) { SetSerialFor_115200();}
//     if (calls >= 2) {Serial.print("\r\n TIMING ");Serial.print(input); Serial.print(" Using ("); Serial.print(reads);Serial.print(") Samples");
//         Serial.print(" last: ");Serial.print(timedInterval);
//         Serial.print("us Average : ");Serial.print(SUMTotal/reads);
//         Serial.print("us  Max : ");Serial.print(_MaxInterval);Serial.println("uS");}
//     else {Serial.print("\r\n TIMED ");Serial.print(input); Serial.print(" was :");Serial.print(timedInterval);Serial.println("uS");}
//     _MaxInterval=0;SUMTotal=0;reads=0; calls=0;
//     //if (OutOfSetup) { SetSerialFor_DefaultBaud(); }
//   }
// }
void EEPROM_WRITE() {
  // save my current settings
  //dataline(Current_Settings, "EEPROM_save");
  Serial.println("SAVING EEPROM");
  EEPROM.put(0, Current_Settings);
  EEPROM.commit();
  delay(50);
}
void EEPROM_READ() {
  EEPROM.begin(512);
  Serial.println("READING EEPROM");
  EEPROM.get(0, Saved_Settings);
  //dataline(Saved_Settings, "EEPROM_Read");
}
