/* 
_sBoatData from Timo Llapinen by Dr.András Szép under GNU General Public License (GPL).

*/
#ifndef _Structures_H_
#define _Structures_H_
#include <NMEA0183.h>  // for the TL NMEA0183 library functions
#include <NMEA0183Msg.h>
#include <NMEA0183Messages.h> // for the doubleNA

struct _sDisplay_Config {  // will be Display_Config for the JSON set Defaults and user settings for displays 
  char Mag_Var[15]; // got to save double variable as a string! east is positive
  int Start_Page ;
  char PanelName[25];
  char APpassword[25]; 
  char FourWayBR[10] ;
  char FourWayBL[10] ; 
  char FourWayTR[10] ;
  char FourWayTL[10] ;
};


struct _sWiFi_settings_Config {  // MAINLY WIFI AND DATA LOGGING key,ssid,PW,udpport, UDP,serial,Espnow
  int EpromKEY;      // Key is changed to allow check for clean EEprom and no data stored change in the default will result in eeprom being reset
                     //  int DisplayPage;   // start page after defaults
  char ssid[25];
  char password[25];
  char UDP_PORT[5];  // hold udp port as char to make using keyboard easier for now. use atoi when needed!!
  bool UDP_ON;
  bool Serial_on;
  bool ESP_NOW_ON;
  bool Log_ON;
  bool NMEA_log_ON;
  bool BLE_enable;
  int Num_Victron_Devices;
};
struct _MyColors {  // for later Day/Night settings
  uint16_t TextColor;
  uint16_t BackColor;
  uint16_t BorderColor;
  int BoxW;
  int BoxH;
  int FontH;
  int FontS;
};

struct _sInstData {  // struct to hold instrument data AND the time it was updated.
  double data = NMEA0183DoubleNA;
  double lastdata = NMEA0183DoubleNA;
  unsigned long updated;
  bool displayed;  // displayed is used by Digital displays
  bool greyed;     // when the data is OLD! 
  bool graphed;    // is used by Graphs, so you can display digital and graph on same page!
  int source;      // Ready to try an experiment with two GPS to see how they track .
 
};



struct _sBoatData {
  unsigned long DaysSince1970;  // Days since 1970-01-01

  _sInstData SOG, STW, COG, Latitude, Longitude,MagHeading, TrueHeading, WaterDepth,
    WindDirectionT, WindDirectionM, WindSpeedK, WindSpeedM, WindAngleApp, WindAngleGround;
     //_sInstData will be used with NEWUPdate and greys if old

  double SatsInView,Variation, GPSTime, GPSDate,  // keep some GPS stuff in double ..
    Altitude, HDOP, GeoidalSeparation, DGPSAge;
    
  bool MOBActivated;

};

struct _sButton {
  int h, v, width, height, bordersize;
  uint16_t BackColor, TextColor, BorderColor;
  int Font;                  //-1 == not forced (not used?)
  bool Keypressed;           //used by keypressed
  unsigned long LastDetect;  //used by keypressed
  int PrintLine;             // used for UpdateLinef()
  bool screenfull,debugpause;
};

struct Phv {   // struct for int positions h v typically on screen 
  int h, v;
};

/// for Victron stuff:





struct _sVicdevice {  // will need a new equivalent to toNewStruct(char *field, _sInstData &data); 
  int Device_Type=0;   // selector equivalent to victronRecordType {1=solar 2=smartshunt..     }
  double data; // use for main reading, assumed voltage
  double lastdata ;
  double data2 ; // use for secondary reading, assumed current
  double lastdata2 ;
  double data3 ; // use for tertiary reading, aux batt, temp? etc.. 
  double lastdata3 ;
  double data4; // use for quadrenary (?) reading, aux batt, temp? etc.. 
  double lastdata4 ;
  
  unsigned long updated;
  bool displayed;  // displayed is used by Digital displays
  bool greyed;     // when the data is OLD! 
  bool graphed;    // is used by Graphs, so you can display digital and graph on same page!
};

struct _sVictronData {
  _sVicdevice MainBatteryshunt, AuxbatteryShunt, EngineBatteryshunt,
  Mainsolar, Secondsolar, mainscharger;
};
struct _sMyVictronDevices{   // equivalent to _sDisplay_Config all known victron devices MAc and encryption keys.
                //10 index for multiple saved instrument settings first 
  char charMacAddr[10][13];   // a 12 char (+1!) array  typ= "ea9df3ebc625"  
  char charKey [10][33];      //32 etc...
  char FileCommentName [10][32];
  int displayV[10];
  int displayH[10];
  //byte byteMacAddr[10][7];        // 6 bytes for MAC - initialized by setup() from quoted strings  ???NOPE!!!
 // byte byteKey[10][17];           // 16 bytes for encryption key - initialized by setup() from quoted strings
 // char cachedDeviceName[10][33];
};

#endif  // _Structures_H_
