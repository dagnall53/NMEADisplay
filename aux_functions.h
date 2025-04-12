#ifndef _AUX_FUN_H_
#define _AUX_FUN_H_
//*** process NMEA packet into Boat variables for display 
#include <Arduino.h>     //necessary for the String, uint16_t etc variable definitions
#include "Structures.h"  // to tell it about the tBoatdata and button structs.
#include <Arduino_GFX_Library.h>  // for the graphics functions

extern Arduino_RGB_Display  *gfx ; //  change if alternate displays !
bool processPacket(const char* buf,  tBoatData &stringBD );

//** Graphics display functions ***
//void UpdateData(Button button, instData &data , const char* fmt);
// Gives two size display, Digits in big, decimal and efverything after in small font
void UpdateDataTwoSize(bool horizCenter, bool vertCenter,int bigfont, int smallfont,Button button, instData &data, const char *fmt);

void UpdateLinef(int font,Button& button, const char* fmt, ...) ; // (sequentially) types lines (in 'font') into the selected button space 
void UpdateLinef(uint16_t color,int font,Button& button, const char* fmt, ...) ; // (sequentially) types lines (in 'font') into the selected button space 

void GFXBorderBoxPrintf(Button button, const char* fmt, ...) ;
//full version without button structure
void GFXBorderBoxPrintf(int h, int v, int width, int height, int bordersize,
                        uint16_t backgroundcol, uint16_t textcol, uint16_t BorderColor, const char* fmt, ...);

void AddTitleBorderBox(int font,Button button, const char* fmt, ...);
void AddTitleInsideBox(int font,int pos, Button button, const char *fmt, ...);

void PTriangleFill(Phv P1, Phv P2, Phv P3, uint16_t COLOUR);
void Pdrawline(Phv P1, Phv P2, uint16_t COLOUR) ;
void PfillCircle(Phv P1, int rad, uint16_t COLOUR);

//void DrawGraph (Button button, instData &DATA, double dmin, double dmax);
void DrawGraph (Button button, instData &DATA, double dmin, double dmax, int font,const char *units,const char *msg);
void DrawGPSPlot(bool reset, Button button,tBoatData BoatData, double magnification );

int Circular(int x, int min,int max);
int GraphRange( double data, int low, int high, double dmin ,double dmax );

void toNewStruct(char *field, instData &data);
double DoubleInstdataAdd(instData &data1, instData &data);//returns a double of value data.data +data1.data;



#endif