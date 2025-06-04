#ifndef ESP_NOW_FILES_H
#define ESP_NOW_FILES_H

#include <Arduino.h>  // else does not recognise variable names

// these must be defined in main ino
//extern bool line_EXT;
extern char nmea_EXT[500];
extern bool EspNowIsRunning;

//local


bool Start_ESP_EXT();  // start espnow 
 
bool Test_ESP_NOW(); // for the loop to update 

void EXTHeartbeat();
void EXTSEND(const char* buf);
void EXTSENDf(const char* fmt, ...);


#endif