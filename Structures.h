/* 
tBoatdata from Timo Llapinen by Dr.András Szép under GNU General Public License (GPL).


*/
#ifndef _BoatData_H_
#define _BoatData_H_


struct MySettings {  //key,default page,ssid,PW,Displaypage, UDP,Mode,serial,Espnow
  int EpromKEY;      // Key is changed to allow check for clean EEprom and no data stored change in the default will result in eeprom being reset
  int DisplayPage;   // start page after defaults 
  char ssid[16];
  char password[16];
  char UDP_PORT[5];  // hold udp port as char to make using keyboard easier for now. use atoi when needed!!
  bool UDP_ON;
  bool Serial_on;
  bool ESP_NOW_ON;
};
struct MyColors {  // for later Day/Night settings
  uint16_t TextColor;
  uint16_t BackColor;
  uint16_t BorderColor;
};


  struct tBoatData {
    unsigned long DaysSince1970;  // Days since 1970-01-01

    double SOG, STW, COG, MagHeading, TrueHeading, Variation,
      GPSTime,  // Secs since midnight,
      Latitude, Longitude, Altitude, HDOP, GeoidalSeparation, DGPSAge,
      WaterTemperature, WaterDepth, Offset,
      RPM,
      WindDirectionT, WindDirectionM, WindSpeedK, WindSpeedM,
      WindAngle;
    int GPSQualityIndicator, SatelliteCount, DGPSReferenceStationID;
    bool MOBActivated;
    char Status;
  };

#endif  // _BoatData_H_
