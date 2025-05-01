#ifndef ESP_NOW_FILES_H
#define ESP_NOW_FILES_H

#include <Arduino.h>  // else does not recognise variable names

// these must be defined in main ino
//extern bool line_EXT;
extern char nmea_EXT[500];
extern bool EspNowIsRunning;

//local


bool Start_ESP_EXT();  // start espnow and run Test_EspNOW() when data is seen
void Test_EspNOW(const uint8_t* mac, const uint8_t* incomingData, int len);
bool UpdateEspNow(); // for the loop to update 

void EXTHeartbeat();
void EXTSEND(const char* buf);
void EXTSENDf(const char* fmt, ...);


#endif