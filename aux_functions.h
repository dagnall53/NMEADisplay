#ifndef _AUX_FUN_H_
#define _AUX_FUN_H_
//*** process NMEA packet into Boat variables for display 
#include <Arduino.h>     //necessary for the String, uint16_t etc variable definitions
#include "Structures.h"  // to tell it about the tBoatdata and button structs.
#include <Arduino_GFX_Library.h>  // for the graphics functions

extern Arduino_RGB_Display  *gfx ; //  change if alternate displays !
bool processPacket(const char* buf,  tBoatData &stringBD );

//** Graphics display functions ***
void UpdateData(Button button, instData &data , const char* fmt);

void UpdateLinef(Button& button, const char* fmt, ...) ; // (sequentially) types lines into the button space 

// is always called via  UpdateData void NEWUpdate(Button button,unsigned long updated, const char* fmt, ...);

void UpdateCentered(int h, int v, int width, int height, int bordersize,
                    uint16_t backgroundcol, uint16_t textcol, uint16_t BorderColor, const char* fmt, ...) ;
void UpdateCentered(Button button, const char* fmt, ...);

void GFXBorderBoxPrintf(Button button, const char* fmt, ...) ;

void GFXBorderBoxPrintf(int h, int v, int width, int height, int bordersize,
                        uint16_t backgroundcol, uint16_t textcol, uint16_t BorderColor, const char* fmt, ...);

void AddTitleBorderBox(Button button, const char* fmt, ...);

void AddTitleBorderBox(int h, int v, uint16_t BorderColor, const char* fmt, ...) ;



#endif